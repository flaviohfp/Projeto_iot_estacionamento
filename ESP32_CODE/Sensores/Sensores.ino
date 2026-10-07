// ESP32 #1: quatro HC-SR04. TX2 (GPIO17) -> RX2 (GPIO16) da ESP32 #2.
// Una os GNDs. O ECHO de cada HC-SR04 precisa de divisor de 5 V para 3,3 V.
#include <Arduino.h>

const int trigPins[4] = {4, 5, 12, 27};
const int echoPins[4] = {2, 18, 14, 26};
constexpr float DISTANCIA_OCUPADA_CM = 12.0f;
constexpr float DISTANCIA_LIVRE_CM = 16.0f;
constexpr uint8_t CONFIRMACOES = 3;
constexpr uint32_t TIMEOUT_ECHO_US = 25000;

bool ocupada[4] = {};
bool conhecida[4] = {};
bool candidata[4] = {};
uint8_t consecutivas[4] = {};
uint8_t falhas[4] = {};
float distancia[4] = {};
uint32_t sequencia = 0;

void medir(int i) {
  digitalWrite(trigPins[i], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[i], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[i], LOW);
  unsigned long pulso = pulseIn(echoPins[i], HIGH, TIMEOUT_ECHO_US);
  distancia[i] = pulso * 0.0343f / 2.0f;
  if (!pulso || distancia[i] < 2.0f || distancia[i] > 400.0f) {
    if (falhas[i] < CONFIRMACOES) ++falhas[i];
    consecutivas[i] = 0;
    // Uma falha isolada conserva o estado; tres falhas o tornam desconhecido.
    if (falhas[i] >= CONFIRMACOES) conhecida[i] = false;
    return;
  }
  falhas[i] = 0;
  bool nova = conhecida[i] ? ocupada[i] : distancia[i] <= DISTANCIA_OCUPADA_CM;
  if (distancia[i] <= DISTANCIA_OCUPADA_CM) nova = true;
  if (distancia[i] >= DISTANCIA_LIVRE_CM) nova = false;
  if (!consecutivas[i] || nova != candidata[i]) {
    candidata[i] = nova;
    consecutivas[i] = 1;
  } else if (consecutivas[i] < CONFIRMACOES) {
    ++consecutivas[i];
  }
  if (consecutivas[i] >= CONFIRMACOES) {
    ocupada[i] = nova;
    conhecida[i] = true;
  }
}

void enviar() {
  String mensagem = "{\"v\":1,\"seq\":" + String(++sequencia) + ",\"vagas\":[";
  for (int i = 0; i < 4; ++i) {
    if (i) mensagem += ',';
    mensagem += "{\"numero\":" + String(i + 1);
    mensagem += ",\"ocupada\":" + String(ocupada[i] ? "true" : "false");
    mensagem += ",\"valida\":" + String(conhecida[i] ? "true" : "false");
    mensagem += ",\"distanciaCm\":";
    mensagem += falhas[i] ? String("null") : String(distancia[i], 1);
    mensagem += '}';
  }
  mensagem += "]}";
  Serial2.println(mensagem);
  Serial.println(mensagem);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  for (int i = 0; i < 4; ++i) {
    pinMode(trigPins[i], OUTPUT);
    digitalWrite(trigPins[i], LOW);
    pinMode(echoPins[i], INPUT);
  }
  Serial.println("Sensores iniciados. UART TX=17; ajuste os limites de distancia.");
}

void loop() {
  for (int i = 0; i < 4; ++i) {
    medir(i);
    // Medicao sequencial evita que um sensor receba o eco do vizinho.
    delay(65);
  }
  enviar();
}
