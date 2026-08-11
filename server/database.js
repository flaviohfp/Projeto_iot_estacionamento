const fs = require("fs");
const path = require("path");
const sqlite3 = require("sqlite3");
const { open } = require("sqlite");

const databaseDir = path.join(__dirname, "..", "database");
const databaseFile = path.join(databaseDir, "estacionamento.sqlite");

async function initializeDatabase() {
  fs.mkdirSync(databaseDir, { recursive: true });

  const db = await open({
    filename: databaseFile,
    driver: sqlite3.Database
  });

  await db.exec("PRAGMA foreign_keys = ON;");

  await db.exec(`
    CREATE TABLE IF NOT EXISTS vagas (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      numero INTEGER NOT NULL UNIQUE CHECK (numero BETWEEN 1 AND 4),
      ocupada INTEGER NOT NULL DEFAULT 0,
      ultima_atualizacao TEXT,
      entrada_atual TEXT
    );

    CREATE TABLE IF NOT EXISTS eventos (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      vaga INTEGER NOT NULL CHECK (vaga BETWEEN 1 AND 4),
      tipo TEXT NOT NULL CHECK (tipo IN ('ENTRADA', 'SAIDA')),
      data_hora TEXT NOT NULL,
      entrada TEXT,
      saida TEXT,
      duracao_segundos INTEGER,
      duracao_texto TEXT,
      FOREIGN KEY (vaga) REFERENCES vagas(numero)
    );

    CREATE TABLE IF NOT EXISTS metadata (
      chave TEXT PRIMARY KEY,
      valor TEXT
    );
  `);

  for (let numero = 1; numero <= 4; numero += 1) {
    await db.run(
      "INSERT OR IGNORE INTO vagas (numero, ocupada, ultima_atualizacao, entrada_atual) VALUES (?, 0, NULL, NULL)",
      numero
    );
  }

  await db.run("INSERT OR IGNORE INTO metadata (chave, valor) VALUES ('last_update', NULL)");

  return db;
}

module.exports = {
  initializeDatabase
};
