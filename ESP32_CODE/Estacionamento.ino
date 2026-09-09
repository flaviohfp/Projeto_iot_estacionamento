#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "AutomacaoF17";
const char* password = "Automacao-F17";

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int sensoresIR[4] = {32, 33, 25, 26};
const int leds[4] = {13, 14, 27, 12};

bool vagaOcupada[4] = {false, false, false, false};

unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 500;

String contentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  if (filename.endsWith(".json")) return "application/json";
  if (filename.endsWith(".png")) return "image/png";
  if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) return "image/jpeg";
  if (filename.endsWith(".svg")) return "image/svg+xml";
  if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool servirArquivo(String path) {
  if (path.endsWith("/")) path += "index.html";

  if (!LittleFS.exists(path)) return false;

  File arquivo = LittleFS.open(path, "r");
  if (!arquivo) return false;

  server.streamFile(arquivo, contentType(path));
  arquivo.close();
  return true;
}

void paginaInicial() {
  if (!servirArquivo("/index.html")) {
    server.send(500, "text/plain", "Erro: index.html não encontrado.");
  }
}

void arquivoCSS() {
  if (!servirArquivo("/style.css")) {
    server.send(404, "text/plain", "style.css não encontrado.");
  }
}

void arquivoJS() {
  if (!servirArquivo("/script.js")) {
    server.send(404, "text/plain", "script.js não encontrado.");
  }
}

void atualizarVagas() {
  for (int i = 0; i < 4; i++) {
    int leitura = digitalRead(sensoresIR[i]);

    // LOW = ocupada / HIGH = livre
    if (leitura == LOW) {
      vagaOcupada[i] = true;
      digitalWrite(leds[i], LOW);
    } else {
      vagaOcupada[i] = false;
      digitalWrite(leds[i], HIGH);
    }
  }
}

void atualizarLCD() {
  int livres = 0;
  int ocupadas = 0;

  for (int i = 0; i < 4; i++) {
    if (vagaOcupada[i]) ocupadas++;
    else livres++;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Livres: ");
  lcd.print(livres);

  lcd.setCursor(0, 1);
  lcd.print("Ocupadas: ");
  lcd.print(ocupadas);
}

void enviarStatus() {
  int livres = 0;
  int ocupadas = 0;

  for (int i = 0; i < 4; i++) {
    if (vagaOcupada[i]) ocupadas++;
    else livres++;
  }

  int taxaOcupacao = (ocupadas * 100) / 4;

  String json = "{";
  json += "\"vaga1\":"; json += vagaOcupada[0] ? "true" : "false";
  json += ",\"vaga2\":"; json += vagaOcupada[1] ? "true" : "false";
  json += ",\"vaga3\":"; json += vagaOcupada[2] ? "true" : "false";
  json += ",\"vaga4\":"; json += vagaOcupada[3] ? "true" : "false";
  json += ",\"livres\":"; json += livres;
  json += ",\"ocupadas\":"; json += ocupadas;
  json += ",\"taxaOcupacao\":"; json += taxaOcupacao;
  json += "}";

  server.send(200, "application/json", json);
}

void enviarHealth() {
  String json = "{";
  json += "\"hardware\":\"ESP32\"";
  json += ",\"sensorsReady\":true";
  json += ",\"lcdReady\":true";
  json += ",\"rtcReady\":false";
  json += ",\"oledReady\":false";
  json += ",\"database\":\"none\"";
  json += ",\"realtime\":\"polling\"";
  json += "}";

  server.send(200, "application/json", json);
}

void paginaNaoEncontrada() {
  String mensagem = "404 - Pagina nao encontrada\n\n";
  mensagem += "URI: ";
  mensagem += server.uri();
  server.send(404, "text/plain", mensagem);
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
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Nao foi possivel conectar ao Wi-Fi.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" ESTACIONAMENTO INTELIGENTE");
  Serial.println("================================");

  for (int i = 0; i < 4; i++) {
    pinMode(sensoresIR[i], INPUT);
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
  delay(1500);

  if (!LittleFS.begin(true)) {
    Serial.println("Erro ao iniciar LittleFS.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro LittleFS");
    return;
  }

  Serial.println("LittleFS iniciado.");

  conectarWiFi();

  atualizarVagas();
  atualizarLCD();

  server.on("/", HTTP_GET, paginaInicial);
  server.on("/style.css", HTTP_GET, arquivoCSS);
  server.on("/script.js", HTTP_GET, arquivoJS);
  server.on("/api/status", HTTP_GET, enviarStatus);
  server.on("/api/health", HTTP_GET, enviarHealth);
  server.onNotFound(paginaNaoEncontrada);

  server.begin();

  Serial.println("Servidor HTTP iniciado.");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Acesse: http://");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP:");

    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());

    delay(3000);
    atualizarLCD();
  }
}

void loop() {
  server.handleClient();

  unsigned long agora = millis();

  if (agora - ultimaLeitura >= intervaloLeitura) {
    ultimaLeitura = agora;
    atualizarVagas();
    atualizarLCD();
  }
}
