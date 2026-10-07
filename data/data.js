window.ParkingData = (() => {
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
