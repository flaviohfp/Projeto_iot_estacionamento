const path = require("path");
const express = require("express");
const cors = require("cors");
const { initializeDatabase } = require("./database");
const { createParkingService } = require("./parkingService");

const ENABLE_API_KEY = String(process.env.ENABLE_API_KEY || "false").toLowerCase() === "true";
const API_KEY = process.env.API_KEY || "";

function apiKeyMiddleware(req, res, next) {
  if (!ENABLE_API_KEY) {
    return next();
  }

  if (!API_KEY) {
    return res.status(500).json({
      success: false,
      error: "API Key habilitada, mas API_KEY nao foi configurada no .env."
    });
  }

  const providedKey = req.header("X-API-Key");
  if (providedKey !== API_KEY) {
    return res.status(401).json({
      success: false,
      error: "API Key invalida ou ausente."
    });
  }

  return next();
}

function asyncHandler(handler) {
  return (req, res, next) => {
    Promise.resolve(handler(req, res, next)).catch(next);
  };
}

async function createApp({ io = null, serveStatic = false } = {}) {
  const app = express();
  const db = await initializeDatabase();
  const parkingService = createParkingService(db);

  app.locals.parkingService = parkingService;

  app.use(cors());
  app.use(express.json());

  if (serveStatic) {
    app.use(express.static(path.join(__dirname, "..", "public")));
  }

  async function publishUpdate() {
    const payload = await parkingService.getSnapshot();
    if (io) {
      io.emit("parking:update", payload);
    }
    return payload;
  }

  app.get("/api/vagas", apiKeyMiddleware, asyncHandler(async (req, res) => {
    res.json(await parkingService.getStatus());
  }));

  app.post("/api/vagas/status", apiKeyMiddleware, asyncHandler(async (req, res) => {
    const result = await parkingService.updateVagaStatus(req.body);
    const payload = await publishUpdate();
    res.json({
      success: true,
      changed: result.changed,
      event: result.event,
      status: payload.status,
      historico: payload.historico
    });
  }));

  app.get("/api/historico", apiKeyMiddleware, asyncHandler(async (req, res) => {
    res.json(await parkingService.getHistorico());
  }));

  app.get("/api/status/display", apiKeyMiddleware, asyncHandler(async (req, res) => {
    res.json(await parkingService.getDisplayStatus());
  }));

  app.get("/api/health", asyncHandler(async (req, res) => {
    res.json({
      success: true,
      database: db.kind,
      realtime: io ? "socket.io" : "polling"
    });
  }));

  app.use((req, res) => {
    res.status(404).json({
      success: false,
      error: "Rota nao encontrada."
    });
  });

  app.use((err, req, res, next) => {
    const status = err.statusCode || 500;
    res.status(status).json({
      success: false,
      error: err.message || "Erro interno do servidor."
    });
  });

  return app;
}

module.exports = {
  createApp
};
