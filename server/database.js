const admin = require("firebase-admin");

const TOTAL_VAGAS = 4;

function createInitialLocalState() {
  return {
    vagas: Array.from({ length: TOTAL_VAGAS }, (_, index) => ({
      numero: index + 1,
      ocupada: false,
      ultima_atualizacao: null,
      entrada_atual: null
    })),
    eventos: [],
    ultimaAtualizacao: null,
    nextEventId: 1
  };
}

function createLocalDatabase() {
  const state = createInitialLocalState();

  function clone(value) {
    return JSON.parse(JSON.stringify(value));
  }

  return {
    kind: "local-memory",
    async getVagas() {
      return clone(state.vagas);
    },
    async getUltimaAtualizacao() {
      return state.ultimaAtualizacao;
    },
    async getHistorico(limit = 100) {
      return clone(state.eventos.slice(0, limit));
    },
    async registrarStatus({ vaga, ocupada, timestamp, formatDuration }) {
      const vagaAtual = state.vagas.find((item) => item.numero === vaga);

      if (!vagaAtual) {
        const error = new Error("Vaga nao encontrada.");
        error.statusCode = 404;
        throw error;
      }

      state.ultimaAtualizacao = timestamp;

      if (Boolean(vagaAtual.ocupada) === ocupada) {
        vagaAtual.ultima_atualizacao = timestamp;
        return {
          changed: false,
          event: null
        };
      }

      const eventId = String(state.nextEventId);
      state.nextEventId += 1;

      if (ocupada) {
        const event = {
          id: eventId,
          vaga,
          tipo: "ENTRADA",
          data_hora: timestamp,
          entrada: timestamp,
          saida: null,
          duracao_segundos: null,
          duracao_texto: null
        };

        vagaAtual.ocupada = true;
        vagaAtual.ultima_atualizacao = timestamp;
        vagaAtual.entrada_atual = timestamp;
        state.eventos.unshift(event);

        return {
          changed: true,
          event: clone(event)
        };
      }

      const entrada = vagaAtual.entrada_atual || timestamp;
      const duracaoSegundos = Math.max(0, Math.floor((new Date(timestamp) - new Date(entrada)) / 1000));
      const duracaoTexto = formatDuration(duracaoSegundos);
      const event = {
        id: eventId,
        vaga,
        tipo: "SAIDA",
        data_hora: timestamp,
        entrada,
        saida: timestamp,
        duracao_segundos: duracaoSegundos,
        duracao_texto: duracaoTexto
      };

      vagaAtual.ocupada = false;
      vagaAtual.ultima_atualizacao = timestamp;
      vagaAtual.entrada_atual = null;
      state.eventos.unshift(event);

      return {
        changed: true,
        event: clone(event)
      };
    }
  };
}

function parseServiceAccount() {
  if (process.env.FIREBASE_SERVICE_ACCOUNT_BASE64) {
    const json = Buffer.from(process.env.FIREBASE_SERVICE_ACCOUNT_BASE64, "base64").toString("utf8");
    return JSON.parse(json);
  }

  if (process.env.FIREBASE_SERVICE_ACCOUNT) {
    return JSON.parse(process.env.FIREBASE_SERVICE_ACCOUNT);
  }

  if (
    process.env.FIREBASE_PROJECT_ID &&
    process.env.FIREBASE_CLIENT_EMAIL &&
    process.env.FIREBASE_PRIVATE_KEY
  ) {
    return {
      project_id: process.env.FIREBASE_PROJECT_ID,
      client_email: process.env.FIREBASE_CLIENT_EMAIL,
      private_key: process.env.FIREBASE_PRIVATE_KEY.replace(/\\n/g, "\n")
    };
  }

  return null;
}

function initializeFirebaseApp() {
  if (admin.apps.length) {
    return admin.app();
  }

  const serviceAccount = parseServiceAccount();

  if (!serviceAccount) {
    throw new Error(
      "Firebase nao configurado. Defina FIREBASE_PROJECT_ID, FIREBASE_CLIENT_EMAIL e FIREBASE_PRIVATE_KEY na Vercel."
    );
  }

  return admin.initializeApp({
    credential: admin.credential.cert(serviceAccount)
  });
}

function mapVagaDoc(doc) {
  const data = doc.data() || {};
  return {
    numero: Number(data.numero || doc.id),
    ocupada: Boolean(data.ocupada),
    ultima_atualizacao: data.ultimaAtualizacao || null,
    entrada_atual: data.entradaAtual || null
  };
}

function mapEventoDoc(doc) {
  const data = doc.data() || {};
  return {
    id: doc.id,
    vaga: data.vaga,
    tipo: data.tipo,
    data_hora: data.dataHora,
    entrada: data.entrada || null,
    saida: data.saida || null,
    duracao_segundos: data.duracaoSegundos ?? null,
    duracao_texto: data.duracaoTexto || null
  };
}

async function initializeDatabase() {
  if (String(process.env.USE_LOCAL_DATABASE || "false").toLowerCase() === "true") {
    return createLocalDatabase();
  }

  const hasFirebaseConfig = Boolean(parseServiceAccount());
  if (!hasFirebaseConfig) {
    return createLocalDatabase();
  }

  initializeFirebaseApp();
  const firestore = admin.firestore();

  const vagas = firestore.collection("vagas");
  const metadata = firestore.collection("metadata");

  const batch = firestore.batch();

  for (let numero = 1; numero <= TOTAL_VAGAS; numero += 1) {
    const ref = vagas.doc(String(numero));
    const snapshot = await ref.get();

    if (!snapshot.exists) {
      batch.set(ref, {
        numero,
        ocupada: false,
        ultimaAtualizacao: null,
        entradaAtual: null
      });
    }
  }

  const statusRef = metadata.doc("status");
  const statusSnapshot = await statusRef.get();
  if (!statusSnapshot.exists) {
    batch.set(statusRef, {
      ultimaAtualizacao: null
    });
  }

  await batch.commit();

  return {
    kind: "firebase",
    firestore,
    async getVagas() {
      const snapshot = await vagas.orderBy("numero", "asc").get();
      return snapshot.docs.map(mapVagaDoc);
    },
    async getUltimaAtualizacao() {
      const snapshot = await statusRef.get();
      return snapshot.exists ? snapshot.data().ultimaAtualizacao || null : null;
    },
    async getHistorico(limit = 100) {
      const snapshot = await firestore
        .collection("eventos")
        .orderBy("dataHora", "desc")
        .limit(limit)
        .get();

      return snapshot.docs.map(mapEventoDoc);
    },
    async registrarStatus({ vaga, ocupada, timestamp, formatDuration }) {
      const vagaRef = vagas.doc(String(vaga));
      const eventoRef = firestore.collection("eventos").doc();

      return firestore.runTransaction(async (transaction) => {
        const vagaSnapshot = await transaction.get(vagaRef);

        if (!vagaSnapshot.exists) {
          const error = new Error("Vaga nao encontrada.");
          error.statusCode = 404;
          throw error;
        }

        const vagaAtual = mapVagaDoc(vagaSnapshot);
        if (Boolean(vagaAtual.ocupada) === ocupada) {
          transaction.set(statusRef, { ultimaAtualizacao: timestamp }, { merge: true });
          transaction.set(vagaRef, { ultimaAtualizacao: timestamp }, { merge: true });

          return {
            changed: false,
            event: null
          };
        }

        if (ocupada) {
          const event = {
            id: eventoRef.id,
            vaga,
            tipo: "ENTRADA",
            data_hora: timestamp,
            entrada: timestamp,
            saida: null,
            duracao_segundos: null,
            duracao_texto: null
          };

          transaction.set(statusRef, { ultimaAtualizacao: timestamp }, { merge: true });
          transaction.set(vagaRef, {
            numero: vaga,
            ocupada: true,
            ultimaAtualizacao: timestamp,
            entradaAtual: timestamp
          }, { merge: true });
          transaction.set(eventoRef, {
            vaga,
            tipo: "ENTRADA",
            dataHora: timestamp,
            entrada: timestamp,
            saida: null,
            duracaoSegundos: null,
            duracaoTexto: null
          });

          return {
            changed: true,
            event
          };
        }

        const entrada = vagaAtual.entrada_atual || timestamp;
        const duracaoSegundos = Math.max(0, Math.floor((new Date(timestamp) - new Date(entrada)) / 1000));
        const duracaoTexto = formatDuration(duracaoSegundos);
        const event = {
          id: eventoRef.id,
          vaga,
          tipo: "SAIDA",
          data_hora: timestamp,
          entrada,
          saida: timestamp,
          duracao_segundos: duracaoSegundos,
          duracao_texto: duracaoTexto
        };

        transaction.set(statusRef, { ultimaAtualizacao: timestamp }, { merge: true });
        transaction.set(vagaRef, {
          numero: vaga,
          ocupada: false,
          ultimaAtualizacao: timestamp,
          entradaAtual: null
        }, { merge: true });
        transaction.set(eventoRef, {
          vaga,
          tipo: "SAIDA",
          dataHora: timestamp,
          entrada,
          saida: timestamp,
          duracaoSegundos,
          duracaoTexto
        });

        return {
          changed: true,
          event
        };
      });
    }
  };
}

module.exports = {
  initializeDatabase
};
