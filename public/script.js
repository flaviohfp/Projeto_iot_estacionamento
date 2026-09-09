const parkingMap = document.getElementById("parking-map");
const simulationControls = document.getElementById("simulation-controls");
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
const oledFree = document.getElementById("oled-free");
const oledList = document.getElementById("oled-list");

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
    const duration = vaga.ocupada ? formatDuration(secondsBetween(vaga.entradaAtual)) : null;
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
  oledFree.textContent = `${status.livres} ${status.livres === 1 ? "livre" : "livres"}`;
  oledList.textContent = `Vagas: ${formatVagasList(status.vagas.filter((vaga) => !vaga.ocupada).map((vaga) => vaga.numero))}`;
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

function renderSimulation(status) {
  if (hardwareMode) {
    simulationControls.textContent = "Modo físico: aproxime ou retire um veículo dos sensores da maquete. LEDs e OLED acompanham as leituras automaticamente.";
    return;
  }
  simulationControls.innerHTML = status.vagas.map((vaga) => `
    <div class="simulation-control">
      <div>
        <strong>Vaga ${vaga.numero}</strong>
        <small>${vaga.ocupada ? "Ocupada" : "Livre"}</small>
      </div>
      <label class="switch" title="Alternar estado da vaga ${vaga.numero}">
        <input type="checkbox" data-sim-vaga="${vaga.numero}" ${vaga.ocupada ? "checked" : ""}>
        <span class="slider"></span>
      </label>
    </div>
  `).join("");

  simulationControls.querySelectorAll("input[data-sim-vaga]").forEach((input) => {
    input.addEventListener("change", async (event) => {
      const vaga = Number(event.target.dataset.simVaga);
      const ocupada = event.target.checked;
      event.target.disabled = true;

      try {
        const data = await ParkingData.update({ vaga, ocupada });
        renderAll({
          status: data.status,
          historico: data.historico || []
        });
      } catch (error) {
        alert(error.message);
        event.target.checked = !ocupada;
      } finally {
        event.target.disabled = false;
      }
    });
  });
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
        <td>${(event.saida ? formatDuration(secondsBetween(event.entrada, new Date(event.saida))) : "-")}</td>
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
        element.textContent = formatDuration(secondsBetween(vaga.entradaAtual));
      }
    });
}

function renderAll(payload) {
  currentStatus = payload.status;
  setConnectionState(true, ParkingData.mode === "demo" ? "Demonstração local" : "Dados recebidos");
  renderParkingMap(payload.status);
  renderIndicators(payload.status);
  renderAvailability(payload.status);
  renderSimulation(payload.status);
  renderHistory(payload.historico || []);
  lastUpdate.textContent = formatDateTime(payload.status.ultimaAtualizacao).time;
  updateRunningDurations();
}


async function loadInitialData() {
  if (loading) return;
  loading = true;
  try {
    renderAll(await ParkingData.read());
    if (hardwareMode && (!currentStatus.ultimaAtualizacao || Date.now() - new Date(currentStatus.ultimaAtualizacao).getTime() > 30000)) {
      setConnectionState(false, "Sem leitura recente dos sensores");
      availabilityMessage.textContent = "Últimos dados recebidos — aguardando atualização dos sensores.";
      availableList.innerHTML = "";
    }
  } catch (error) {
    setConnectionState(false, "Sem conexão — dados indisponíveis ou desatualizados");
    availabilityMessage.textContent = error.message;
    availableList.innerHTML = "";
    if (!currentStatus) {
      [freeCount, occupiedCount, occupancyRate, oledFree].forEach(el => el.textContent = "--");
      parkingMap.textContent = "Aguardando dados das quatro vagas.";
    }
  } finally { loading = false; }
}
hardwareMode = ParkingData.mode !== "demo";
databaseMode.textContent = hardwareMode ? (ParkingData.mode === "firebase" ? "Banco: Firebase" : "Servidor: ESP32") : "Dados: neste navegador";
realtimeMode.textContent = hardwareMode ? "Atualização: a cada 3 segundos" : "Simulação local";
apiEndpoint.textContent = ParkingData.mode === "esp32" ? (window.PARKING_CONFIG.esp32Url || "Mesmo endereço do site") : ParkingData.mode === "firebase" ? "Cloud Firestore" : "Integração futura";
document.getElementById("endpoint-method").textContent = hardwareMode ? "GET" : "DEMO";
document.getElementById("hardware-status").textContent = hardwareMode ? "Painel de leitura. Os registros devem ser enviados pela ESP32." : "ESP32 ainda não integrada. Use os controles abaixo para testar o painel.";
loadInitialData();
timerId = window.setInterval(updateRunningDurations, 1000);
if (hardwareMode) pollingId = window.setInterval(loadInitialData, 3000);
window.addEventListener("storage", () => { if (!hardwareMode) loadInitialData(); });
window.addEventListener("beforeunload", () => { clearInterval(timerId); clearInterval(pollingId); });
