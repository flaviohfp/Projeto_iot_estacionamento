window.ParkingData = (() => {
  const config = window.PARKING_CONFIG || {};
  const mode = new URLSearchParams(window.location.search).get("modo") === "demo" ? "demo" : config.mode;
  const key = "estacionamento-demo-v2";
  let memory;

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
      if (response.status === 404) throw Error("Firestore não encontrado. Confira o projeto e se o banco foi criado.");
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
    if (!config.firebaseProjectId) throw Error("Preencha firebaseProjectId em config.js.");

    const base = `https://firestore.googleapis.com/v1/projects/${encodeURIComponent(config.firebaseProjectId)}/databases/(default)/documents`;
    const [vagas, meta, eventos] = await Promise.all([
      json(`${base}/vagas?pageSize=4`),
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

    const vagaDocs = (vagas.documents || []).map(d => fields(d.fields));
    if (vagaDocs.length < 4) {
      throw Error("Firebase conectado, mas as quatro vagas ainda não foram registradas pela ESP32.");
    }

    return normalize({
      status: {
        vagas: vagaDocs,
        ultimaAtualizacao: fields(meta.fields).ultimaAtualizacao || null
      },
      historico: eventos.filter(e => e.document).map(e => fields(e.document.fields))
    });
  }

  async function read() {
    if (mode === "demo") return readDemo();
    if (mode === "firebase") return readFirebase();
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
