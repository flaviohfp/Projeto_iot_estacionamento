# Estacionamento inteligente — painel web (2 vagas)

Parte web do projeto: somente HTML, CSS e JavaScript, para ser servida pela ESP32.

```
data/
  index.html
  style.css
  script.js
server.js      (só para testar no computador, não vai para a ESP32)
package.json   (só para o "npm start", não vai para a ESP32)
```

## O que a ESP32 precisa fazer

1. Servir os 3 arquivos: `/` → `index.html`, `/style.css`, `/script.js`.
2. Responder `GET /api/vagas` com JSON no formato:

```json
{"vagas": [
  {"numero": 1, "ocupada": false, "erro": false, "distancia": 25.3, "segundos": 0},
  {"numero": 2, "ocupada": true,  "erro": false, "distancia": 4.1,  "segundos": 42}
]}
```

| Campo       | Significado                                                        |
|-------------|--------------------------------------------------------------------|
| `numero`    | número da vaga (1 ou 2)                                            |
| `ocupada`   | `true` se o HC-SR04 detectou carro                                 |
| `erro`      | `true` se o sensor falhou (o site mostra "SEM SENSOR")             |
| `distancia` | leitura do HC-SR04 em cm, ou `null` sem eco                        |
| `segundos`  | há quanto tempo a vaga está ocupada (`millis()`), `0` se livre     |

O site consulta a API a cada 1 segundo. O card "Display LCD" mostra o mesmo texto que o LCD 16x2 deve exibir:

```
Livres: 1 de 2        (ou "ESTAC. LOTADO" quando não há vaga livre)
1:LIVRE 2:OCUP.       (cada vaga: "N:LIVRE", "N:OCUP." ou "N:ERRO")
```

## Testar no computador

```bash
npm start
```

Abra http://localhost:3000. O servidor simula a ESP32 trocando uma vaga a cada 5 segundos.
