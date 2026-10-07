// node --experimental-vm-modules --test tests/data.test.cjs
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

async function provider(mode, response) {
  const calls = [];
  let saved;
  const context = vm.createContext({
    window: { PARKING_CONFIG: { mode, firebase: { projectId: 'iotestacionamento-e2b70' } }, location: { search: '' } },
    URLSearchParams, AbortSignal, structuredClone,
    localStorage: { getItem: () => saved || null, setItem: (_, value) => { saved = value; } },
    fetch: async url => { calls.push(url); return response(url); }
  });
  const sdk = new vm.SyntheticModule(['app'], function () {
    this.setExport('app', { options: { projectId: 'iotestacionamento-e2b70' } });
  }, { context });
  await sdk.link(() => {});
  await sdk.evaluate();
  new vm.Script(fs.readFileSync('data/data.js', 'utf8'), {
    importModuleDynamically: async () => sdk
  }).runInContext(context);
  return { data: context.window.ParkingData, calls };
}

const now = new Date().toISOString();
const status = { ultimaAtualizacao: now, vagas: [1, 2, 3, 4].map(numero => ({ numero, ocupada: numero === 2, entradaAtual: numero === 2 ? now : null })) };
const ok = data => ({ ok: true, status: 200, json: async () => data });
const firestoreFields = object => Object.fromEntries(Object.entries(object).map(([name, value]) => [name,
  value === null ? { nullValue: null } : typeof value === 'boolean' ? { booleanValue: value } :
  typeof value === 'number' ? { integerValue: String(value) } : { stringValue: value }
]));

test('painel local usa a API e calcula ocupacao sem consultar Firebase', async () => {
  const p = await provider('esp32', url => ok(url.endsWith('/api/vagas') ? status : []));
  const result = await p.data.read();
  assert.equal(result.status.livres, 3);
  assert.equal(result.status.taxaOcupacao, 25);
  assert.deepEqual(p.calls.sort(), ['/api/historico', '/api/vagas']);
  await assert.rejects(p.data.update({ vaga: 1, ocupada: true }), /sensores/);
});

test('falha do sensor na API local nao vira vaga livre', async () => {
  const p = await provider('esp32', url => url.endsWith('/api/vagas') ? {
    ok: false, status: 503, json: async () => ({ error: { message: 'Falha UART' } })
  } : ok([]));
  await assert.rejects(p.data.read(), /Falha UART/);
});

test('painel local recebe idade da leitura mesmo antes da sincronizacao NTP', async () => {
  const p = await provider('esp32', url => ok(url.endsWith('/api/vagas')
    ? { ...structuredClone(status), ultimaAtualizacao: null, idadeLeituraMs: 120 } : []));
  const result = await p.data.read();
  assert.equal(result.status.ultimaAtualizacao, null);
  assert.equal(result.status.idadeLeituraMs, 120);
  assert.equal(result.status.livres, 3);
});

test('Firestore le os quatro documentos canonicos e conserva horarios UTC', async () => {
  const p = await provider('firebase', url => {
    const match = url.match(/\/vagas\/vaga([1-4])$/);
    if (match) return ok({ fields: firestoreFields(status.vagas[Number(match[1]) - 1]) });
    if (url.endsWith('/metadata/status')) return ok({ fields: firestoreFields({ ultimaAtualizacao: now, sensoresOk: true }) });
    return ok([]);
  });
  const result = await p.data.read();
  assert.equal(result.status.ocupadas, 1);
  assert.equal(result.status.vagas[1].entradaAtual, now);
  assert.equal(p.calls.filter(url => /\/vagas\/vaga[1-4]$/.test(url)).length, 4);
});

test('sensoresOk=false bloqueia a apresentacao de ocupacao antiga no remoto', async () => {
  const p = await provider('firebase', url => {
    const match = url.match(/\/vagas\/vaga([1-4])$/);
    if (match) return ok({ fields: firestoreFields(status.vagas[Number(match[1]) - 1]) });
    if (url.endsWith('/metadata/status')) return ok({ fields: firestoreFields({ ultimaAtualizacao: now, sensoresOk: false }) });
    return ok([]);
  });
  await assert.rejects(p.data.read(), /falha nos sensores/);
});

test('demonstracao guarda entrada e saida somente no navegador', async () => {
  const p = await provider('demo', () => { throw Error('A demo nao deve acessar rede'); });
  const entry = await p.data.update({ vaga: 1, ocupada: true });
  const exit = await p.data.update({ vaga: 1, ocupada: false });
  assert.equal(entry.status.ocupadas, 1);
  assert.equal(exit.status.livres, 4);
  assert.equal(exit.historico[0].tipo, 'SAIDA');
  assert.equal(exit.historico[0].entrada, entry.historico[0].entrada);
  assert.equal(p.calls.length, 0);
});
