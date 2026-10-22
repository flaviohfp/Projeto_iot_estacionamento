#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// =========================================
// ESTACIONAMENTO INTELIGENTE
// 1 ESP32 + 2 SENSORES + 2 LEDS RGB + LCD
// =========================================

// LCD I2C 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =========================================
// PINOS DOS SENSORES HC-SR04
// =========================================

// Vaga 1: TRIG D4 / ECHO D5
// Vaga 2: TRIG D2 / ECHO D18

const int trigPins[2] = {4, 2};
const int echoPins[2] = {5, 18};

// =========================================
// PINOS DOS LEDS RGB
// =========================================

// Vaga 1: VERDE D13 / VERMELHO D14
// Vaga 2: VERDE D27 / VERMELHO D12

const int ledVerde[2] = {13, 27};
const int ledVermelho[2] = {14, 12};

// Modulos RGB com terminal negativo no GND
const int LED_LIGADO = HIGH;
const int LED_DESLIGADO = LOW;

// =========================================
// CONFIGURACOES
// =========================================

// Carrinho a 10 cm ou menos: OCUPADA
const float DISTANCIA_OCUPADA = 10.0;

// Carrinho a 13 cm ou mais: LIVRE
const float DISTANCIA_LIVRE = 13.0;

// Duas leituras consecutivas para confirmar
const int CONFIRMACOES = 2;

// Cinco leituras invalidas indicam erro
const int LIMITE_ERROS = 5;

// Atualizacao mais lenta: 500 ms
const unsigned long INTERVALO = 500;

// Pausa entre sensores: 80 ms
const unsigned long PAUSA_SENSORES = 80;

// =========================================
// ESTADOS DAS VAGAS
// =========================================

// -1 = verificando / erro
//  0 = livre
//  1 = ocupada

int estado[2] = {-1, -1};
int candidato[2] = {-1, -1};
int confirmacoes[2] = {0, 0};
int erros[2] = {0, 0};

unsigned long ultimoCiclo = 0;

// Adicoes para o SITE: nao modificam a logica dos sensores ou LEDs.
float distanciaSite[2] = {-1.0, -1.0};
unsigned long ocupadaDesde[2] = {0, 0};
WebServer servidor(80);

// Alterar somente estes dados conforme seu Wi-Fi:
const char* WIFI_SSID = "Matheus";
const char* WIFI_SENHA = "matheus0402";


// =========================================
// MEDIR DISTANCIA
// =========================================

float medirDistancia(int vaga) {

  int trig = trigPins[vaga];
  int echo = echoPins[vaga];

  digitalWrite(trig, LOW);
  delayMicroseconds(5);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // Aguarda o eco por ate 30 milissegundos
  unsigned long duracao = pulseIn(
    echo,
    HIGH,
    30000UL
  );

  if (duracao == 0) {
    return -1.0;
  }

  float distancia = duracao * 0.0343 / 2.0;

  // Rejeita valores fora da faixa valida
  if (distancia < 2.0 || distancia > 400.0) {
    return -1.0;
  }

  return distancia;
}

// =========================================
// CONTROLAR LED INDIVIDUAL
// =========================================

void atualizarLED(int vaga) {

  // Apaga primeiro as duas cores desta vaga
  digitalWrite(ledVerde[vaga], LED_DESLIGADO);
  digitalWrite(ledVermelho[vaga], LED_DESLIGADO);

  if (estado[vaga] == 0) {

    // Vaga livre: VERDE
    digitalWrite(ledVerde[vaga], LED_LIGADO);

  } else if (estado[vaga] == 1) {

    // Vaga ocupada: VERMELHO
    digitalWrite(ledVermelho[vaga], LED_LIGADO);

  }

  // Se estado == -1, ambas ficam apagadas
}

// =========================================
// ATUALIZAR DISPLAY LCD
// =========================================

void atualizarLCD() {

  lcd.setCursor(0, 0);
  lcd.print("Vaga 1: ");

  if (estado[0] == 0) {
    lcd.print("LIVRE   ");
  } else if (estado[0] == 1) {
    lcd.print("OCUPADA ");
  } else {
    lcd.print("VERIFIC.");
  }

  lcd.setCursor(0, 1);
  lcd.print("Vaga 2: ");

  if (estado[1] == 0) {
    lcd.print("LIVRE   ");
  } else if (estado[1] == 1) {
    lcd.print("OCUPADA ");
  } else {
    lcd.print("VERIFIC.");
  }
}

// =========================================
// DEFINIR ESTADO DA VAGA
// =========================================

void definirEstado(int vaga, int novoEstado) {

  // So atualiza quando o estado mudar
  if (estado[vaga] == novoEstado) {
    return;
  }

  estado[vaga] = novoEstado;
  // Apenas registra o instante para o cronometro exibido no site.
  if (novoEstado == 1) ocupadaDesde[vaga] = millis();
  else ocupadaDesde[vaga] = 0;

  // Altera somente o LED da vaga correta
  atualizarLED(vaga);

  // Atualiza o LCD
  atualizarLCD();

  Serial.print(">>> VAGA ");
  Serial.print(vaga + 1);

  if (novoEstado == 0) {
    Serial.println(" LIVRE");
  } else if (novoEstado == 1) {
    Serial.println(" OCUPADA");
  } else {
    Serial.println(" SEM LEITURA VALIDA");
  }
}

// =========================================
// PROCESSAR SENSOR DA VAGA
// =========================================

void processarSensor(int vaga) {

  float distancia = medirDistancia(vaga);
  distanciaSite[vaga] = distancia; // Apenas informa a distancia ao site.

  Serial.print("Vaga ");
  Serial.print(vaga + 1);
  Serial.print(" | ");

  // LEITURA INVALIDA
  if (distancia < 0) {

    Serial.println("SEM ECO");

    erros[vaga]++;
    candidato[vaga] = -1;
    confirmacoes[vaga] = 0;


    return;
  }

  // Leitura valida
  erros[vaga] = 0;

  Serial.print(distancia, 1);
  Serial.print(" cm | ");

  int novoEstado = -1;

  // DETECCAO DO CARRINHO
  if (distancia <= DISTANCIA_OCUPADA) {

    novoEstado = 1;
    Serial.println("CARRINHO DETECTADO");

  } else if (distancia >= DISTANCIA_LIVRE) {

    novoEstado = 0;
    Serial.println("SEM CARRINHO");

  } else {

    // Entre 10 e 13 cm:
    // mantem a cor anterior
    Serial.println("ZONA INTERMEDIARIA");

    candidato[vaga] = -1;
    confirmacoes[vaga] = 0;

    return;
  }

  // ESTADO JA CONFIRMADO
  if (novoEstado == estado[vaga]) {

    candidato[vaga] = -1;
    confirmacoes[vaga] = 0;

    return;
  }

  // CONTAR LEITURAS CONSECUTIVAS
  if (candidato[vaga] == novoEstado) {

    confirmacoes[vaga]++;

  } else {

    candidato[vaga] = novoEstado;
    confirmacoes[vaga] = 1;
  }

  // MUDA A COR APOS 2 LEITURAS
  if (confirmacoes[vaga] >= CONFIRMACOES) {

    candidato[vaga] = -1;
    confirmacoes[vaga] = 0;

    definirEstado(vaga, novoEstado);
  }
}


// =========================================
// SITE ORIGINAL EMBUTIDO NO CODIGO
// =========================================
const char paginaHTML[] PROGMEM = R"HTMLSITE(<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Estacionamento Inteligente</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <header class="topbar">
    <div>
      <p class="eyebrow">Projeto IoT - 2 vagas</p>
      <h1>Estacionamento Inteligente</h1>
    </div>
    <div class="system-status" aria-live="polite">
      <span id="connection-dot" class="online-dot offline"></span>
      <span id="connection-text">Conectando</span>
      <small>Última atualização: <strong id="last-update">--:--:--</strong></small>
    </div>
  </header>

  <main class="dashboard">
    <section class="hero-panel">
      <div class="section-header">
        <div>
          <span class="panel-kicker">Maquete conectada ao site</span>
          <h2>Mapa das Vagas</h2>
        </div>
        <div class="health-row">
          <span class="health-chip">Servidor: ESP32</span>
          <span class="health-chip">Atualização: a cada 1 s</span>
        </div>
      </div>
      <div class="parking-lot" aria-label="Mapa das vagas do estacionamento">
        <div id="parking-map" class="parking-grid"></div>
      </div>
    </section>

    <section class="indicators" aria-label="Indicadores do estacionamento">
      <article class="metric-card">
        <span>Vagas Livres</span>
        <strong id="free-count">--</strong>
      </article>
      <article class="metric-card">
        <span>Vagas Ocupadas</span>
        <strong id="occupied-count">--</strong>
      </article>
      <article class="metric-card">
        <span>Taxa de Ocupação</span>
        <strong id="occupancy-rate">--</strong>
      </article>
      <article class="metric-card lcd-card">
        <span>Display LCD</span>
        <pre id="lcd-screen" class="lcd-screen">----------------
----------------</pre>
      </article>
    </section>

    <section class="integration-grid">
      <div class="availability-section section-block">
        <div class="section-header">
          <h2>Vagas disponíveis agora</h2>
        </div>
        <div id="availability-message" class="availability-message"></div>
        <div id="available-list" class="available-list"></div>
      </div>

      <div class="integration-panel section-block">
        <div class="section-header">
          <h2>Integração ESP32</h2>
        </div>
        <div class="endpoint-box">
          <span>GET</span>
          <code>/api/vagas</code>
        </div>
        <p>A ESP32 lê os sensores ultrassônicos, acende os LEDs RGB, escreve no LCD e serve este painel.</p>
        <div class="hardware-list" aria-label="Componentes da maquete">
          <span>1 ESP32</span>
          <span>2 HC-SR04</span>
          <span>2 LEDs RGB</span>
          <span>LCD I2C 16x2</span>
        </div>
      </div>
    </section>
  </main>

  <script src="script.js"></script>
</body>
</html>
)HTMLSITE";
const char paginaCSS[] PROGMEM = R"CSSSITE(:root {
  --blue-950: #071426;
  --blue-900: #0c1f38;
  --blue-800: #12345a;
  --blue-100: #e8f1fb;
  --green: #16a34a;
  --green-dark: #15803d;
  --green-soft: #dcfce7;
  --red: #dc2626;
  --red-dark: #b91c1c;
  --red-soft: #fee2e2;
  --amber: #f59e0b;
  --asphalt: #26313f;
  --line: #f8fafc;
  --white: #ffffff;
  --gray-50: #f8fafc;
  --gray-100: #eef2f7;
  --gray-300: #cbd5e1;
  --gray-500: #64748b;
  --gray-700: #334155;
  --shadow: 0 18px 40px rgba(7, 20, 38, 0.12);
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-height: 100vh;
  background: linear-gradient(180deg, #f8fafc 0%, #eef4f8 100%);
  color: var(--blue-950);
  font-family: Arial, Helvetica, sans-serif;
}

.topbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 24px;
  padding: 28px clamp(18px, 5vw, 56px);
  background: var(--blue-950);
  color: var(--white);
}

.eyebrow {
  margin: 0 0 6px;
  color: #9cc7f2;
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0;
  text-transform: uppercase;
}

h1,
h2,
p {
  margin-top: 0;
}

h1 {
  margin-bottom: 0;
  font-size: clamp(1.7rem, 4vw, 2.8rem);
  line-height: 1.1;
}

h2 {
  margin-bottom: 0;
  font-size: 1.16rem;
}

.system-status {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
  justify-content: flex-end;
  color: var(--blue-100);
  font-weight: 700;
}

.system-status small {
  width: 100%;
  color: #b7c9dc;
  font-weight: 400;
  text-align: right;
}

.online-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: var(--green);
  box-shadow: 0 0 0 6px rgba(22, 163, 74, 0.18);
}

.online-dot.offline {
  background: var(--red);
  box-shadow: 0 0 0 6px rgba(220, 38, 38, 0.18);
}

.dashboard {
  width: min(1180px, calc(100% - 32px));
  margin: 26px auto 44px;
}

.section-block,
.hero-panel {
  margin-top: 22px;
}

.hero-panel,
.integration-panel,
.parking-card,
.metric-card {
  background: var(--white);
  border: 1px solid var(--gray-100);
  border-radius: 8px;
}

.hero-panel,
.metric-card,
.integration-panel {
  box-shadow: var(--shadow);
}

.hero-panel {
  padding: 18px;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  margin-bottom: 14px;
}

.panel-kicker {
  display: block;
  margin-bottom: 5px;
  color: var(--gray-500);
  font-size: 0.76rem;
  font-weight: 900;
  text-transform: uppercase;
}

.health-row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  justify-content: flex-end;
}

.health-chip {
  padding: 8px 10px;
  border-radius: 8px;
  background: var(--blue-100);
  color: var(--blue-900);
  font-size: 0.78rem;
  font-weight: 900;
}

.parking-lot {
  position: relative;
  overflow: hidden;
  padding: 18px;
  border-radius: 8px;
  background:
    linear-gradient(90deg, rgba(255, 255, 255, 0.16) 1px, transparent 1px) 0 0 / 50% 100%,
    linear-gradient(180deg, #313d4c 0%, var(--asphalt) 100%);
  border: 1px solid #1f2937;
}

.parking-lot::before {
  content: "";
  position: absolute;
  left: 18px;
  right: 18px;
  top: 16px;
  height: 5px;
  border-radius: 999px;
  background: rgba(248, 250, 252, 0.75);
}

.parking-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  position: relative;
  z-index: 1;
}

.parking-card {
  position: relative;
  overflow: hidden;
  min-height: 240px;
  padding: 18px 16px 16px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  background: rgba(255, 255, 255, 0.96);
  box-shadow: 0 12px 28px rgba(7, 20, 38, 0.18);
}

.parking-card::before,
.parking-card::after {
  content: "";
  position: absolute;
  top: 0;
  bottom: 0;
  width: 4px;
  background: var(--line);
  opacity: 0.9;
}

.parking-card::before {
  left: 0;
}

.parking-card::after {
  right: 0;
}

.parking-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  position: relative;
  z-index: 1;
}

.parking-card .label {
  color: var(--gray-500);
  font-size: 0.82rem;
  font-weight: 800;
  text-transform: uppercase;
}

.parking-card .number {
  margin-top: 4px;
  font-size: 2.15rem;
  font-weight: 900;
}

.car-mark {
  position: relative;
  display: block;
  width: 64px;
  height: 44px;
  border-radius: 20px 20px 10px 10px;
  background: var(--green);
  border: 3px solid var(--green-dark);
  flex: 0 0 auto;
}

.car-mark::before {
  content: "";
  position: absolute;
  left: 13px;
  right: 13px;
  top: 7px;
  height: 14px;
  border-radius: 10px 10px 4px 4px;
  background: rgba(255, 255, 255, 0.84);
}

.car-mark::after {
  content: "";
  position: absolute;
  left: 8px;
  right: 8px;
  bottom: -7px;
  height: 13px;
  background:
    radial-gradient(circle at 0 50%, var(--blue-950) 0 6px, transparent 7px),
    radial-gradient(circle at 100% 50%, var(--blue-950) 0 6px, transparent 7px);
}

.car-light {
  position: absolute;
  top: 2px;
  right: -12px;
  width: 13px;
  height: 13px;
  border-radius: 50%;
  background: var(--green);
  border: 2px solid var(--white);
  box-shadow: 0 0 0 4px rgba(22, 163, 74, 0.18), 0 0 14px rgba(22, 163, 74, 0.65);
}

.occupied .car-mark {
  background: var(--red);
  border-color: var(--red-dark);
}

.occupied .car-light {
  background: var(--red);
  box-shadow: 0 0 0 4px rgba(220, 38, 38, 0.18), 0 0 14px rgba(220, 38, 38, 0.65);
}

.sensor-mark {
  position: absolute;
  right: 18px;
  bottom: 18px;
  width: 36px;
  height: 36px;
  border: 2px dashed var(--amber);
  border-radius: 50%;
  background: rgba(245, 158, 11, 0.12);
}

.sensor-mark::after {
  content: "";
  position: absolute;
  inset: 11px;
  border-radius: 50%;
  background: var(--amber);
}

.parking-details {
  min-height: 56px;
  color: var(--gray-700);
  font-size: 0.9rem;
  line-height: 1.45;
  position: relative;
  z-index: 1;
}

.parking-details strong {
  color: var(--blue-950);
}

.status-pill {
  width: fit-content;
  padding: 9px 13px;
  border-radius: 999px;
  color: var(--green);
  background: var(--green-soft);
  font-size: 0.86rem;
  font-weight: 900;
  position: relative;
  z-index: 1;
}

.occupied .status-pill {
  color: var(--red);
  background: var(--red-soft);
}

.error .car-mark {
  background: var(--gray-300);
  border-color: var(--gray-500);
}

.error .car-light {
  background: #2563eb;
  box-shadow: 0 0 0 4px rgba(37, 99, 235, 0.18), 0 0 14px rgba(37, 99, 235, 0.65);
}

.error .status-pill {
  color: #92400e;
  background: #fef3c7;
}

.indicators {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 16px;
  margin-top: 22px;
}

.metric-card {
  padding: 20px;
}

.metric-card span {
  display: block;
  color: var(--gray-500);
  font-weight: 800;
}

.metric-card strong {
  display: block;
  margin-top: 10px;
  font-size: 2.1rem;
}

.metric-card small {
  display: block;
  margin-top: 7px;
  color: var(--gray-500);
  font-weight: 700;
}

.lcd-card {
  background: var(--blue-950);
  color: var(--white);
}

.lcd-card span,
.lcd-card small {
  color: #b7d5ed;
}

.lcd-screen {
  margin: 12px 0 0;
  padding: 10px 12px;
  border-radius: 6px;
  background: #1f4d2b;
  color: #b8f5c4;
  font-family: "Courier New", Courier, monospace;
  font-size: 1rem;
  font-weight: 700;
  line-height: 1.4;
  overflow-x: auto;
}

.integration-grid {
  display: grid;
  grid-template-columns: minmax(0, 1.3fr) minmax(300px, 0.7fr);
  gap: 18px;
  align-items: stretch;
}

.availability-message {
  padding: 14px 16px;
  border-radius: 8px;
  background: var(--green-soft);
  color: #166534;
  font-weight: 900;
}

.availability-message.full {
  background: var(--red-soft);
  color: #991b1b;
}

.availability-message.warning {
  background: #fef3c7;
  color: #92400e;
}

.available-list {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 12px;
}

.available-chip {
  padding: 10px 13px;
  border-radius: 8px;
  background: var(--white);
  border: 1px solid var(--gray-300);
  color: var(--blue-900);
  font-weight: 800;
}

.integration-panel {
  padding: 18px;
}

.endpoint-box {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 13px;
  border-radius: 8px;
  background: var(--blue-100);
  color: var(--blue-900);
  min-width: 0;
}

.endpoint-box span {
  flex: 0 0 auto;
  padding: 6px 8px;
  border-radius: 6px;
  background: var(--blue-900);
  color: var(--white);
  font-size: 0.74rem;
  font-weight: 900;
}

.endpoint-box code {
  min-width: 0;
  overflow-wrap: anywhere;
  font-size: 0.9rem;
  font-weight: 800;
}

.hardware-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 9px;
  margin-top: 13px;
}

.hardware-list span {
  padding: 10px;
  border-radius: 8px;
  border: 1px solid var(--gray-100);
  color: var(--gray-700);
  font-size: 0.86rem;
  font-weight: 800;
}

@media (max-width: 980px) {
  .parking-grid,
  .indicators,
  .integration-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 720px) {
  .topbar {
    align-items: flex-start;
    flex-direction: column;
  }

  .system-status,
  .health-row {
    justify-content: flex-start;
  }

  .system-status small {
    text-align: left;
  }

  .indicators,
  .integration-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 540px) {
  .parking-grid {
    grid-template-columns: 1fr;
  }

  .parking-card {
    min-height: 220px;
  }

  .parking-lot {
    padding: 14px;
  }

  .hardware-list {
    grid-template-columns: 1fr;
  }
}
)CSSSITE";
const char paginaJS[] PROGMEM = R"JSSITE(// Painel do estacionamento servido pela ESP32.
// A ESP32 responde GET /api/vagas com:
// {"vagas":[{"numero":1,"ocupada":true,"erro":false,"distancia":6.4,"segundos":42}, ...]}
// "segundos" = tempo que a vaga está ocupada (millis() na ESP32).
// "distancia" = leitura do HC-SR04 em cm (null quando não há eco).
const API_URL = "/api/vagas";
const INTERVALO_MS = 1000;

const $ = id => document.getElementById(id);
const parkingMap = $("parking-map");
const freeCount = $("free-count");
const occupiedCount = $("occupied-count");
const occupancyRate = $("occupancy-rate");
const availableList = $("available-list");
const availabilityMessage = $("availability-message");
const lastUpdate = $("last-update");
const connectionDot = $("connection-dot");
const connectionText = $("connection-text");
const lcdScreen = $("lcd-screen");

let vagas = null;
let recebidoEm = 0;
let carregando = false;

function setConnectionState(online, text) {
  connectionDot.classList.toggle("offline", !online);
  connectionText.textContent = text;
}

function formatDuration(totalSeconds) {
  const s = Math.max(0, Math.floor(totalSeconds));
  return [Math.floor(s / 3600), Math.floor((s % 3600) / 60), s % 60]
    .map(v => String(v).padStart(2, "0"))
    .join(":");
}

function segundosOcupada(vaga) {
  return vaga.segundos + (Date.now() - recebidoEm) / 1000;
}

function normalize(data) {
  const lista = data && data.vagas;
  if (!Array.isArray(lista) || !lista.length) {
    throw new Error("Resposta inválida da ESP32.");
  }
  return lista
    .map(v => ({
      numero: Number(v.numero),
      ocupada: v.ocupada === true,
      erro: v.erro === true,
      distancia: Number.isFinite(v.distancia) ? v.distancia : null,
      segundos: Number.isFinite(v.segundos) ? v.segundos : 0
    }))
    .sort((a, b) => a.numero - b.numero);
}

function estado(vaga) {
  if (vaga.erro) return { classe: "error", texto: "SEM SENSOR", lcd: "ERRO" };
  if (vaga.ocupada) return { classe: "occupied", texto: "OCUPADA", lcd: "OCUP." };
  return { classe: "", texto: "LIVRE", lcd: "LIVRE" };
}

function renderParkingMap() {
  parkingMap.innerHTML = vagas.map(vaga => {
    const e = estado(vaga);
    const distancia = vaga.distancia === null ? "sem eco" : `${vaga.distancia.toFixed(1)} cm`;
    let details = "<div>Disponível para entrada</div>";
    if (vaga.erro) details = "<div>Verifique a ligação do sensor</div>";
    else if (vaga.ocupada) details = `<div>Tempo estacionado: <strong data-duration="${vaga.numero}">--:--:--</strong></div>`;

    return `
      <article class="parking-card ${e.classe}">
        <div class="parking-head">
          <div>
            <span class="label">Vaga</span>
            <div class="number">${vaga.numero}</div>
          </div>
          <span class="car-mark" aria-hidden="true"><span class="car-light"></span></span>
        </div>
        <span class="sensor-mark" aria-hidden="true"></span>
        <div class="parking-details">
          ${details}
          <div>Sensor: <strong>${distancia}</strong></div>
        </div>
        <span class="status-pill">${e.texto}</span>
      </article>
    `;
  }).join("");
}

// Mesmo texto que a ESP32 escreve no LCD 16x2.
function lcdLinhas(livres) {
  const linha1 = livres.length ? `Livres: ${livres.length} de ${vagas.length}` : "ESTAC. LOTADO";
  const linha2 = vagas.map(v => `${v.numero}:${estado(v).lcd}`.padEnd(8)).join("");
  return [linha1.padEnd(16).slice(0, 16), linha2.padEnd(16).slice(0, 16)].join("\n");
}

function renderIndicators() {
  const livres = vagas.filter(v => !v.ocupada && !v.erro);
  const ocupadas = vagas.filter(v => v.ocupada && !v.erro);
  const comErro = vagas.filter(v => v.erro);

  freeCount.textContent = livres.length;
  occupiedCount.textContent = ocupadas.length;
  occupancyRate.textContent = `${Math.round((ocupadas.length / vagas.length) * 100)}%`;
  lcdScreen.textContent = lcdLinhas(livres);

  availableList.innerHTML = livres
    .map(v => `<span class="available-chip">Vaga ${v.numero}</span>`)
    .join("");

  if (comErro.length) {
    availabilityMessage.textContent = `Falha no sensor da vaga ${comErro.map(v => v.numero).join(", ")}`;
    availabilityMessage.className = "availability-message warning";
  } else if (!livres.length) {
    availabilityMessage.textContent = "ESTACIONAMENTO LOTADO";
    availabilityMessage.className = "availability-message full";
  } else {
    availabilityMessage.textContent = "Estacionamento com vagas disponíveis";
    availabilityMessage.className = "availability-message";
  }
}

function updateRunningDurations() {
  if (!vagas) return;
  vagas.filter(v => v.ocupada).forEach(vaga => {
    const el = document.querySelector(`[data-duration="${vaga.numero}"]`);
    if (el) el.textContent = formatDuration(segundosOcupada(vaga));
  });
}

function renderError(message) {
  vagas = null;
  setConnectionState(false, "Sem conexão com a ESP32");
  availabilityMessage.textContent = message;
  availabilityMessage.className = "availability-message warning";
  availableList.innerHTML = "";
  [freeCount, occupiedCount, occupancyRate].forEach(el => { el.textContent = "--"; });
  lcdScreen.textContent = "----------------\n----------------";
  parkingMap.textContent = "Aguardando leitura dos sensores.";
}

async function carregar() {
  if (carregando) return;
  carregando = true;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 4000);
  try {
    const response = await fetch(API_URL, { cache: "no-store", signal: controller.signal });
    if (!response.ok) throw new Error(`ESP32 respondeu com erro ${response.status}.`);
    vagas = normalize(await response.json());
    recebidoEm = Date.now();
    setConnectionState(true, "Conectado à ESP32");
    renderParkingMap();
    renderIndicators();
    updateRunningDurations();
    lastUpdate.textContent = new Date().toLocaleTimeString("pt-BR");
  } catch (error) {
    renderError(error.name === "AbortError" ? "A ESP32 não respondeu a tempo." : error.message);
  } finally {
    clearTimeout(timeout);
    carregando = false;
  }
}

carregar();
setInterval(updateRunningDurations, 1000);
setInterval(() => { if (!document.hidden) carregar(); }, INTERVALO_MS);
document.addEventListener("visibilitychange", () => { if (!document.hidden) carregar(); });
)JSSITE";

void responderVagas() {
  // Formato esperado pelo script.js original.
  // O estado -1 significa leitura ainda nao confirmada.
  String json = "{\"vagas\":[";
  for (int i = 0; i < 2; i++) {
    if (i) json += ",";
    json += "{\"numero\":" + String(i + 1);
    json += ",\"ocupada\":" + String(estado[i] == 1 ? "true" : "false");
    json += ",\"erro\":" + String(estado[i] == -1 ? "true" : "false");
    json += ",\"distancia\":";
    if (distanciaSite[i] < 0) json += "null";
    else json += String(distanciaSite[i], 1);
    json += ",\"segundos\":";
    json += String(estado[i] == 1 ? (millis() - ocupadaDesde[i]) / 1000UL : 0UL);
    json += "}";
  }
  json += "]}";
  servidor.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  servidor.send(200, "application/json; charset=utf-8", json);
}

void iniciarSite() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);

  Serial.print("Conectando ao Wi-Fi");
  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 10000UL) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("SITE: http://");
    Serial.println(WiFi.localIP());
  } else {
    // Permite abrir o site mesmo sem roteador.
    WiFi.mode(WIFI_AP);
    WiFi.softAP("EstacionamentoESP32", "12345678");
    Serial.println("Rede: EstacionamentoESP32 | Senha: 12345678");
    Serial.print("SITE: http://");
    Serial.println(WiFi.softAPIP());
  }

  servidor.on("/", HTTP_GET, []() {
    servidor.send_P(200, "text/html; charset=utf-8", paginaHTML);
  });
  servidor.on("/style.css", HTTP_GET, []() {
    servidor.send_P(200, "text/css; charset=utf-8", paginaCSS);
  });
  servidor.on("/script.js", HTTP_GET, []() {
    servidor.send_P(200, "application/javascript; charset=utf-8", paginaJS);
  });
  servidor.on("/api/vagas", HTTP_GET, responderVagas);
  servidor.onNotFound([]() { servidor.send(404, "text/plain", "Nao encontrado"); });
  servidor.begin();
  Serial.println("Servidor do site iniciado.");
}

// =========================================
// SETUP
// =========================================

void setup() {

  Serial.begin(115200);

  for (int i = 0; i < 2; i++) {

    // Sensores
    pinMode(trigPins[i], OUTPUT);
    digitalWrite(trigPins[i], LOW);

    pinMode(echoPins[i], INPUT);

    // LEDs
    pinMode(ledVerde[i], OUTPUT);
    pinMode(ledVermelho[i], OUTPUT);

    // LEDs inicialmente apagados
    digitalWrite(ledVerde[i], LED_DESLIGADO);
    digitalWrite(ledVermelho[i], LED_DESLIGADO);
  }

  // Iniciar LCD
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("ESTACIONAMENTO");

  lcd.setCursor(0, 1);
  lcd.print("INICIANDO...");

  delay(1000);

  lcd.clear();
  atualizarLCD();

  Serial.println();
  Serial.println("===========================");
  Serial.println("ESTACIONAMENTO INTELIGENTE");
  Serial.println("===========================");
  Serial.println("V1: TRIG 4 / ECHO 5");
  Serial.println("V2: TRIG 2 / ECHO 18");
  Serial.println("LED 1: VERDE 13 / VERM 14");
  Serial.println("LED 2: VERDE 27 / VERM 12");
  Serial.println("OCUPADA: 10 cm ou menos");
  Serial.println("LIVRE: 13 cm ou mais");
  Serial.println("INTERVALO: 500 ms");
  Serial.println("===========================");

  iniciarSite(); // Acrescentado: inicia somente o site.
}

// =========================================
// LOOP PRINCIPAL
// =========================================

void loop() {
  servidor.handleClient(); // Atende o navegador sem mudar a leitura.

  // Atualiza aproximadamente a cada 500 ms
  if (millis() - ultimoCiclo < INTERVALO) {
    return;
  }

  ultimoCiclo = millis();

  // Sensor da vaga 1
  processarSensor(0);

  // Aguarda para evitar interferencia
  delay(PAUSA_SENSORES);

  // Sensor da vaga 2
  processarSensor(1);

  Serial.println("---------------------------");
}
