const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const zlib = require('node:zlib');
const path = require('node:path');
const root = path.resolve(__dirname, '..');

test('firmware incorpora os três arquivos atuais do painel sem perda', () => {
  const header = fs.readFileSync(path.join(root, 'arduino/EstacionamentoServidor/web_assets.h'), 'utf8');
  for (const [name, file] of [['INDEX', 'index.html'], ['STYLE', 'style.css'], ['SCRIPT', 'script.js']]) {
    const bytes = header.match(new RegExp(`ASSET_${name}\\[\\] PROGMEM = \\{([^}]+)\\}`))[1];
    assert.deepEqual(zlib.gunzipSync(Buffer.from(bytes.split(',').map(Number))), fs.readFileSync(path.join(root, 'public', file)));
  }
});

test('painel reconhece hardware, bloqueia simulador e marca falha de conexão', async () => {
  const elements = new Map();
  const element = id => {
    if (!elements.has(id)) elements.set(id, { textContent: '', innerHTML: '', classList: { toggle() {}, add() {}, remove() {} }, querySelectorAll: () => [] });
    return elements.get(id);
  };
  let offline = false;
  const context = vm.createContext({
    document: { getElementById: element, querySelector: () => null, createElement: () => { throw Error('Socket.IO não deve carregar no ESP32'); } },
    window: { location: { origin: 'http://192.168.4.1' }, setInterval() {}, addEventListener() {} },
    AbortSignal, console,
    fetch: async url => {
      if (offline) throw Error('offline');
      const body = url.endsWith('health') ? { hardware: true, sensorsReady: true, realtime: 'polling', database: 'ESP32' }
        : url.endsWith('historico') ? [] : { vagas: [1,2,3,4].map(numero => ({numero, ocupada: false})), livres: 4, ocupadas: 0, taxaOcupacao: 0 };
      return { ok: true, json: async () => body };
    }
  });
  vm.runInContext(fs.readFileSync(path.join(root, 'public/script.js'), 'utf8'), context);
  await new Promise(resolve => setImmediate(resolve));
  assert.match(element('simulation-controls').textContent, /Modo físico/);
  assert.equal(element('endpoint-method').textContent, 'GET');
  assert.match(element('connection-text').textContent, /ESP32 conectada/);
  offline = true;
  await vm.runInContext('loadInitialData()', context);
  assert.match(element('connection-text').textContent, /desatualizados/);
  assert.equal(element('available-list').innerHTML, '');
});
