# Integracao ESP32

Este guia mostra como a maquete fisica podera enviar dados para o sistema web `Estacionamento Inteligente`.

## Visao geral

Sensores IR das 4 vagas -> ESP32 -> Wi-Fi -> API Node.js -> Firebase Firestore -> Dashboard

Componentes previstos:

- 4 sensores IR no piso, um por vaga.
- ESP32 lendo os sensores.
- RTC DS3231 ligado ao ESP32 para data/hora confiavel.
- Display OLED I2C ligado ao ESP32 para mostrar vagas livres.

## Sketch pronto

O codigo para gravar no ESP32 esta em:

```text
arduino/EstacionamentoInteligente/EstacionamentoInteligente.ino
```

Antes de enviar para a placa, altere:

- `WIFI_SSID` e `WIFI_PASSWORD`;
- `SERVER_URL`, usando o IP do computador que esta rodando `npm start`;
- `sensorPins`, conforme a ligacao real dos sensores na maquete;
- `SENSOR_DETECTADO`, caso o modulo IR use `HIGH` em vez de `LOW` para objeto detectado.

## Endpoint principal recomendado

O ESP32 deve enviar o estado das vagas para:

```http
POST http://IP_DO_SERVIDOR:3000/api/vagas/status/lote
Content-Type: application/json
```

Corpo:

```json
{
  "vagas": [
    { "vaga": 1, "ocupada": true },
    { "vaga": 2, "ocupada": false },
    { "vaga": 3, "ocupada": false },
    { "vaga": 4, "ocupada": true }
  ],
  "timestamp": "2026-08-11T14:32:00"
}
```

O campo `timestamp` pode vir do RTC DS3231. Se nao for enviado, o servidor usa a hora do computador.

## Endpoint individual

Tambem existe uma rota individual, util para testes ou simulacao:

```http
POST http://IP_DO_SERVIDOR:3000/api/vagas/status
Content-Type: application/json
```

Corpo:

```json
{
  "vaga": 1,
  "ocupada": true,
  "timestamp": "2026-08-11T14:32:00"
}
```

O campo `timestamp` pode vir do RTC DS3231. Se nao for enviado, o servidor usa a hora do computador.

## Endpoint para o OLED

O ESP32 pode consultar:

```http
GET http://IP_DO_SERVIDOR:3000/api/status/display
```

Resposta:

```json
{
  "livres": 2,
  "vagasLivres": [1, 3]
}
```

No OLED, uma exibicao simples seria:

```text
Livres: 2
Vagas: 1 e 3
```

## Debounce dos sensores

Sensores IR podem oscilar quando o carrinho passa, quando ha reflexo ou quando a alimentacao esta instavel. Para evitar entradas e saidas falsas, o ESP32 deve confirmar a leitura antes de enviar.

Recomendacao:

- ler cada sensor continuamente;
- considerar uma mudanca valida somente se o novo estado permanecer igual por 500 ms a 1 segundo;
- enviar para a API somente quando o estado confirmado for diferente do ultimo estado enviado.

Mesmo assim, o servidor tambem protege contra eventos repetidos: se a vaga ja esta ocupada e o ESP32 envia `ocupada: true` varias vezes, o sistema nao registra novas entradas.

## Configuracao padrao do sketch

O sketch usa por padrao:

```cpp
const int sensorPins[4] = { 13, 12, 14, 27 };
const int SENSOR_DETECTADO = LOW;
```

O envio e feito para a rota em lote:

```cpp
const char* SERVER_URL = "http://192.168.0.100:3000/api/vagas/status/lote";
```

O display OLED e o RTC DS3231 ficam desativados por padrao para o codigo compilar mesmo sem essas bibliotecas. Para ativar, instale as bibliotecas `RTClib`, `Adafruit GFX Library` e `Adafruit SSD1306`, depois altere:

```cpp
#define USAR_RTC_DS3231 1
#define USAR_OLED 1
```

## Exemplo Arduino/ESP32 individual

Este exemplo usa `WiFi.h` e `HTTPClient.h`. Ajuste os pinos dos sensores, o nome da rede, a senha e o IP do computador que esta rodando o servidor.

```cpp
#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "NOME_DA_REDE";
const char* WIFI_PASSWORD = "SENHA_DA_REDE";

// Local:  http://192.168.0.100:3000/api/vagas/status
// Vercel: https://SEU-PROJETO.vercel.app/api/vagas/status
const char* SERVER_URL = "http://192.168.0.100:3000/api/vagas/status";

const int TOTAL_VAGAS = 4;
const int sensorPins[TOTAL_VAGAS] = { 13, 12, 14, 27 };

// Ajuste conforme o modulo IR usado.
// Em muitos sensores IR, LOW significa objeto detectado.
const int SENSOR_DETECTADO = LOW;

bool ultimoEstadoEnviado[TOTAL_VAGAS] = { false, false, false, false };
bool estadoCandidato[TOTAL_VAGAS] = { false, false, false, false };
unsigned long inicioCandidato[TOTAL_VAGAS] = { 0, 0, 0, 0 };

const unsigned long TEMPO_DEBOUNCE_MS = 800;

void conectarWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando ao WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi conectado. IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

bool lerSensorOcupado(int index) {
  int leitura = digitalRead(sensorPins[index]);
  return leitura == SENSOR_DETECTADO;
}

void enviarStatusVaga(int vaga, bool ocupada) {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"vaga\":";
  json += String(vaga);
  json += ",";
  json += "\"ocupada\":";
  json += ocupada ? "true" : "false";
  json += "}";

  int httpCode = http.POST(json);

  Serial.print("Vaga ");
  Serial.print(vaga);
  Serial.print(" -> ");
  Serial.print(ocupada ? "OCUPADA" : "LIVRE");
  Serial.print(" | HTTP ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(http.getString());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    pinMode(sensorPins[i], INPUT);
    bool ocupada = lerSensorOcupado(i);
    ultimoEstadoEnviado[i] = ocupada;
    estadoCandidato[i] = ocupada;
  }

  conectarWiFi();

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    enviarStatusVaga(i + 1, ultimoEstadoEnviado[i]);
    delay(150);
  }
}

void loop() {
  unsigned long agora = millis();

  for (int i = 0; i < TOTAL_VAGAS; i++) {
    bool leituraAtual = lerSensorOcupado(i);

    if (leituraAtual != estadoCandidato[i]) {
      estadoCandidato[i] = leituraAtual;
      inicioCandidato[i] = agora;
    }

    bool leituraEstavel = (agora - inicioCandidato[i]) >= TEMPO_DEBOUNCE_MS;
    bool mudouDesdeUltimoEnvio = estadoCandidato[i] != ultimoEstadoEnviado[i];

    if (leituraEstavel && mudouDesdeUltimoEnvio) {
      ultimoEstadoEnviado[i] = estadoCandidato[i];
      enviarStatusVaga(i + 1, estadoCandidato[i]);
    }
  }

  delay(50);
}
```

## Usando timestamp do RTC DS3231

Quando o modulo RTC estiver integrado, monte o JSON incluindo `timestamp` no formato ISO:

```json
{
  "vaga": 1,
  "ocupada": true,
  "timestamp": "2026-08-11T14:32:00"
}
```

O servidor ja aceita esse campo. Assim, se o computador reiniciar ou estiver com relogio incorreto, a maquete podera mandar o horario confiavel do DS3231.
