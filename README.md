# Estacionamento inteligente — duas ESP32

Quatro HC-SR04 enviam leituras pela ESP32 **Sensores** à ESP32 **Controlador**, por UART. O controlador aciona LEDs RGB e LCD I2C 16×2, hospeda o painel local e envia vagas/histórico ao Firebase Cloud Firestore. O site remoto lê esse banco.

**Guia de instalação, ligações e configuração:** [ESP32_CODE/README.md](ESP32_CODE/README.md).

## Programas para Arduino IDE

- `ESP32_CODE/Sensores/Sensores.ino`: placa dos quatro HC-SR04.
- `ESP32_CODE/Controlador/Controlador.ino`: placa dos LEDs e LCD.
- `ESP32_CODE/Controlador/Config.h`: Wi-Fi, pinos e autenticação Firebase. Preencha senha e UID do usuário do dispositivo antes de enviar.

O arquivo antigo `ESP32_CODE/Estacionamento/Estacionamento.ino` foi preservado como referência da versão com uma placa e sensores digitais. Para a montagem atual, use os dois programas acima.

## Site e servidor

O site fica em `data/`. O controlador contém uma cópia incorporada em `SiteAssets.h`; o envio normal pela Arduino IDE já inclui o painel. Após alterar o site, execute `node tools/embed-site.cjs` e reenvie o controlador.

Painel local: `http://IP_DA_ESP32/`, mostrado no Monitor Serial. API: `/api/vagas`, `/api/historico`, `/api/health`. O painel local consulta a ESP32; o remoto consulta o Firebase. LEDs e LCD funcionam sem internet.

O Hosting publica `data/` no site `iotestacionamento-e2b70-8a237`. Depois de configurar Authentication e o UID em `firestore.rules`, publique com a CLI autenticada:

```sh
firebase deploy --only hosting,firestore:rules --project iotestacionamento-e2b70
```

URL após publicação: https://iotestacionamento-e2b70-8a237.web.app. A Vercel também serve `data/`.

## Firestore

Documentos de ocupação: `vagas/vaga1` até `vagas/vaga4`:

```json
{"numero": 1, "ocupada": true, "entradaAtual": "2026-10-07T12:00:00Z"}
```

`metadata/status`: `ultimaAtualizacao`, `sensoresOk`, `eventosDescartados`. `eventos/{id}`: `vaga`, `tipo` (`ENTRADA`/`SAIDA`), `dataHora`, `entrada`, `saida`. Horários conhecidos usam UTC com `Z`; desconhecidos são `null`.

A escrita exige o usuário Firebase da placa. O controlador envia lotes a cada dez segundos; o site consulta a cada três e sinaliza dados com mais de trinta segundos. Falha de sensor não aparece como vaga livre.

Fontes: [Firestore REST](https://firebase.google.com/docs/firestore/use-rest-api), [Firebase Auth REST](https://firebase.google.com/docs/reference/rest/auth), [hd44780](https://github.com/duinoWitchery/hd44780).
