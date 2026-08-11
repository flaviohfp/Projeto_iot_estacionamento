require("dotenv").config();

const path = require("path");
const express = require("express");
const cors = require("cors");
const http = require("http");
const { Server } = require("socket.io");
const { initializeDatabase } = require("./server/database");
const { createParkingService } = require("./server/parkingService");

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "*"
  }
});

const PORT = Number(process.env.PORT || 3000);
const ENABLE_API_KEY = String(process.env.ENABLE_API_KEY || "false").toLowerCase() === "true";
const API_KEY = process.env.API_KEY || "";

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

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

async function start() {
  const db = await initializeDatabase();
  const parkingService = createParkingService(db);

  async function publishUpdate() {
    const payload = {
      status: await parkingService.getStatus(),
      historico: await parkingService.getHistorico()
    };
    io.emit("parking:update", payload);
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
      status: payload.status
    });
  }));

  app.get("/api/historico", apiKeyMiddleware, asyncHandler(async (req, res) => {
    res.json(await parkingService.getHistorico());
  }));

  app.get("/api/status/display", apiKeyMiddleware, asyncHandler(async (req, res) => {
    res.json(await parkingService.getDisplayStatus());
  }));

  io.on("connection", async (socket) => {
    socket.emit("parking:update", {
      status: await parkingService.getStatus(),
      historico: await parkingService.getHistorico()
    });
  });

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

  server.listen(PORT, () => {
    console.log(`Estacionamento Inteligente rodando em http://localhost:${PORT}`);
  });
}

start().catch((error) => {
  console.error("Falha ao iniciar o servidor:", error);
  process.exit(1);
});
