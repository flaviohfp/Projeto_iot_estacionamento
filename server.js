// Servidor de teste: serve a pasta data/ e simula a API da ESP32.
// Uso: node server.js  ->  http://localhost:3000
const http = require("node:http");
const fs = require("node:fs");
const path = require("node:path");

const PORT = process.env.PORT || 3000;
const ROOT = path.join(__dirname, "data");
const TYPES = { ".html": "text/html; charset=utf-8", ".css": "text/css; charset=utf-8", ".js": "text/javascript; charset=utf-8" };

// Duas vagas simuladas: a cada 5 s uma delas muda de estado.
const vagas = [1, 2].map(numero => ({ numero, ocupada: false, desde: 0 }));
setInterval(() => {
  const vaga = vagas[Math.floor(Math.random() * vagas.length)];
  vaga.ocupada = !vaga.ocupada;
  vaga.desde = Date.now();
}, 5000);

http.createServer((req, res) => {
  const url = new URL(req.url, "http://localhost");

  if (url.pathname === "/api/vagas") {
    const body = {
      vagas: vagas.map(v => ({
        numero: v.numero,
        ocupada: v.ocupada,
        erro: false,
        distancia: Number(((v.ocupada ? 4 : 25) + Math.random()).toFixed(1)),
        segundos: v.ocupada ? Math.floor((Date.now() - v.desde) / 1000) : 0
      }))
    };
    res.writeHead(200, { "Content-Type": "application/json", "Cache-Control": "no-store" });
    return res.end(JSON.stringify(body));
  }

  const file = path.join(ROOT, url.pathname === "/" ? "index.html" : path.normalize(url.pathname));
  if (!file.startsWith(ROOT) || !TYPES[path.extname(file)]) {
    res.writeHead(404);
    return res.end("Não encontrado");
  }
  fs.readFile(file, (err, content) => {
    if (err) {
      res.writeHead(404);
      return res.end("Não encontrado");
    }
    res.writeHead(200, { "Content-Type": TYPES[path.extname(file)] });
    res.end(content);
  });
}).listen(PORT, () => console.log(`Painel de teste: http://localhost:${PORT}`));
