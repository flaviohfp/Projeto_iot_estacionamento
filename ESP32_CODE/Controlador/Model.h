#pragma once
#include <Arduino.h>
#include <time.h>

constexpr int MAX_EVENTOS = 32;
struct Vaga {
  bool conhecida;
  bool ocupada;
  time_t entrada;
};
struct Evento {
  char id[64];
  uint8_t vaga;
  bool entrada;
  time_t dataHora;
  time_t inicio;
};
struct Persistencia {
  uint32_t versao;
  Vaga vagas[4];
  Evento pendentes[MAX_EVENTOS];
  uint8_t quantidade;
  Evento historico[MAX_EVENTOS];
  uint8_t totalHistorico;
  uint32_t descartados;
};
struct Snapshot {
  Persistencia dados;
  bool sensoresOk;
  time_t ultimaAtualizacao;
  uint32_t idadeLeituraMs;
};
