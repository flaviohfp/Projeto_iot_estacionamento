require("dotenv").config();

const http = require("http");
const { Server } = require("socket.io");
const { createApp } = require("./server/app");

const PORT = Number(process.env.PORT || 3000);

async function start() {
  const app = await createApp({ serveStatic: true });
  const server = http.createServer(app);
  const io = new Server(server, {
    cors: {
      origin: "*"
    }
  });

  app.locals.io = io;

  io.on("connection", async (socket) => {
    socket.emit("parking:update", await app.locals.parkingService.getSnapshot());
  });

  server.listen(PORT, () => {
    console.log(`Estacionamento Inteligente rodando em http://localhost:${PORT}`);
  });
}

start().catch((error) => {
  console.error("Falha ao iniciar o servidor:", error);
  process.exit(1);
});
