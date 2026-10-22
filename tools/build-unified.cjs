// Gera um unico sketch para gravar nas duas placas, escolhendo o papel no inicio.
// Uso: node tools/build-unified.cjs
const fs = require('node:fs');
const path = require('node:path');
require('./embed-site.cjs');
const root = path.resolve(__dirname, '..');
const read = file => fs.readFileSync(path.join(root, file), 'utf8').replace(/\r\n/g, '\n');
const inline = file => read(file).replace(/^#pragma once\s*$/gm, '').trim();
function prototypes(source) {
  return [...source.matchAll(/^(?:void|bool|int|time_t|String|Snapshot|JsonObject)\s+\w+\([^\n]*\)\s*\{/gm)]
    .map(match => match[0].replace(/\s*\{$/, ';').replace(/\s*=\s*false/g, ''))
    .filter(line => !/^void (setup|loop)\(/.test(line))
    .join('\n');
}
const sensors = read('ESP32_CODE/Sensores/Sensores.ino').replace(/^#include <Arduino.h>\s*$/m, '');
const original = read('ESP32_CODE/Controlador/Controlador.ino');
const controller = original.replace(/^#include "(?:Config|Model|GoogleRoots|SiteAssets)\.h"\s*$/gm, '');
const includeEnd = controller.indexOf('// A API copia');
const libraries = controller.slice(0, includeEnd);
const body = controller.slice(includeEnd);
const config = inline('ESP32_CODE/Controlador/Config.h');
const rules = read('firestore.rules');
const content = `/*
  ESTACIONAMENTO COMPLETO - DUAS ESP32 DEVKIT
  Um unico arquivo: sensores, LEDs RGB, LCD, servidor, site e Firebase.

  GRAVACAO:
  1. PAPEL_DA_ESP = PLACA_SENSORES: envie para a placa dos HC-SR04.
  2. PAPEL_DA_ESP = PLACA_CONTROLADOR: envie para a placa dos LEDs/LCD.
  Bibliotecas do controlador: ArduinoJson 7.x e hd44780 (Bill Perry).
  Placa na IDE: ESP32 Dev Module. Monitor Serial: 115200.

  UART: GPIO17/TX dos sensores -> GPIO16/RX do controlador; GND comum.
  Cada ECHO precisa de divisor de 5 V para 3,3 V. Consulte o guia.
  O painel local ja esta incorporado. Nenhum upload LittleFS e necessario.
  Para o site remoto: habilite Firebase Authentication Email/senha,
  crie o usuario da placa, preencha senha/UID abaixo e publique firestore.rules.
  O site remoto tambem precisa ser publicado no Hosting, conforme o guia.
  Guia: ESP32_CODE/README.md
  Arquivo gerado por tools/build-unified.cjs; pode ser editado na Arduino IDE.
*/
#include <Arduino.h>

#define PLACA_SENSORES 1
#define PLACA_CONTROLADOR 2
#ifndef PAPEL_DA_ESP
// ALTERE APENAS ESTA OPCAO ENTRE AS DUAS GRAVACOES:
#define PAPEL_DA_ESP PLACA_SENSORES
#endif

// CONFIGURACAO: Wi-Fi, Firebase, LEDs e LCD.
${config}

/*
  REGRAS FIRESTORE - copie o trecho abaixo para Firebase > Firestore > Regras.
  Substitua o UID pelo mesmo valor de FIREBASE_DEVICE_UID e clique Publicar.
  Regras nao sao enviadas ao Firebase pela gravacao na Arduino IDE.

${rules}
*/

// Declaracoes explicitas evitam ambiguidades do preprocessador da Arduino IDE.
void setup();
void loop();

#if PAPEL_DA_ESP == PLACA_SENSORES
${prototypes(sensors)}

// ================= PLACA DOS QUATRO HC-SR04 =================
${sensors}

#elif PAPEL_DA_ESP == PLACA_CONTROLADOR
${libraries}

// ================= TIPOS E PERSISTENCIA =================
${inline('ESP32_CODE/Controlador/Model.h')}

// ================= CERTIFICADOS HTTPS DO GOOGLE =================
${inline('ESP32_CODE/Controlador/GoogleRoots.h')}

// ================= SITE COMPLETO INCORPORADO =================
${inline('ESP32_CODE/Controlador/SiteAssets.h')}

// Configuracao e inicializacao do app Web para a versao hospedada do site.
const char SITE_FIREBASE_CONFIG[] PROGMEM = R"parkingasset(${read('data/config.js')})parkingasset";
const char SITE_FIREBASE_SDK[] PROGMEM = R"parkingasset(${read('data/firebase.js')})parkingasset";

// ================= SERVIDOR, LEDS, LCD E FIREBASE =================
${prototypes(body)}

${body.replace('  server.on("/api/vagas",', `  server.on("/firebase.js", HTTP_GET, []() { server.send_P(200, "application/javascript; charset=utf-8", SITE_FIREBASE_SDK); });
  server.on("/config-firebase.js", HTTP_GET, []() { server.send_P(200, "application/javascript; charset=utf-8", SITE_FIREBASE_CONFIG); });
  server.on("/api/vagas",`)}

#else
#error "PAPEL_DA_ESP deve ser PLACA_SENSORES ou PLACA_CONTROLADOR"
#endif
`;
const directory = path.join(root, 'ESP32_CODE/ProjetoCompleto');
fs.mkdirSync(directory, { recursive: true });
fs.writeFileSync(path.join(directory, 'ProjetoCompleto.ino'), content);
console.log('ProjetoCompleto.ino gerado com os dois programas e todo o site.');
