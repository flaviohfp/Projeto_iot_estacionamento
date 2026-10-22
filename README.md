# Estacionamento inteligente — painel para ESP32

Site estático (somente HTML, CSS e JavaScript) para ser servido pela própria ESP32. Sem banco de dados, sem bibliotecas externas e sem histórico: o painel mostra o estado atual das 4 vagas.

## Arquivos

```
data/
  index.html   página
  style.css    todo o estilo
  script.js    todo o JavaScript
server.js      servidor Node de teste (opcional, não vai para a ESP32)
```

A pasta `data/` é a que vai para a ESP32 (por exemplo, via LittleFS).

## API que a ESP32 precisa oferecer

`GET /api/vagas` deve responder JSON:

```json
{"vagas": [
  {"numero": 1, "ocupada": true,  "segundos": 42},
  {"numero": 2, "ocupada": false, "segundos": 0},
  {"numero": 3, "ocupada": false, "segundos": 0},
  {"numero": 4, "ocupada": true,  "segundos": 310}
]}
```

`segundos` é há quanto tempo a vaga está ocupada (calcule com `millis()` na ESP32; não precisa de internet/NTP). O site consulta a cada 2 segundos e conta o tempo entre as consultas.

## Testar no computador

```bash
node server.js
```

Abra http://localhost:3000. O servidor de teste serve `data/` e simula `/api/vagas`, trocando uma vaga aleatória a cada 5 segundos.
