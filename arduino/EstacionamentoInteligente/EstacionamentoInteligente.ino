#include <WiFi.h>
#include <HTTPClient.h>

// Ative somente se tiver os modulos e bibliotecas instalados.
#define USAR_RTC_DS3231 0
#define USAR_OLED 0

#if USAR_RTC_DS3231
#include <Wire.h>
#include <RTClib.h>
RTC_DS3231 rtc;
#endif

#if USAR_OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif

const char* WIFI_SSID = "NOME_DA_REDE";
const char* WIFI_PASSWORD = "SENHA_DA_REDE";

// Use o IP do computador que esta rodando npm start.
// Exemplo: http://192.168.0.100:3000/api/vagas/status/lote
const char* SERVER_URL = "http://192.168.0.100:3000/api/vagas/status/lote";

// Preencha somente se ENABLE_API_KEY=true no servidor.
const char* API_KEY = "";

const int TOTAL_VAGAS = 4;
const int sensorPins[TOTAL_VAGAS] = { 13, 12, 14, 27 };

// Em muitos sensores IR digitais: LOW = objeto detectado.
// Se no seu modulo for o contrario, troque para HIGH.
const int SENSOR_DETECTADO = LOW;

const unsigned long TEMPO_DEBOUNCE_MS = 800;
const unsigned long INTERVALO_REENVIO_MS = 30000;
const unsigned long INTERVALO_WIFI_MS = 5000;

bool estadosConfirmados[TOTAL_VAGAS] = { false, false, false, false };
bool estadosCandidatos[TOTAL_VAGAS] = { false, false, false, false };
bool estadosEnviados[TOTAL_VAGAS] = { false, false, false, false };
unsigned long inicioCandidato[TOTAL_VAGAS] = { 0, 0, 0, 0 };
unsigned long ultimoEnvio = 0;
unsigned long ultimaTentativaWiFi = 0;
bool primeiroEnvioFeito = false;

bool lerSensorOcupado(int index) {
  return digitalRead(sensorPins[index]) == SENSOR_DETECTADO;
}

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();
  if (agora - ultimaTentativaWiFi < INTERVALO_WIFI_MS) {
    return;
  }

  ultimaTentativaWiFi = agora;
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

String timestampISO() {
#if USAR_RTC_DS3231
  DateTime now = rtc.now();
  char buffer[20];
  snprintf(
    buffer,
    sizeof(buffer),
    "%04d-%02d-%02dT%02d:%02d:%02d",
    now.year(),
    now.month(),
    now.day(),
    now.hour(),
    now.minute(),
    now.second()
  );
  return String(buffer);
#else
  return "";
#endif
}

String montarJsonLote() {
  String json = "{\"vagas\":[";

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    if (i > 0) {
      json += ",";
    }

    json += "{\"vaga\":";
    json += String(i + 1);
    json += ",\"ocupada\":";
    json += estadosConfirmados[i] ? "true" : "false";
    json += "}";
  }

  String timestamp = timestampISO();
  json += "]";
  if (timestamp.length() > 0) {
    json += ",\"timestamp\":\"";
    json += timestamp;
    json += "\"";
  }
  json += "}";

  return json;
}

void atualizarDisplayLocal() {
#if USAR_OLED
  int livres = 0;
  String vagasLivres = "";

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    if (!estadosConfirmados[i]) {
      livres++;
      if (vagasLivres.length() > 0) {
        vagasLivres += " ";
      }
      vagasLivres += String(i + 1);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Estacionamento IoT");
  display.println();
  display.print("Livres: ");
  display.println(livres);
  display.print("Vagas: ");
  display.println(vagasLivres.length() ? vagasLivres : "-");
  display.display();
#endif
}

bool enviarLote() {
  conectarWiFi();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi ainda desconectado. Envio adiado.");
    return false;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  if (String(API_KEY).length() > 0) {
    http.addHeader("X-API-Key", API_KEY);
  }

  String json = montarJsonLote();
  int httpCode = http.POST(json);

  Serial.print("POST lote | HTTP ");
  Serial.println(httpCode);
  Serial.println(json);

  if (httpCode > 0) {
    Serial.println(http.getString());
  }

  http.end();

  if (httpCode >= 200 && httpCode < 300) {
    for (int i = 0; i < TOTAL_VAGAS; i++) {
      estadosEnviados[i] = estadosConfirmados[i];
    }
    ultimoEnvio = millis();
    primeiroEnvioFeito = true;
    return true;
  }

  return false;
}

bool atualizarLeituras() {
  bool houveMudancaConfirmada = false;
  unsigned long agora = millis();

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    bool leituraAtual = lerSensorOcupado(i);

    if (leituraAtual != estadosCandidatos[i]) {
      estadosCandidatos[i] = leituraAtual;
      inicioCandidato[i] = agora;
    }

    bool leituraEstavel = (agora - inicioCandidato[i]) >= TEMPO_DEBOUNCE_MS;
    if (leituraEstavel && estadosConfirmados[i] != estadosCandidatos[i]) {
      estadosConfirmados[i] = estadosCandidatos[i];
      houveMudancaConfirmada = true;

      Serial.print("Vaga ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(estadosConfirmados[i] ? "OCUPADA" : "LIVRE");
    }
  }

  return houveMudancaConfirmada;
}

void inicializarSensores() {
  for (int i = 0; i < TOTAL_VAGAS; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
    bool ocupada = lerSensorOcupado(i);
    estadosConfirmados[i] = ocupada;
    estadosCandidatos[i] = ocupada;
    estadosEnviados[i] = ocupada;
    inicioCandidato[i] = millis();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  inicializarSensores();

#if USAR_RTC_DS3231 || USAR_OLED
  Wire.begin();
#endif

#if USAR_RTC_DS3231
  if (!rtc.begin()) {
    Serial.println("RTC DS3231 nao encontrado. Verifique SDA/SCL e alimentacao.");
  }
#endif

#if USAR_OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nao encontrado em 0x3C.");
  }
#endif

  conectarWiFi();
  atualizarDisplayLocal();
}

void loop() {
  conectarWiFi();

  bool mudou = atualizarLeituras();
  bool precisaReenviar = !primeiroEnvioFeito || (millis() - ultimoEnvio >= INTERVALO_REENVIO_MS);

  if (mudou || precisaReenviar) {
    atualizarDisplayLocal();
    enviarLote();
  }

  delay(50);
}
