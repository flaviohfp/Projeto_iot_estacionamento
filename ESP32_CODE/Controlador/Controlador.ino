// ESP32 #2: UART RX2=16, LEDs RGB, LCD I2C, servidor e Firebase.
// Bibliotecas: ArduinoJson 7 e hd44780 (Bill Perry).
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <time.h>
#include "Config.h"
#include "Model.h"
#include "GoogleRoots.h"
#include "SiteAssets.h"

// A API copia a fila/historico para responder sem segurar o mutex durante HTTP.
SET_LOOP_TASK_STACK_SIZE(16384);

WebServer server(80);
hd44780_I2Cexp lcd; // Detecta o endereco e o mapeamento do adaptador I2C.
Preferences preferences;
SemaphoreHandle_t mutexDados;
Persistencia dados = {};
bool sensorValido[4] = {};
uint32_t ultimoFrame = 0;
bool recebeuFrame = false;
time_t ultimaLeituraValida = 0;
bool lcdDisponivel = false;
bool nvsDisponivel = false;
bool firebaseOk = false;
uint32_t ultimoSucessoFirebase = 0;
char erroFirebase[160] = "Aguardando Wi-Fi e horario NTP";
String linhaSerial;
bool linhaGrande = false;
String token;
uint32_t tokenCriado = 0;
uint32_t tokenDuracao = 0;
uint32_t bootId;
uint32_t eventoSequencia = 0;

time_t agoraUTC() {
  time_t agora = time(nullptr);
  return agora > 1700000000 ? agora : 0;
}

String isoUTC(time_t valor) {
  if (!valor) return "";
  struct tm horario;
  gmtime_r(&valor, &horario);
  char texto[25];
  strftime(texto, sizeof(texto), "%Y-%m-%dT%H:%M:%SZ", &horario);
  return String(texto);
}

bool sensoresSaudaveis() { // Chamado com mutex adquirido.
  if (!recebeuFrame || millis() - ultimoFrame > SENSORES_TIMEOUT_MS) return false;
  for (int i = 0; i < 4; ++i) if (!sensorValido[i]) return false;
  return true;
}

void persistir() { // Chamado com mutex adquirido, apenas nas mudancas.
  if (nvsDisponivel && preferences.putBytes("estado", &dados, sizeof(dados)) != sizeof(dados)) {
    Serial.println("Falha ao salvar estado/fila na memoria NVS.");
  }
}

Snapshot snapshot() {
  Snapshot s;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  s.dados = dados;
  s.sensoresOk = sensoresSaudaveis();
  s.ultimaAtualizacao = ultimaLeituraValida;
  s.idadeLeituraMs = recebeuFrame ? millis() - ultimoFrame : UINT32_MAX;
  xSemaphoreGive(mutexDados);
  return s;
}

void registrarEvento(int i, bool ocupada, time_t agora) {
  Evento evento = {};
  snprintf(evento.id, sizeof(evento.id), "%08lx-%08lx-%lu",
    (unsigned long)(ESP.getEfuseMac() & 0xffffffff), (unsigned long)bootId,
    (unsigned long)++eventoSequencia);
  evento.vaga = i + 1;
  evento.entrada = ocupada;
  evento.dataHora = agora;
  evento.inicio = ocupada ? agora : dados.vagas[i].entrada;
  if (dados.quantidade < MAX_EVENTOS) {
    dados.pendentes[dados.quantidade++] = evento;
  } else {
    ++dados.descartados;
    Serial.println("Fila offline cheia (32 eventos); evento disponivel apenas no historico local.");
  }
  int tamanho = min((int)dados.totalHistorico, MAX_EVENTOS - 1);
  memmove(&dados.historico[1], &dados.historico[0], tamanho * sizeof(Evento));
  dados.historico[0] = evento;
  dados.totalHistorico = min((int)dados.totalHistorico + 1, MAX_EVENTOS);
}

void receberFrame(const String& linha) {
  JsonDocument doc;
  if (deserializeJson(doc, linha) || doc["v"].as<int>() != 1) return;
  JsonArrayConst vagas = doc["vagas"].as<JsonArrayConst>();
  if (vagas.size() != 4) return;
  for (int i = 0; i < 4; ++i) {
    if (vagas[i]["numero"].as<int>() != i + 1 ||
        !vagas[i]["valida"].is<bool>() || !vagas[i]["ocupada"].is<bool>()) return;
  }
  time_t agora = agoraUTC();
  bool mudou = false;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  recebeuFrame = true;
  ultimoFrame = millis();
  for (int i = 0; i < 4; ++i) {
    sensorValido[i] = vagas[i]["valida"].as<bool>();
    if (!sensorValido[i]) continue;
    bool ocupada = vagas[i]["ocupada"].as<bool>();
    if (!dados.vagas[i].conhecida) {
      // A primeira leitura e um estado inicial, sem inventar horario de entrada.
      dados.vagas[i] = {true, ocupada, 0};
      mudou = true;
    } else if (ocupada != dados.vagas[i].ocupada) {
      registrarEvento(i, ocupada, agora);
      dados.vagas[i].ocupada = ocupada;
      dados.vagas[i].entrada = ocupada ? agora : 0;
      mudou = true;
    }
  }
  if (sensoresSaudaveis() && agora) ultimaLeituraValida = agora;
  if (mudou) persistir();
  xSemaphoreGive(mutexDados);
}

void lerUART() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      if (!linhaGrande && linhaSerial.length()) receberFrame(linhaSerial);
      linhaSerial = "";
      linhaGrande = false;
    } else if (c != '\r' && !linhaGrande) {
      if (linhaSerial.length() < 1024) linhaSerial += c;
      else { linhaGrande = true; linhaSerial = ""; }
    }
  }
}

void escreverLinhaLCD(int linha, const String& texto) {
  String completa = texto.substring(0, LCD_COLUNAS);
  while (completa.length() < LCD_COLUNAS) completa += ' ';
  lcd.setCursor(0, linha);
  lcd.print(completa);
}

void atualizarSaidas() {
  bool validos[4];
  bool ocupadas[4];
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  bool recente = recebeuFrame && millis() - ultimoFrame <= SENSORES_TIMEOUT_MS;
  for (int i = 0; i < 4; ++i) {
    validos[i] = recente && sensorValido[i] && dados.vagas[i].conhecida;
    ocupadas[i] = dados.vagas[i].ocupada;
  }
  xSemaphoreGive(mutexDados);
  int livres = 0;
  bool todasValidas = true;
  String linha = "";
  for (int i = 0; i < 4; ++i) {
    bool verde = validos[i] && !ocupadas[i];
    bool vermelho = validos[i] && ocupadas[i];
    // Na falha, ambas as cores apagadas: a vaga nunca aparece como livre.
    digitalWrite(pinoVerde[i], (verde != RGB_ANODO_COMUM) ? HIGH : LOW);
    digitalWrite(pinoVermelho[i], (vermelho != RGB_ANODO_COMUM) ? HIGH : LOW);
    if (!validos[i]) todasValidas = false;
    if (verde) ++livres;
    linha += "V" + String(i + 1) + (validos[i] ? (ocupadas[i] ? "X " : "L ") : "? ");
  }
  if (!lcdDisponivel) return;
  String titulo = todasValidas ? (livres ? "Livres: " + String(livres) + "/4" : "LOTADO 0/4") : "Falha sensores";
  static String anterior1, anterior2;
  if (titulo != anterior1) { escreverLinhaLCD(0, titulo); anterior1 = titulo; }
  if (linha != anterior2) { escreverLinhaLCD(1, linha); anterior2 = linha; }
}

void campoData(JsonObject fields, const char* nome, time_t valor) {
  if (valor) fields[nome]["stringValue"] = isoUTC(valor);
  else fields[nome]["nullValue"] = nullptr;
}

JsonObject novoDocumento(JsonArray writes, const String& caminho) {
  JsonObject update = writes.add<JsonObject>()["update"].to<JsonObject>();
  update["name"] = String("projects/") + FIREBASE_PROJECT_ID + "/databases/(default)/documents/" + caminho;
  return update["fields"].to<JsonObject>();
}

void definirErro(const String& erro, bool sucesso = false) {
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  firebaseOk = sucesso;
  if (sucesso) ultimoSucessoFirebase = millis();
  snprintf(erroFirebase, sizeof(erroFirebase), "%s", erro.c_str());
  xSemaphoreGive(mutexDados);
  Serial.println(erro);
}

int requisicao(const String& url, const String& corpo, String& resposta, bool autorizada) {
  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOTS);
  client.setHandshakeTimeout(8);
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(7000);
  if (!http.begin(client, url)) return -1;
  http.addHeader("Content-Type", "application/json");
  if (autorizada) http.addHeader("Authorization", "Bearer " + token);
  int codigo = http.POST(corpo);
  resposta = http.getString();
  http.end();
  return codigo;
}

bool autenticar() {
  if (token.length() && millis() - tokenCriado < tokenDuracao) return true;
  if (String(FIREBASE_PASSWORD).startsWith("PREENCHA") || String(FIREBASE_DEVICE_UID).startsWith("PREENCHA")) {
    definirErro("Preencha a senha e o UID do usuario Firebase em Config.h.");
    return false;
  }
  JsonDocument doc;
  doc["email"] = FIREBASE_EMAIL;
  doc["password"] = FIREBASE_PASSWORD;
  doc["returnSecureToken"] = true;
  String corpo, resposta;
  serializeJson(doc, corpo);
  int codigo = requisicao(String("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=") + FIREBASE_API_KEY, corpo, resposta, false);
  doc.clear();
  if (deserializeJson(doc, resposta)) {
    definirErro("Falha TLS/rede no login Firebase: " + String(codigo));
    return false;
  }
  if (codigo != 200) {
    definirErro("Login Firebase: " + String(doc["error"]["message"] | "erro de autenticacao"));
    return false;
  }
  if (String(doc["localId"].as<const char*>()) != FIREBASE_DEVICE_UID) {
    definirErro("UID do login nao corresponde ao Config.h.");
    return false;
  }
  token = doc["idToken"].as<String>();
  uint32_t segundos = doc["expiresIn"].as<String>().toInt();
  if (!token.length() || segundos < 120) { token = ""; return false; }
  tokenCriado = millis();
  tokenDuracao = (segundos - 60) * 1000UL; // Refaz o login antes de expirar.
  return true;
}

void sincronizarFirebase() {
  if (!autenticar()) return;
  Snapshot s = snapshot();
  JsonDocument doc;
  JsonArray writes = doc["writes"].to<JsonArray>();
  if (s.sensoresOk && s.ultimaAtualizacao) {
    for (int i = 0; i < 4; ++i) {
      JsonObject f = novoDocumento(writes, "vagas/vaga" + String(i + 1));
      f["numero"]["integerValue"] = String(i + 1);
      f["ocupada"]["booleanValue"] = s.dados.vagas[i].ocupada;
      campoData(f, "entradaAtual", s.dados.vagas[i].entrada);
    }
  }
  JsonObject meta = novoDocumento(writes, "metadata/status");
  campoData(meta, "ultimaAtualizacao", s.ultimaAtualizacao);
  meta["sensoresOk"]["booleanValue"] = s.sensoresOk && s.ultimaAtualizacao;
  meta["eventosDescartados"]["integerValue"] = String(s.dados.descartados);
  int enviar = min((int)s.dados.quantidade, 10);
  for (int i = 0; i < enviar; ++i) {
    const Evento& e = s.dados.pendentes[i];
    JsonObject f = novoDocumento(writes, "eventos/" + String(e.id));
    f["vaga"]["integerValue"] = String(e.vaga);
    f["tipo"]["stringValue"] = e.entrada ? "ENTRADA" : "SAIDA";
    campoData(f, "dataHora", e.dataHora);
    campoData(f, "entrada", e.inicio);
    campoData(f, "saida", e.entrada ? 0 : e.dataHora);
  }
  String corpo, resposta;
  serializeJson(doc, corpo);
  int codigo = requisicao(String("https://firestore.googleapis.com/v1/projects/") + FIREBASE_PROJECT_ID + "/databases/(default)/documents:commit", corpo, resposta, true);
  if (codigo >= 200 && codigo < 300) {
    xSemaphoreTake(mutexDados, portMAX_DELAY);
    // O produtor so acrescenta ao fim. IDs fixos tornam tentativas idempotentes.
    if (enviar) {
      memmove(&dados.pendentes[0], &dados.pendentes[enviar], (dados.quantidade - enviar) * sizeof(Evento));
      dados.quantidade -= enviar;
      persistir();
    }
    xSemaphoreGive(mutexDados);
    definirErro("Firebase sincronizado", true);
  } else {
    if (codigo == 401) token = "";
    JsonDocument erro;
    deserializeJson(erro, resposta);
    definirErro("Firestore " + String(codigo) + ": " + String(erro["error"]["message"] | "falha de rede/TLS"));
  }
}

void tarefaFirebase(void*) {
  // Requisicoes HTTPS nao bloqueiam o LCD, os LEDs, a UART ou o servidor.
  for (;;) {
    if (WiFi.status() == WL_CONNECTED && agoraUTC()) sincronizarFirebase();
    else definirErro("Aguardando Wi-Fi/horario NTP; controle local ativo.");
    vTaskDelay(pdMS_TO_TICKS(INTERVALO_FIREBASE_MS));
  }
}

void responderStatus() {
  Snapshot s = snapshot();
  if (!s.sensoresOk) {
    server.send(503, "application/json", "{\"error\":{\"message\":\"Sem leitura valida dos quatro sensores. Verifique UART e HC-SR04.\"}}");
    return;
  }
  JsonDocument doc;
  if (s.ultimaAtualizacao) doc["ultimaAtualizacao"] = isoUTC(s.ultimaAtualizacao);
  else doc["ultimaAtualizacao"] = nullptr;
  doc["idadeLeituraMs"] = s.idadeLeituraMs;
  JsonArray vagas = doc["vagas"].to<JsonArray>();
  for (int i = 0; i < 4; ++i) {
    JsonObject v = vagas.add<JsonObject>();
    v["numero"] = i + 1;
    v["ocupada"] = s.dados.vagas[i].ocupada;
    if (s.dados.vagas[i].entrada) v["entradaAtual"] = isoUTC(s.dados.vagas[i].entrada);
    else v["entradaAtual"] = nullptr;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void responderHistorico() {
  Snapshot s = snapshot();
  JsonDocument doc;
  JsonArray lista = doc.to<JsonArray>();
  for (int i = 0; i < s.dados.totalHistorico; ++i) {
    const Evento& e = s.dados.historico[i];
    JsonObject item = lista.add<JsonObject>();
    item["vaga"] = e.vaga;
    item["tipo"] = e.entrada ? "ENTRADA" : "SAIDA";
    if (e.dataHora) item["dataHora"] = isoUTC(e.dataHora); else item["dataHora"] = nullptr;
    if (e.inicio) item["entrada"] = isoUTC(e.inicio); else item["entrada"] = nullptr;
    if (!e.entrada && e.dataHora) item["saida"] = isoUTC(e.dataHora); else item["saida"] = nullptr;
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void responderHealth() {
  JsonDocument doc;
  doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.localIP().toString();
  doc["ntp"] = agoraUTC() != 0;
  doc["lcd"] = lcdDisponivel;
  xSemaphoreTake(mutexDados, portMAX_DELAY);
  doc["sensores"] = sensoresSaudaveis();
  doc["firebase"] = firebaseOk && millis() - ultimoSucessoFirebase < 30000;
  doc["mensagemFirebase"] = erroFirebase;
  doc["eventosPendentes"] = dados.quantidade;
  doc["eventosDescartados"] = dados.descartados;
  xSemaphoreGive(mutexDados);
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  bootId = esp_random();
  mutexDados = xSemaphoreCreateMutex();
  if (!mutexDados) { Serial.println("Sem memoria para mutex."); while (true) delay(1000); }
  nvsDisponivel = preferences.begin("parking-v2", false);
  if (nvsDisponivel && preferences.getBytesLength("estado") == sizeof(dados)) {
    preferences.getBytes("estado", &dados, sizeof(dados));
  }
  if (dados.versao != 2 || dados.quantidade > MAX_EVENTOS || dados.totalHistorico > MAX_EVENTOS) {
    memset(&dados, 0, sizeof(dados));
    dados.versao = 2;
  }
  Serial2.setRxBufferSize(2048);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  linhaSerial.reserve(1024);
  for (int i = 0; i < 4; ++i) {
    pinMode(pinoVerde[i], OUTPUT);
    pinMode(pinoVermelho[i], OUTPUT);
    digitalWrite(pinoVerde[i], RGB_ANODO_COMUM ? HIGH : LOW);
    digitalWrite(pinoVermelho[i], RGB_ANODO_COMUM ? HIGH : LOW);
  }
  Wire.begin(LCD_SDA, LCD_SCL);
  lcdDisponivel = lcd.begin(LCD_COLUNAS, LCD_LINHAS) == 0;
  if (lcdDisponivel) { lcd.backlight(); escreverLinhaLCD(0, "Estacionamento"); escreverLinhaLCD(1, "Aguardando UART"); }
  else Serial.println("LCD nao encontrado. Verifique I2C e alimentacao.");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", SITE_INDEX); });
  server.on("/index.html", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", SITE_INDEX); });
  server.on("/style.css", HTTP_GET, []() { server.send_P(200, "text/css; charset=utf-8", SITE_STYLE); });
  server.on("/data.js", HTTP_GET, []() { server.send_P(200, "application/javascript; charset=utf-8", SITE_DATA); });
  server.on("/script.js", HTTP_GET, []() { server.send_P(200, "application/javascript; charset=utf-8", SITE_SCRIPT); });
  server.on("/config.js", HTTP_GET, []() { server.send(200, "application/javascript", "window.PARKING_CONFIG={mode:'esp32',esp32Url:''};"); });
  server.on("/api/vagas", HTTP_GET, responderStatus);
  server.on("/api/status", HTTP_GET, responderStatus);
  server.on("/api/historico", HTTP_GET, responderHistorico);
  server.on("/api/health", HTTP_GET, responderHealth);
  server.onNotFound([]() { server.send(404, "text/plain", "Nao encontrado"); });
  server.begin();
  if (xTaskCreatePinnedToCore(tarefaFirebase, "firebase", 16384, nullptr, 1, nullptr, 0) != pdPASS) {
    definirErro("Nao foi possivel iniciar tarefa Firebase.");
  }
  Serial.println("Controle local ativo. Aguarde o IP no Monitor Serial (115200).");
}

void loop() {
  lerUART();
  atualizarSaidas();
  server.handleClient();
  static uint32_t ultimaTentativa = 0;
  static bool conectadoAntes = false;
  bool conectado = WiFi.status() == WL_CONNECTED;
  if (conectado && !conectadoAntes) {
    Serial.print("Painel local: http://"); Serial.println(WiFi.localIP());
  }
  conectadoAntes = conectado;
  if (!conectado && millis() - ultimaTentativa > 15000) {
    ultimaTentativa = millis();
    WiFi.reconnect();
  }
  delay(2);
}
