const fs = require("fs");
const path = require("path");

const databaseDir = path.join(__dirname, "..", "database");
const databaseFile = path.join(databaseDir, "estacionamento.sqlite");

async function initializeDatabase() {
  if (process.env.DATABASE_URL) {
    return initializePostgresDatabase();
  }

  return initializeSqliteDatabase();
}

async function initializeSqliteDatabase() {
  const sqlite3 = require("sqlite3");
  const { open } = require("sqlite");

  fs.mkdirSync(databaseDir, { recursive: true });

  const db = await open({
    filename: databaseFile,
    driver: sqlite3.Database
  });

  db.kind = "sqlite";
  db.transaction = async (callback) => {
    await db.exec("BEGIN TRANSACTION");
    try {
      const result = await callback(db);
      await db.exec("COMMIT");
      return result;
    } catch (error) {
      await db.exec("ROLLBACK");
      throw error;
    }
  };

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

function convertPlaceholders(sql) {
  let index = 0;
  return sql.replace(/\?/g, () => {
    index += 1;
    return `$${index}`;
  });
}

function createPostgresAdapter(poolOrClient) {
  const db = {
    kind: "postgres",
    async all(sql, ...params) {
      const result = await poolOrClient.query(convertPlaceholders(sql), params);
      return result.rows;
    },
    async get(sql, ...params) {
      const result = await poolOrClient.query(convertPlaceholders(sql), params);
      return result.rows[0] || null;
    },
    async run(sql, ...params) {
      let query = convertPlaceholders(sql);
      const isEventInsert = /^\s*INSERT\s+INTO\s+eventos/i.test(query);

      if (isEventInsert && !/\bRETURNING\b/i.test(query)) {
        query += " RETURNING id";
      }

      const result = await poolOrClient.query(query, params);
      return {
        lastID: result.rows[0]?.id || null,
        changes: result.rowCount
      };
    },
    async exec(sql) {
      await poolOrClient.query(sql);
    }
  };

  if (typeof poolOrClient.connect === "function") {
    db.transaction = async (callback) => {
      const client = await poolOrClient.connect();
      const tx = createPostgresAdapter(client);

      try {
        await client.query("BEGIN");
        const result = await callback(tx);
        await client.query("COMMIT");
        return result;
      } catch (error) {
        await client.query("ROLLBACK");
        throw error;
      } finally {
        client.release();
      }
    };
  }

  return db;
}

async function initializePostgresDatabase() {
  const { Pool } = require("pg");

  const pool = new Pool({
    connectionString: process.env.DATABASE_URL,
    ssl: process.env.DATABASE_SSL === "false" ? false : { rejectUnauthorized: false }
  });

  const db = createPostgresAdapter(pool);

  await db.exec(`
    CREATE TABLE IF NOT EXISTS vagas (
      id SERIAL PRIMARY KEY,
      numero INTEGER NOT NULL UNIQUE CHECK (numero BETWEEN 1 AND 4),
      ocupada INTEGER NOT NULL DEFAULT 0,
      ultima_atualizacao TEXT,
      entrada_atual TEXT
    );

    CREATE TABLE IF NOT EXISTS eventos (
      id SERIAL PRIMARY KEY,
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
      "INSERT INTO vagas (numero, ocupada, ultima_atualizacao, entrada_atual) VALUES (?, 0, NULL, NULL) ON CONFLICT (numero) DO NOTHING",
      numero
    );
  }

  await db.run("INSERT INTO metadata (chave, valor) VALUES ('last_update', NULL) ON CONFLICT (chave) DO NOTHING");

  return db;
}

module.exports = {
  initializeDatabase
};
