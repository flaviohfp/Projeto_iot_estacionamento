#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>

const char* ssid = "AutomacaoF17";
const char* password = "Automacao-F17";

const char* FIREBASE_PROJECT_ID = "iotestacionamento-e2b70";

WebServer server(80);

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensores[4] = {32, 33, 25, 26};
const int leds[4] = {13, 14, 27, 12};

bool vagas[4] = {false, false, false, false};
bool estadoAnterior[4] = {false, false, false, false};

String entradaAtual[4];

unsigned long ultimaLeitura = 0;
unsigned long ultimoEnvioFirebase = 0;

const unsigned long intervaloLeitura = 300;
const unsigned long intervaloFirebase = 1500;

String dataHoraAtual() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "";
  }

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);

  return String(buffer);
}

String jsonEscape(String texto) {
  texto.replace("\\", "\\\\");
  texto.replace("\"", "\\\"");
  return texto;
}

void atualizarLCD() {
  int livres = 0;

  for (int i = 0; i < 4; i++) {
    if (!vagas[i]) {
      livres++;
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Livres: ");
  lcd.print(livres);
  lcd.print("/4");

  lcd.setCursor(0, 1);

  for (int i = 0; i < 4; i++) {
    lcd.print("V");
    lcd.print(i + 1);
    lcd.print(vagas[i] ? "X " : "L ");
  }
}

String jsonStatus() {
  int livres = 0;
  int ocupadas = 0;

  for (int i = 0; i < 4; i++) {
    if (vagas[i]) {
      ocupadas++;
    } else {
      livres++;
    }
  }

  int taxa = (ocupadas * 100) / 4;

  String json = "{";
  json += "\"ultimaAtualizacao\":\"" + jsonEscape(dataHoraAtual()) + "\",";
  json += "\"livres\":" + String(livres) + ",";
  json += "\"ocupadas\":" + String(ocupadas) + ",";
  json += "\"taxaOcupacao\":" + String(taxa) + ",";
  json += "\"vagas\":[";

  for (int i = 0; i < 4; i++) {
    if (i > 0) json += ",";

    json += "{";
    json += "\"numero\":" + String(i + 1) + ",";
    json += "\"ocupada\":" + String(vagas[i] ? "true" : "false") + ",";
    json += "\"entradaAtual\":";

    if (entradaAtual[i].length() > 0) {
      json += "\"" + jsonEscape(entradaAtual[i]) + "\"";
    } else {
      json += "null";
    }

    json += "}";
  }

  json += "]}";

  return json;
}

void handleStatus() {
  server.send(200, "application/json", jsonStatus());
}

void handleHealth() {
  String json = "{";
  json += "\"wifi\":";
  json += WiFi.status() == WL_CONNECTED ? "true" : "false";
  json += ",";
  json += "\"firebase\":true";
  json += "}";

  server.send(200, "application/json", json);
}

void enviarFirestore(String caminho, String jsonData, String metodo = "PATCH") {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String url = "https://firestore.googleapis.com/v1/projects/";
  url += FIREBASE_PROJECT_ID;
  url += "/databases/(default)/documents/";
  url += caminho;

  if (!http.begin(client, url)) {
    return;
  }

  http.addHeader("Content-Type", "application/json");

  int codigo = -1;

  if (metodo == "PATCH") {
    codigo = http.PATCH(jsonData);
  } else {
    codigo = http.POST(jsonData);
  }

  http.end();
}

void enviarEvento(int numero, String tipo, String entrada, String saida) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  String agora = dataHoraAtual();

  String json = "{";
  json += "\"fields\":{";

  json += "\"vaga\":{\"integerValue\":\"" + String(numero) + "\"},";
  json += "\"tipo\":{\"stringValue\":\"" + jsonEscape(tipo) + "\"},";
  json += "\"dataHora\":{\"stringValue\":\"" + jsonEscape(agora) + "\"},";

  if (entrada.length() > 0) {
    json += "\"entrada\":{\"stringValue\":\"" + jsonEscape(entrada) + "\"},";
  } else {
    json += "\"entrada\":{\"nullValue\":null},";
  }

  if (saida.length() > 0) {
    json += "\"saida\":{\"stringValue\":\"" + jsonEscape(saida) + "\"}";
  } else {
    json += "\"saida\":{\"nullValue\":null}";
  }

  json += "}}";

  enviarFirestore("eventos", json, "POST");
}

void sincronizarFirebase(bool forcar) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!forcar && millis() - ultimoEnvioFirebase < intervaloFirebase) {
    return;
  }

  ultimoEnvioFirebase = millis();

  for (int i = 0; i < 4; i++) {
    String json = "{";
    json += "\"fields\":{";

    json += "\"numero\":{\"integerValue\":\"" + String(i + 1) + "\"},";
    json += "\"ocupada\":{\"booleanValue\":" + String(vagas[i] ? "true" : "false") + "},";

    if (entradaAtual[i].length() > 0) {
      json += "\"entradaAtual\":{\"stringValue\":\"" + jsonEscape(entradaAtual[i]) + "\"}";
    } else {
      json += "\"entradaAtual\":{\"nullValue\":null}";
    }

    json += "}}";

    enviarFirestore("vagas/vaga" + String(i + 1), json, "PATCH");
    estadoAnterior[i] = vagas[i];
  }

  int livres = 0;
  int ocupadas = 0;

  for (int i = 0; i < 4; i++) {
    if (vagas[i]) ocupadas++;
    else livres++;
  }

  String agora = dataHoraAtual();

  String statusJson = "{";
  statusJson += "\"fields\":{";
  statusJson += "\"ultimaAtualizacao\":{\"stringValue\":\"" + jsonEscape(agora) + "\"},";
  statusJson += "\"livres\":{\"integerValue\":\"" + String(livres) + "\"},";
  statusJson += "\"ocupadas\":{\"integerValue\":\"" + String(ocupadas) + "\"}";
  statusJson += "}}";

  enviarFirestore("metadata/status", statusJson, "PATCH");
}

void atualizarVagas() {
  bool houveMudanca = false;

  for (int i = 0; i < 4; i++) {
    // Ajuste HIGH/LOW se o seu sensor IR funcionar invertido.
    bool ocupada = digitalRead(sensores[i]) == LOW;

    vagas[i] = ocupada;

    digitalWrite(leds[i], ocupada ? HIGH : LOW);

    if (vagas[i] != estadoAnterior[i]) {
      houveMudanca = true;

      String agora = dataHoraAtual();

      if (vagas[i]) {
        entradaAtual[i] = agora;

        enviarEvento(i + 1, "ENTRADA", entradaAtual[i], "");
      } else {
        enviarEvento(i + 1, "SAIDA", entradaAtual[i], agora);

        entradaAtual[i] = "";
      }
    }
  }

  atualizarLCD();

  if (houveMudanca) {
    sincronizarFirebase(true);
  } else {
    sincronizarFirebase(false);
  }
}

String contentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";

  return "text/plain";
}

bool servirArquivo(String caminho) {
  if (caminho == "/") {
    caminho = "/index.html";
  }

  if (!LittleFS.exists(caminho)) {
    return false;
  }

  File arquivo = LittleFS.open(caminho, "r");

  server.streamFile(arquivo, contentType(caminho));

  arquivo.close();

  return true;
}

void handleNotFound() {
  if (!servirArquivo(server.uri())) {
    server.send(404, "text/plain", "Arquivo nao encontrado");
  }
}

void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando ao Wi-Fi");

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Nao foi possivel conectar ao Wi-Fi.");
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(sensores[i], INPUT);
    pinMode(leds[i], OUTPUT);

    digitalWrite(leds[i], LOW);
  }

  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Estacionamento");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao montar LittleFS.");
    lcd.clear();
    lcd.print("Erro LittleFS");
  }

  conectarWiFi();

  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/health", HTTP_GET, handleHealth);
  server.onNotFound(handleNotFound);

  server.begin();

  delay(1000);

  for (int i = 0; i < 4; i++) {
    bool ocupada = digitalRead(sensores[i]) == LOW;

    vagas[i] = ocupada;
    estadoAnterior[i] = ocupada;

    digitalWrite(leds[i], ocupada ? HIGH : LOW);

    if (ocupada) {
      entradaAtual[i] = dataHoraAtual();
    }
  }

  atualizarLCD();

  ultimoEnvioFirebase = 0;
  sincronizarFirebase(true);

  Serial.println("Servidor iniciado.");
}

void loop() {
  server.handleClient();

  if (millis() - ultimaLeitura >= intervaloLeitura) {
    ultimaLeitura = millis();
    atualizarVagas();
  }
}
