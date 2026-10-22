// Painel do estacionamento servido pela ESP32.
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
