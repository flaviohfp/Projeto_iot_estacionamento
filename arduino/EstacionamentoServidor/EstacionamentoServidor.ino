#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include "config.h"
#include "web_assets.h"
#if USAR_OLED || USAR_RTC_DS3231
#include <Wire.h>
#endif
#if USAR_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif
#if USAR_RTC_DS3231
#include <RTClib.h>
#include <sys/time.h>
RTC_DS3231 rtc;
#endif

WebServer server(80);
bool occupied[4] = {}, candidate[4] = {}, ready[4] = {};
unsigned long candidateSince[4] = {}, entered[4] = {};
String entryTime[4], updated[4];
String events[40];
int eventHead = 0, eventCount = 0;
bool oledReady = false;
bool rtcReady = false;
bool storageReady = false;
bool storageHealthy = false;
Preferences historyStore;
unsigned long lastWifiAttempt = 0;

void restoreHistory() {
  storageReady = historyStore.begin("parking", false);
  storageHealthy = storageReady;
  if (!storageReady) return;
  String log = historyStore.getString("events", "");
  int start = 0;
  while (start < (int)log.length()) {
    int end = log.indexOf('\n', start);
    if (end < 0) break;
    events[eventHead] = log.substring(start, end);
    eventHead = (eventHead + 1) % 40;
    if (eventCount < 40) eventCount++;
    start = end + 1;
  }
}
void saveHistory() {
  if (!storageReady) return;
  String log;
  for (int i = eventCount; i > 0; i--) log += events[(eventHead - i + 40) % 40] + "\n";
  storageHealthy = historyStore.putString("events", log) == log.length();
  if (!storageHealthy) Serial.println("Falha ao persistir historico");
}

String timestamp() {
  time_t now = time(nullptr);
#if USAR_RTC_DS3231
  if (rtcReady && !rtc.lostPower()) now = rtc.now().unixtime();
#endif
  if (now < 1700000000) return "";
  struct tm utc;
  gmtime_r(&now, &utc);
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc);
  return String(buffer);
}
String dateJson(const String& value) { return value.length() ? "\"" + value + "\"" : "null"; }
bool sensorsReady() {
  for (int i = 0; i < 4; i++) if (!ready[i]) return false;
  return true;
}
void updateOutputs() {
  int free = 0;
  for (int i = 0; i < 4; i++) {
    if (ready[i] && !occupied[i]) free++;
#if USAR_LEDS
    digitalWrite(LED_RED[i], (ready[i] && occupied[i]) != LED_ANODO_COMUM ? HIGH : LOW);
    digitalWrite(LED_GREEN[i], (ready[i] && !occupied[i]) != LED_ANODO_COMUM ? HIGH : LOW);
#endif
  }
#if USAR_OLED
  if (!oledReady) return;
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); display.println("Estacionamento IoT");
  if (!sensorsReady()) display.println("Lendo sensores...");
  else {
    display.print("Livres: "); display.println(free);
    display.print("Vagas: ");
    for (int i = 0; i < 4; i++) if (!occupied[i]) { display.print(i + 1); display.print(" "); }
  }
  display.display();
#endif
}
void readSensors() {
  unsigned long now = millis();
  bool changed = false;
  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(SENSOR_PINS[i]) == SENSOR_DETECTADO;
    if (reading != candidate[i]) { candidate[i] = reading; candidateSince[i] = now; }
    if (now - candidateSince[i] < DEBOUNCE_MS) continue;
    if (ready[i] && occupied[i] == reading) continue;
    bool initial = !ready[i];
    ready[i] = true; occupied[i] = reading; updated[i] = timestamp(); changed = true;
    String previousEntry = entryTime[i];
    unsigned long seconds = (now - entered[i]) / 1000;
    if (reading) { entered[i] = now; entryTime[i] = updated[i]; }
    else entryTime[i] = "";
    // A primeira leitura é uma fotografia; não invente uma entrada anterior.
    if (initial) continue;
    char duration[24];
    snprintf(duration, sizeof(duration), "%02lu:%02lu:%02lu", seconds / 3600, seconds / 60 % 60, seconds % 60);
    events[eventHead] = "{\"vaga\":" + String(i + 1) + ",\"tipo\":\"" + (reading ? "ENTRADA" : "SAIDA") +
      "\",\"dataHora\":" + dateJson(updated[i]) + ",\"entrada\":" + dateJson(reading ? entryTime[i] : previousEntry) +
      ",\"saida\":" + (reading ? String("null") : dateJson(updated[i])) +
      ",\"duracaoTexto\":" + (reading ? String("null") : "\"" + String(duration) + "\"") + "}";
    eventHead = (eventHead + 1) % 40;
    if (eventCount < 40) eventCount++;
    saveHistory();
  }
  if (changed) updateOutputs();
}
String statusJson() {
  int count = 0;
  String vagas = "[";
  for (int i = 0; i < 4; i++) {
    if (occupied[i]) count++;
    if (i) vagas += ",";
    vagas += "{\"numero\":" + String(i + 1) + ",\"ocupada\":" + (occupied[i] ? "true" : "false") +
      ",\"entradaAtual\":" + dateJson(entryTime[i]) + ",\"ultimaAtualizacao\":" + dateJson(updated[i]) + "}";
  }
  return "{\"total\":4,\"livres\":" + String(4 - count) + ",\"ocupadas\":" + String(count) +
    ",\"taxaOcupacao\":" + String(count * 25) + ",\"ultimaAtualizacao\":" + dateJson(timestamp()) + ",\"vagas\":" + vagas + "]}";
}
void json(const String& body, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}
void setup() {
  Serial.begin(115200);
  restoreHistory();
  for (int i = 0; i < 4; i++) {
    pinMode(SENSOR_PINS[i], INPUT_PULLUP);
    candidate[i] = digitalRead(SENSOR_PINS[i]) == SENSOR_DETECTADO;
    candidateSince[i] = millis();
#if USAR_LEDS
    pinMode(LED_RED[i], OUTPUT); pinMode(LED_GREEN[i], OUTPUT);
#endif
  }
#if USAR_OLED || USAR_RTC_DS3231
  Wire.begin(21, 22);
#endif
#if USAR_OLED
  oledReady = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledReady) Serial.println("OLED nao encontrado");
#endif
#if USAR_RTC_DS3231
  rtcReady = rtc.begin();
  if (rtcReady && !rtc.lostPower()) {
    timeval tv = {}; tv.tv_sec = rtc.now().unixtime(); settimeofday(&tv, nullptr);
  } else Serial.println("RTC ausente ou sem horario valido; ajuste em UTC.");
#endif
  updateOutputs();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (strlen(WIFI_SSID)) WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Abra http://"); Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, [] { server.sendHeader("Content-Encoding", "gzip"); server.send_P(200, "text/html; charset=utf-8", (const char*)ASSET_INDEX, sizeof(ASSET_INDEX)); });
  server.on("/style.css", HTTP_GET, [] { server.sendHeader("Content-Encoding", "gzip"); server.send_P(200, "text/css", (const char*)ASSET_STYLE, sizeof(ASSET_STYLE)); });
  server.on("/script.js", HTTP_GET, [] { server.sendHeader("Content-Encoding", "gzip"); server.send_P(200, "application/javascript", (const char*)ASSET_SCRIPT, sizeof(ASSET_SCRIPT)); });
  server.on("/api/health", HTTP_GET, [] {
    json("{\"success\":true,\"hardware\":true,\"simulation\":false,\"database\":\"" + String(storageHealthy ? "ESP32 (flash persistente)" : "ESP32 (falha de armazenamento)") + "\",\"realtime\":\"polling\",\"rtcReady\":" + (rtcReady ? "true" : "false") + ",\"oledReady\":" + (oledReady ? "true" : "false") + ",\"clockSynced\":" + String(timestamp().length() ? "true" : "false") + ",\"sensorsReady\":" + (sensorsReady() ? "true" : "false") + "}");
  });
  server.on("/api/rtc", HTTP_POST, [] {
#if USAR_RTC_DS3231
    if (!rtcReady) { json("{\"error\":\"RTC DS3231 nao encontrado\"}", 503); return; }
    String value = server.arg("plain");
    if (value.length() != 10) { json("{\"error\":\"Envie epoch UTC em segundos\"}", 400); return; }
    for (unsigned int i = 0; i < value.length(); i++) if (!isDigit(value[i])) { json("{\"error\":\"Horario invalido\"}", 400); return; }
    unsigned long epoch = strtoul(value.c_str(), nullptr, 10);
    if (epoch < 1700000000UL || epoch > 4102444799UL) { json("{\"error\":\"Horario fora do intervalo\"}", 400); return; }
    rtc.adjust(DateTime((uint32_t)epoch));
    timeval tv = {}; tv.tv_sec = epoch; settimeofday(&tv, nullptr);
    json("{\"success\":true}");
#else
    json("{\"error\":\"RTC desabilitado\"}", 503);
#endif
  });
  server.on("/api/vagas", HTTP_GET, [] {
    if (!sensorsReady()) { json("{\"error\":\"Aguardando estabilizacao dos sensores\"}", 503); return; }
    json(statusJson());
  });
  server.on("/api/historico", HTTP_GET, [] {
    String body = "[";
    for (int i = 0; i < eventCount; i++) { if (i) body += ","; body += events[(eventHead - 1 - i + 40) % 40]; }
    json(body + "]");
  });
  server.on("/api/status/display", HTTP_GET, [] {
    if (!sensorsReady()) { json("{\"error\":\"Sensores iniciando\"}", 503); return; }
    String list = ""; int free = 0;
    for (int i = 0; i < 4; i++) if (!occupied[i]) { if (free++) list += ","; list += String(i + 1); }
    json("{\"livres\":" + String(free) + ",\"vagasLivres\":[" + list + "]}");
  });
  server.onNotFound([] { json("{\"error\":\"Modo fisico: leituras controladas pelos sensores\"}", server.method() == HTTP_GET ? 404 : 405); });
  server.begin();
}
void loop() {
  readSensors();
  server.handleClient();
  if (strlen(WIFI_SSID) && WiFi.status() != WL_CONNECTED && millis() - lastWifiAttempt > 30000) {
    lastWifiAttempt = millis(); WiFi.reconnect();
  }
  delay(2);
}
