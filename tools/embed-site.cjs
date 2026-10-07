// Execute com: node tools/embed-site.cjs
// Atualiza os arquivos do site incorporados ao firmware; nao exige LittleFS.
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const assets = { INDEX: 'index.html', STYLE: 'style.css', DATA: 'data.js', SCRIPT: 'script.js' };
let header = '// Gerado por tools/embed-site.cjs a partir de data/.\n#pragma once\n#include <Arduino.h>\n';
for (const [name, file] of Object.entries(assets)) {
  const content = fs.readFileSync(path.join(root, 'data', file), 'utf8');
  if (content.includes(')parkingasset"')) throw Error(`Delimitador reservado em ${file}`);
  header += `\nconst char SITE_${name}[] PROGMEM = R"parkingasset(${content})parkingasset";\n`;
}
fs.writeFileSync(path.join(root, 'ESP32_CODE/Controlador/SiteAssets.h'), header);
console.log('Site incorporado ao firmware Controlador.');
