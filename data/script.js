// Painel do estacionamento servido pela ESP32.
// A ESP32 deve responder GET /api/vagas com:
// {"vagas":[{"numero":1,"ocupada":true,"segundos":42}, ...]}
// "segundos" = tempo que a vaga está ocupada (calculado com millis() na ESP32).
const API_URL = "/api/vagas";
const INTERVALO_MS = 2000;
const TOTAL_VAGAS = 4;

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
const lcdFree = $("lcd-free");
const lcdList = $("lcd-list");

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
  if (!Array.isArray(lista) || lista.length !== TOTAL_VAGAS) {
    throw new Error("Resposta inválida da ESP32.");
  }
  return lista
    .map(v => ({
      numero: Number(v.numero),
      ocupada: v.ocupada === true,
      segundos: Number.isFinite(Number(v.segundos)) ? Number(v.segundos) : null
    }))
    .sort((a, b) => a.numero - b.numero);
}

function renderParkingMap() {
  parkingMap.innerHTML = vagas.map(vaga => {
    const details = vaga.ocupada
      ? `<div>Tempo estacionado: <strong data-duration="${vaga.numero}">--:--:--</strong></div>`
      : "<div>Disponível para entrada</div>";

    return `
      <article class="parking-card ${vaga.ocupada ? "occupied" : ""}">
        <div class="parking-head">
          <div>
            <span class="label">Vaga</span>
            <div class="number">${vaga.numero}</div>
          </div>
          <span class="car-mark" aria-hidden="true"><span class="car-light"></span></span>
        </div>
        <span class="sensor-mark" aria-hidden="true"></span>
        <div class="parking-details">${details}</div>
        <span class="status-pill">${vaga.ocupada ? "OCUPADA" : "LIVRE"}</span>
      </article>
    `;
  }).join("");
}

function renderIndicators() {
  const livres = vagas.filter(v => !v.ocupada);
  const ocupadas = TOTAL_VAGAS - livres.length;

  freeCount.textContent = livres.length;
  occupiedCount.textContent = ocupadas;
  occupancyRate.textContent = `${Math.round((ocupadas / TOTAL_VAGAS) * 100)}%`;
  lcdFree.textContent = `${livres.length} ${livres.length === 1 ? "livre" : "livres"}`;
  lcdList.textContent = `Vagas: ${livres.length ? livres.map(v => v.numero).join(", ") : "--"}`;

  if (!livres.length) {
    availabilityMessage.textContent = "ESTACIONAMENTO LOTADO";
    availabilityMessage.classList.add("full");
    availableList.innerHTML = "";
    return;
  }
  availabilityMessage.textContent = "Estacionamento com vagas disponíveis";
  availabilityMessage.classList.remove("full");
  availableList.innerHTML = livres
    .map(v => `<span class="available-chip">Vaga ${v.numero}</span>`)
    .join("");
}

function updateRunningDurations() {
  if (!vagas) return;
  vagas.filter(v => v.ocupada).forEach(vaga => {
    const el = document.querySelector(`[data-duration="${vaga.numero}"]`);
    if (el) el.textContent = vaga.segundos === null ? "--:--:--" : formatDuration(segundosOcupada(vaga));
  });
}

function renderError(message) {
  vagas = null;
  setConnectionState(false, "Sem conexão com a ESP32");
  availabilityMessage.textContent = message;
  availabilityMessage.classList.remove("full");
  availableList.innerHTML = "";
  [freeCount, occupiedCount, occupancyRate, lcdFree].forEach(el => { el.textContent = "--"; });
  lcdList.textContent = "Vagas: --";
  parkingMap.textContent = "Aguardando leitura das quatro vagas.";
}

async function carregar() {
  if (carregando) return;
  carregando = true;
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 5000);
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
