const TOTAL_VAGAS = 4;

function createHttpError(statusCode, message) {
  const error = new Error(message);
  error.statusCode = statusCode;
  return error;
}

function parseTimestamp(timestamp) {
  if (!timestamp) {
    return new Date();
  }

  const parsed = new Date(timestamp);
  if (Number.isNaN(parsed.getTime())) {
    throw createHttpError(400, "timestamp invalido. Use um formato ISO, por exemplo: 2026-08-11T14:32:00.");
  }

  return parsed;
}

function validatePayload(payload) {
  const vaga = Number(payload.vaga);
  if (!Number.isInteger(vaga) || vaga < 1 || vaga > TOTAL_VAGAS) {
    throw createHttpError(400, "Vaga invalida. Use somente os numeros 1, 2, 3 ou 4.");
  }

  if (typeof payload.ocupada !== "boolean") {
    throw createHttpError(400, "Status invalido. O campo ocupada deve ser booleano: true ou false.");
  }

  return {
    vaga,
    ocupada: payload.ocupada,
    timestamp: parseTimestamp(payload.timestamp).toISOString()
  };
}

function formatDuration(totalSeconds) {
  if (totalSeconds == null) {
    return null;
  }

  const seconds = Math.max(0, Number(totalSeconds));
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const remainingSeconds = seconds % 60;

  return [hours, minutes, remainingSeconds]
    .map((value) => String(value).padStart(2, "0"))
    .join(":");
}

function mapVaga(row) {
  return {
    numero: row.numero,
    ocupada: Boolean(row.ocupada),
    ultimaAtualizacao: row.ultima_atualizacao,
    entradaAtual: row.entrada_atual
  };
}

function mapEvento(row) {
  return {
    id: row.id,
    vaga: row.vaga,
    tipo: row.tipo,
    dataHora: row.data_hora,
    entrada: row.entrada,
    saida: row.saida,
    duracaoSegundos: row.duracao_segundos,
    duracaoTexto: row.duracao_texto
  };
}

function createParkingService(db) {
  async function getVagas() {
    const rows = await db.getVagas();
    return rows.map(mapVaga);
  }

  async function getStatus() {
    const vagas = await getVagas();
    const ocupadas = vagas.filter((vaga) => vaga.ocupada).length;
    const livres = TOTAL_VAGAS - ocupadas;
    const taxaOcupacao = Math.round((ocupadas / TOTAL_VAGAS) * 100);
    const ultimaAtualizacao = await db.getUltimaAtualizacao();

    return {
      total: TOTAL_VAGAS,
      livres,
      ocupadas,
      taxaOcupacao,
      ultimaAtualizacao,
      vagas
    };
  }

  async function getDisplayStatus() {
    const vagas = await getVagas();
    const vagasLivres = vagas
      .filter((vaga) => !vaga.ocupada)
      .map((vaga) => vaga.numero);

    return {
      livres: vagasLivres.length,
      vagasLivres
    };
  }

  async function getHistorico() {
    const rows = await db.getHistorico(100);
    return rows.map(mapEvento);
  }

  async function getSnapshot() {
    return {
      status: await getStatus(),
      historico: await getHistorico()
    };
  }

  async function updateVagaStatus(payload) {
    const update = validatePayload(payload);
    const result = await db.registrarStatus({
      vaga: update.vaga,
      ocupada: update.ocupada,
      timestamp: update.timestamp,
      formatDuration
    });

    return {
      changed: result.changed,
      event: result.event ? mapEvento(result.event) : null
    };
  }

  return {
    getStatus,
    getDisplayStatus,
    getHistorico,
    getSnapshot,
    updateVagaStatus
  };
}

module.exports = {
  createParkingService
};
