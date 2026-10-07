// Gerado por tools/embed-site.cjs a partir de data/.
#pragma once
#include <Arduino.h>

const char SITE_INDEX[] PROGMEM = R"parkingasset(<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Estacionamento Inteligente</title>
  <link rel="stylesheet" href="./style.css">
</head>
<body>
  <header class="topbar">
    <div>
      <p class="eyebrow">Projeto IoT - 4 vagas</p>
      <h1>Estacionamento Inteligente</h1>
      <a id="mode-link" href="?modo=demo">Testar demonstração local</a>
    </div>
    <div class="system-status" aria-live="polite">
      <span id="connection-dot" class="online-dot"></span>
      <span id="connection-text">Conectando</span>
      <small>Ultima atualizacao: <strong id="last-update">--:--:--</strong></small>
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
          <span id="database-mode" class="health-chip">Banco: Firebase</span>
          <span id="realtime-mode" class="health-chip">Tempo real: --</span>
        </div>
      </div>
      <div class="parking-lot" aria-label="Mapa das quatro vagas do estacionamento">
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
        <span>Taxa de Ocupacao</span>
        <strong id="occupancy-rate">--</strong>
      </article>
      <article class="metric-card lcd-card">
        <span>Display LCD</span>
        <strong id="lcd-free">--</strong>
        <small id="lcd-list">Vagas: --</small>
      </article>
    </section>

    <section class="integration-grid">
      <div class="availability-section section-block">
        <div class="section-header">
          <h2>Vagas disponiveis agora</h2>
        </div>
        <div id="availability-message" class="availability-message"></div>
        <div id="available-list" class="available-list"></div>
      </div>

      <div class="integration-panel section-block">
        <div class="section-header">
          <h2>Integração ESP32</h2>
        </div>
        <div class="endpoint-box">
          <span id="endpoint-method">FIREBASE</span>
          <code id="api-endpoint">Cloud Firestore</code>
        </div>
        <p id="hardware-status" role="status">Verificando integração com a maquete...</p>
        <div class="hardware-list" aria-label="Componentes da maquete">
          <span>2 ESP32</span>
          <span>4 HC-SR04</span>
          <span>4 LEDs RGB</span>
          <span>LCD I2C 16x2</span>
          <span>NTP (horário)</span>
        </div>
      </div>
    </section>

    <section class="section-block">
      <div class="section-header">
        <h2>Historico de Movimentacoes</h2>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Data</th>
              <th>Hora</th>
              <th>Vaga</th>
              <th>Evento</th>
              <th>Entrada</th>
              <th>Saida</th>
              <th>Tempo de permanencia</th>
            </tr>
          </thead>
          <tbody id="history-body">
            <tr>
              <td colspan="7" class="empty-table">Nenhuma movimentacao registrada.</td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>

  </main>

  <script src="./config.js"></script>
  <script src="./data.js"></script>
  <script src="./script.js"></script>
</body>
</html>
)parkingasset";

const char SITE_STYLE[] PROGMEM = R"parkingasset(:root {
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
.metric-card,
.table-wrap,
.simulation-control {
  background: var(--white);
  border: 1px solid var(--gray-100);
  border-radius: 8px;
}

.hero-panel,
.metric-card,
.table-wrap,
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
    linear-gradient(90deg, rgba(255, 255, 255, 0.16) 1px, transparent 1px) 0 0 / 25% 100%,
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
  grid-template-columns: repeat(4, minmax(0, 1fr));
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
  left: 50%;
  bottom: 82px;
  width: 36px;
  height: 36px;
  transform: translateX(-50%);
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

.lcd-card strong {
  color: #63e6be;
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

.table-wrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  min-width: 820px;
}

th,
td {
  padding: 13px 14px;
  border-bottom: 1px solid var(--gray-100);
  text-align: left;
  white-space: nowrap;
}

th {
  background: var(--blue-900);
  color: var(--white);
  font-size: 0.84rem;
}

td {
  color: var(--gray-700);
}

tr:last-child td {
  border-bottom: 0;
}

.event-badge {
  display: inline-flex;
  align-items: center;
  min-width: 74px;
  justify-content: center;
  padding: 7px 10px;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 900;
}

.event-badge.entry {
  color: #166534;
  background: var(--green-soft);
}

.event-badge.exit {
  color: #991b1b;
  background: var(--red-soft);
}

.empty-table {
  text-align: center;
  color: var(--gray-500);
}

.simulation-panel {
  padding-top: 4px;
}

.simulation-grid {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 14px;
}

.simulation-control {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 15px;
}

.simulation-control strong {
  display: block;
}

.simulation-control small {
  color: var(--gray-500);
}

.switch {
  position: relative;
  display: inline-block;
  width: 58px;
  height: 32px;
  flex: 0 0 auto;
}

.switch input {
  opacity: 0;
  width: 0;
  height: 0;
}

.slider {
  position: absolute;
  cursor: pointer;
  inset: 0;
  background: var(--green);
  border-radius: 999px;
  transition: 0.2s;
}

.slider::before {
  content: "";
  position: absolute;
  height: 24px;
  width: 24px;
  left: 4px;
  bottom: 4px;
  background: var(--white);
  border-radius: 50%;
  transition: 0.2s;
}

.switch input:checked + .slider {
  background: var(--red);
}

.switch input:focus-visible + .slider {
  outline: 3px solid #2563eb;
  outline-offset: 3px;
}

#mode-link { display: inline-block; margin-top: 12px; color: inherit; }

.switch input:checked + .slider::before {
  transform: translateX(26px);
}

@media (max-width: 980px) {
  .parking-grid,
  .simulation-grid,
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
  .parking-grid,
  .simulation-grid {
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
)parkingasset";

const char SITE_DATA[] PROGMEM = R"parkingasset(window.ParkingData = (() => {
  const config = window.PARKING_CONFIG || {};
  const mode = new URLSearchParams(window.location.search).get("modo") === "demo" ? "demo" : config.mode;
  const key = "estacionamento-demo-v2";
  let memory;
  let firebaseApp;

  const validDate = value => typeof value === "string" && Number.isFinite(Date.parse(value));

  function initial() {
    return {
      status: {
        vagas: Array.from({ length: 4 }, (_, i) => ({ numero: i + 1, ocupada: false, entradaAtual: null })),
        ultimaAtualizacao: null
      },
      historico: []
    };
  }

  function normalize(data) {
    const vagas = data?.status?.vagas;
    if (!Array.isArray(vagas) || vagas.length !== 4 ||
        new Set(vagas.map(v => v.numero)).size !== 4 ||
        vagas.some(v => !Number.isInteger(v.numero) || v.numero < 1 || v.numero > 4 || typeof v.ocupada !== "boolean")) {
      throw Error("Aguardando dados válidos das quatro vagas.");
    }

    vagas.forEach(v => {
      v.entradaAtual = validDate(v.entradaAtual) ? v.entradaAtual : null;
    });
    vagas.sort((a, b) => a.numero - b.numero);

    const ocupadas = vagas.filter(v => v.ocupada).length;
    if (data.status.ultimaAtualizacao && !validDate(data.status.ultimaAtualizacao)) {
      throw Error("Horário inválido recebido da ESP32.");
    }
    Object.assign(data.status, {
      total: 4,
      ocupadas,
      livres: 4 - ocupadas,
      taxaOcupacao: ocupadas * 25
    });

    data.historico = (Array.isArray(data.historico) ? data.historico : [])
      .filter(e => Number.isInteger(e.vaga) && e.vaga >= 1 && e.vaga <= 4 && ["ENTRADA", "SAIDA"].includes(e.tipo))
      .slice(0, 100);

    data.historico.forEach(e => {
      for (const field of ["dataHora", "entrada", "saida"]) {
        e[field] = validDate(e[field]) ? e[field] : null;
      }
    });

    return data;
  }

  async function json(url, options = {}) {
    const response = await fetch(url, {
      ...options,
      cache: "no-store",
      signal: AbortSignal.timeout(7000)
    });

    if (!response.ok) {
      const details = await response.json().catch(() => ({}));
      const message = details.error?.message || "";
      if (response.status === 403) throw Error("Firebase: leitura não autorizada. Verifique as regras do Firestore.");
      if (response.status === 404) throw Error("Aguardando os registros das ESP32 no Firestore. Confira se o banco (default) foi criado.");
      throw Error(message || `Falha na leitura (${response.status}).`);
    }
    return response.json();
  }

  function fields(f = {}) {
    return Object.fromEntries(Object.entries(f).map(([k, v]) => [
      k,
      v.nullValue !== undefined ? null :
      v.integerValue !== undefined ? Number(v.integerValue) :
      v.booleanValue !== undefined ? v.booleanValue :
      v.timestampValue !== undefined ? v.timestampValue :
      v.doubleValue !== undefined ? Number(v.doubleValue) :
      v.stringValue !== undefined ? v.stringValue : null
    ]));
  }

  async function readDemo() {
    try {
      memory = normalize(JSON.parse(localStorage.getItem(key)) || memory || initial());
    } catch {
      memory = memory || initial();
    }
    return normalize(structuredClone(memory));
  }

  async function readFirebase() {
    if (!config.firebase?.projectId) throw Error("Preencha a configuração Firebase em config.js.");
    if (!firebaseApp) {
      try { firebaseApp = (await import('./firebase.js')).app; }
      catch { throw Error("Não foi possível inicializar o SDK Firebase. Verifique sua conexão."); }
    }

    const base = `https://firestore.googleapis.com/v1/projects/${encodeURIComponent(firebaseApp.options.projectId)}/databases/(default)/documents`;
    const [vagas, meta, eventos] = await Promise.all([
      Promise.all([1, 2, 3, 4].map(i => json(`${base}/vagas/vaga${i}`))),
      json(`${base}/metadata/status`),
      json(`${base}:runQuery`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          structuredQuery: {
            from: [{ collectionId: "eventos" }],
            orderBy: [{ field: { fieldPath: "dataHora" }, direction: "DESCENDING" }],
            limit: 100
          }
        })
      })
    ]);

    const vagaDocs = vagas.map(d => fields(d.fields));
    if (vagaDocs.length < 4) {
      throw Error("Firebase conectado, mas as quatro vagas ainda não foram registradas pela ESP32.");
    }

    const metadata = fields(meta.fields);
    if (metadata.sensoresOk === false) throw Error("ESP32 conectada, mas há falha nos sensores ou na comunicação UART.");
    return normalize({
      status: {
        vagas: vagaDocs,
        ultimaAtualizacao: metadata.ultimaAtualizacao || null
      },
      historico: eventos.filter(e => e.document).map(e => fields(e.document.fields))
    });
  }

  async function read() {
    if (mode === "demo") return readDemo();
    if (mode === "firebase") return readFirebase();
    if (mode === "esp32") {
      const base = (config.esp32Url || "").replace(/\/$/, "");
      const [status, historico] = await Promise.all([json(`${base}/api/vagas`), json(`${base}/api/historico`)]);
      return normalize({ status, historico });
    }
    throw Error("Modo inválido em config.js.");
  }

  async function update({ vaga, ocupada }) {
    if (mode !== "demo") throw Error("As mudanças reais são feitas pelos sensores da ESP32.");
    const data = await readDemo();
    const item = data.status.vagas.find(v => v.numero === vaga);
    if (!item || typeof ocupada !== "boolean") throw Error("Vaga inválida.");
    if (item.ocupada === ocupada) return data;

    const now = new Date().toISOString();
    data.historico.unshift({
      vaga,
      tipo: ocupada ? "ENTRADA" : "SAIDA",
      dataHora: now,
      entrada: ocupada ? now : item.entradaAtual,
      saida: ocupada ? null : now
    });
    Object.assign(item, { ocupada, entradaAtual: ocupada ? now : null });
    data.status.ultimaAtualizacao = now;
    memory = normalize(data);
    localStorage.setItem(key, JSON.stringify(memory));
    return structuredClone(memory);
  }

  return {
    mode,
    read,
    update
  };
})();
)parkingasset";

const char SITE_SCRIPT[] PROGMEM = R"parkingasset(const parkingMap = document.getElementById("parking-map");
const freeCount = document.getElementById("free-count");
const occupiedCount = document.getElementById("occupied-count");
const occupancyRate = document.getElementById("occupancy-rate");
const availableList = document.getElementById("available-list");
const availabilityMessage = document.getElementById("availability-message");
const historyBody = document.getElementById("history-body");
const lastUpdate = document.getElementById("last-update");
const connectionDot = document.getElementById("connection-dot");
const connectionText = document.getElementById("connection-text");
const databaseMode = document.getElementById("database-mode");
const realtimeMode = document.getElementById("realtime-mode");
const apiEndpoint = document.getElementById("api-endpoint");
const lcdFree = document.getElementById("lcd-free");
const lcdList = document.getElementById("lcd-list");

let currentStatus = null;
let timerId = null;
let pollingId = null;
let hardwareMode = false;
let loading = false;


function setConnectionState(isOnline, text) {
  connectionDot.classList.toggle("offline", !isOnline);
  connectionText.textContent = text;
}

function formatDateTime(isoString) {
  if (!isoString) {
    return {
      date: "--/--/----",
      time: "--:--:--"
    };
  }

  const date = new Date(isoString);
  return {
    date: date.toLocaleDateString("pt-BR"),
    time: date.toLocaleTimeString("pt-BR")
  };
}

function secondsBetween(startIso, endDate = new Date()) {
  if (!startIso) {
    return 0;
  }

  return Math.max(0, Math.floor((endDate - new Date(startIso)) / 1000));
}

function formatDuration(totalSeconds) {
  const seconds = Math.max(0, Number(totalSeconds || 0));
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const remainingSeconds = seconds % 60;

  return [hours, minutes, remainingSeconds]
    .map((value) => String(value).padStart(2, "0"))
    .join(":");
}

function formatVagasList(vagas) {
  if (!vagas.length) {
    return "--";
  }

  return vagas.join(", ");
}

function renderParkingMap(status) {
  parkingMap.innerHTML = status.vagas.map((vaga) => {
    const occupiedClass = vaga.ocupada ? "occupied" : "";
    const entrada = formatDateTime(vaga.entradaAtual);
    const duration = vaga.ocupada && vaga.entradaAtual ? formatDuration(secondsBetween(vaga.entradaAtual)) : "--:--:--";
    const details = vaga.ocupada
      ? `<div>Entrada: <strong>${entrada.time}</strong></div><div>Tempo estacionado: <strong data-duration="${vaga.numero}">${duration}</strong></div>`
      : "<div>Disponivel para entrada</div>";

    return `
      <article class="parking-card ${occupiedClass}" data-vaga="${vaga.numero}">
        <div class="parking-head">
          <div>
            <span class="label">Vaga</span>
            <div class="number">${vaga.numero}</div>
          </div>
          <span class="car-mark" aria-hidden="true">
            <span class="car-light"></span>
          </span>
        </div>
        <span class="sensor-mark" aria-hidden="true"></span>
        <div class="parking-details">${details}</div>
        <span class="status-pill">${vaga.ocupada ? "OCUPADA" : "LIVRE"}</span>
      </article>
    `;
  }).join("");
}

function renderIndicators(status) {
  freeCount.textContent = status.livres;
  occupiedCount.textContent = status.ocupadas;
  occupancyRate.textContent = `${status.taxaOcupacao}%`;
  lcdFree.textContent = `${status.livres} ${status.livres === 1 ? "livre" : "livres"}`;
  lcdList.textContent = `Vagas: ${formatVagasList(status.vagas.filter((vaga) => !vaga.ocupada).map((vaga) => vaga.numero))}`;
}

function renderAvailability(status) {
  const livres = status.vagas.filter((vaga) => !vaga.ocupada);

  if (livres.length === 0) {
    availabilityMessage.textContent = "ESTACIONAMENTO LOTADO";
    availabilityMessage.classList.add("full");
    availableList.innerHTML = "";
    return;
  }

  availabilityMessage.textContent = "Estacionamento com vagas disponiveis";
  availabilityMessage.classList.remove("full");
  availableList.innerHTML = livres
    .map((vaga) => `<span class="available-chip">Vaga ${vaga.numero}</span>`)
    .join("");
}

function renderHistory(history) {
  if (!history.length) {
    historyBody.innerHTML = '<tr><td colspan="7" class="empty-table">Nenhuma movimentacao registrada.</td></tr>';
    return;
  }

  historyBody.innerHTML = history.map((event) => {
    const dataHora = formatDateTime(event.dataHora);
    const entrada = formatDateTime(event.entrada);
    const saida = formatDateTime(event.saida);
    const isEntry = event.tipo === "ENTRADA";

    return `
      <tr>
        <td>${dataHora.date}</td>
        <td>${dataHora.time}</td>
        <td>Vaga ${event.vaga}</td>
        <td><span class="event-badge ${isEntry ? "entry" : "exit"}">${isEntry ? "Entrada" : "Saida"}</span></td>
        <td>${event.entrada ? entrada.time : "-"}</td>
        <td>${event.saida ? saida.time : "-"}</td>
        <td>${(event.saida && event.entrada ? formatDuration(secondsBetween(event.entrada, new Date(event.saida))) : "-")}</td>
      </tr>
    `;
  }).join("");
}

function updateRunningDurations() {
  if (!currentStatus) {
    return;
  }

  currentStatus.vagas
    .filter((vaga) => vaga.ocupada)
    .forEach((vaga) => {
      const element = document.querySelector(`[data-duration="${vaga.numero}"]`);
      if (element) {
        element.textContent = vaga.entradaAtual ? formatDuration(secondsBetween(vaga.entradaAtual)) : "--:--:--";
      }
    });
}

function renderAll(payload) {
  currentStatus = payload.status;
  setConnectionState(true, ParkingData.mode === "demo" ? "Demonstração local" : "Dados recebidos");
  renderParkingMap(payload.status);
  renderIndicators(payload.status);
  renderAvailability(payload.status);
  renderHistory(payload.historico || []);
  lastUpdate.textContent = formatDateTime(payload.status.ultimaAtualizacao).time;
  updateRunningDurations();
}


async function loadInitialData() {
  if (loading) return;
  loading = true;
  try {
    renderAll(await ParkingData.read());
    // O painel local usa a idade da leitura, inclusive antes do horario NTP.
    const leituraLocal = ParkingData.mode === "esp32" && Number.isFinite(currentStatus.idadeLeituraMs);
    const idade = leituraLocal ? currentStatus.idadeLeituraMs
      : currentStatus.ultimaAtualizacao ? Date.now() - new Date(currentStatus.ultimaAtualizacao).getTime() : Infinity;
    if (hardwareMode && idade > (leituraLocal ? 3000 : 30000)) {
      setConnectionState(false, "Sem leitura recente dos sensores");
      availabilityMessage.textContent = "Últimos dados recebidos — aguardando atualização dos sensores.";
      availableList.innerHTML = "";
      availabilityMessage.classList.remove("full");
    }
  } catch (error) {
    setConnectionState(false, "Sem conexão — dados indisponíveis ou desatualizados");
    availabilityMessage.textContent = error.message;
    availabilityMessage.classList.remove("full");
    availableList.innerHTML = "";
    currentStatus = null;
    [freeCount, occupiedCount, occupancyRate, lcdFree].forEach(el => el.textContent = "--");
    lcdList.textContent = "Vagas: --";
    parkingMap.textContent = "Aguardando leitura válida das quatro vagas.";
  } finally { loading = false; }
}
hardwareMode = ParkingData.mode !== "demo";
const modeLink = document.getElementById("mode-link");
modeLink.href = hardwareMode ? "?modo=demo" : window.location.pathname;
modeLink.textContent = hardwareMode ? "Testar demonstração local" : "Voltar ao painel conectado";

databaseMode.textContent = hardwareMode ? (ParkingData.mode === "esp32" ? "Servidor: ESP32 local" : "Banco: Firebase") : "Dados: neste navegador";
realtimeMode.textContent = hardwareMode ? "Atualização: a cada 3 segundos" : "Simulação local";
apiEndpoint.textContent = hardwareMode ? (ParkingData.mode === "esp32" ? "/api/vagas" : "Cloud Firestore REST") : "LocalStorage";
document.getElementById("endpoint-method").textContent = hardwareMode ? (ParkingData.mode === "esp32" ? "GET" : "FIREBASE") : "DEMO";
document.getElementById("hardware-status").textContent = hardwareMode
  ? "A ESP32 dos sensores envia as leituras por UART à ESP32 dos LEDs/LCD. O painel acompanha as quatro vagas."
  : "Modo demonstração local. Os dados ficam somente neste navegador.";

loadInitialData();
timerId = window.setInterval(updateRunningDurations, 1000);
if (hardwareMode) pollingId = window.setInterval(() => { if (!document.hidden) loadInitialData(); }, 3000);
document.addEventListener("visibilitychange", () => { if (!document.hidden) loadInitialData(); });
window.addEventListener("storage", () => { if (!hardwareMode) loadInitialData(); });
window.addEventListener("beforeunload", () => { clearInterval(timerId); clearInterval(pollingId); });
)parkingasset";
