const parkingMap = document.getElementById("parking-map");
const simulationControls = document.getElementById("simulation-controls");
const freeCount = document.getElementById("free-count");
const occupiedCount = document.getElementById("occupied-count");
const occupancyRate = document.getElementById("occupancy-rate");
const availableList = document.getElementById("available-list");
const availabilityMessage = document.getElementById("availability-message");
const historyBody = document.getElementById("history-body");
const lastUpdate = document.getElementById("last-update");

let currentStatus = null;
let timerId = null;
let pollingId = null;

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
        <div>
          <span class="label">Vaga</span>
          <div class="number">${vaga.numero}</div>
          <span class="car-mark" aria-hidden="true"></span>
        </div>
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
        const response = await fetch("/api/vagas/status", {
          method: "POST",
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify({
            vaga,
            ocupada
          })
        });

        if (!response.ok) {
          const data = await response.json().catch(() => ({}));
          throw new Error(data.error || "Falha ao atualizar vaga.");
        }

        const data = await response.json();
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
        <td>${event.duracaoTexto || "-"}</td>
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
  renderParkingMap(payload.status);
  renderIndicators(payload.status);
  renderAvailability(payload.status);
  renderSimulation(payload.status);
  renderHistory(payload.historico || []);
  lastUpdate.textContent = formatDateTime(payload.status.ultimaAtualizacao).time;
  updateRunningDurations();
}

async function loadInitialData() {
  const [statusResponse, historyResponse] = await Promise.all([
    fetch("/api/vagas"),
    fetch("/api/historico")
  ]);

  if (!statusResponse.ok || !historyResponse.ok) {
    throw new Error("Nao foi possivel carregar os dados do estacionamento.");
  }

  renderAll({
    status: await statusResponse.json(),
    historico: await historyResponse.json()
  });
}

function setupRealtime() {
  const socketScript = document.createElement("script");
  socketScript.src = "/socket.io/socket.io.js";
  socketScript.onload = () => {
    if (!window.io) {
      return;
    }

    const socket = window.io();
    socket.on("parking:update", renderAll);
  };
  document.head.appendChild(socketScript);
}

loadInitialData().catch((error) => {
  availabilityMessage.textContent = error.message;
  availabilityMessage.classList.add("full");
});

setupRealtime();
timerId = window.setInterval(updateRunningDurations, 1000);
pollingId = window.setInterval(loadInitialData, 3000);

window.addEventListener("beforeunload", () => {
  window.clearInterval(timerId);
  window.clearInterval(pollingId);
});
