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
    const rows = await db.all("SELECT * FROM vagas ORDER BY numero ASC");
    return rows.map(mapVaga);
  }

  async function getStatus() {
    const vagas = await getVagas();
    const ocupadas = vagas.filter((vaga) => vaga.ocupada).length;
    const livres = TOTAL_VAGAS - ocupadas;
    const taxaOcupacao = Math.round((ocupadas / TOTAL_VAGAS) * 100);
    const metadata = await db.get("SELECT valor FROM metadata WHERE chave = 'last_update'");
    const ultimaAtualizacao = metadata?.valor || null;

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
    const rows = await db.all("SELECT * FROM eventos ORDER BY data_hora DESC, id DESC LIMIT 100");
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
    const vagaAtual = await db.get("SELECT * FROM vagas WHERE numero = ?", update.vaga);

    if (!vagaAtual) {
      throw createHttpError(404, "Vaga nao encontrada.");
    }

    const ocupadaAtual = Boolean(vagaAtual.ocupada);
    if (ocupadaAtual === update.ocupada) {
      await db.run(
        "UPDATE metadata SET valor = ? WHERE chave = 'last_update'",
        update.timestamp
      );
      await db.run(
        "UPDATE vagas SET ultima_atualizacao = ? WHERE numero = ?",
        update.timestamp,
        update.vaga
      );
      return {
        changed: false,
        event: null
      };
    }

    const event = await db.transaction(async (tx) => {
      await tx.run(
        "UPDATE metadata SET valor = ? WHERE chave = 'last_update'",
        update.timestamp
      );

      if (update.ocupada) {
        await tx.run(
          "UPDATE vagas SET ocupada = 1, ultima_atualizacao = ?, entrada_atual = ? WHERE numero = ?",
          update.timestamp,
          update.timestamp,
          update.vaga
        );

        const result = await tx.run(
          "INSERT INTO eventos (vaga, tipo, data_hora, entrada, saida, duracao_segundos, duracao_texto) VALUES (?, 'ENTRADA', ?, ?, NULL, NULL, NULL)",
          update.vaga,
          update.timestamp,
          update.timestamp
        );

        return mapEvento(await tx.get("SELECT * FROM eventos WHERE id = ?", result.lastID));
      } else {
        const entrada = vagaAtual.entrada_atual || update.timestamp;
        const duracaoSegundos = Math.max(0, Math.floor((new Date(update.timestamp) - new Date(entrada)) / 1000));
        const duracaoTexto = formatDuration(duracaoSegundos);

        await tx.run(
          "UPDATE vagas SET ocupada = 0, ultima_atualizacao = ?, entrada_atual = NULL WHERE numero = ?",
          update.timestamp,
          update.vaga
        );

        const result = await tx.run(
          "INSERT INTO eventos (vaga, tipo, data_hora, entrada, saida, duracao_segundos, duracao_texto) VALUES (?, 'SAIDA', ?, ?, ?, ?, ?)",
          update.vaga,
          update.timestamp,
          entrada,
          update.timestamp,
          duracaoSegundos,
          duracaoTexto
        );

        return mapEvento(await tx.get("SELECT * FROM eventos WHERE id = ?", result.lastID));
      }
    });

    return {
      changed: true,
      event
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
