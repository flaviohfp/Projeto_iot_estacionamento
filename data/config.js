// Configuração do painel.
// O ESP32 grava os dados no Cloud Firestore e o site faz as leituras.
// Para este projeto escolar, as regras do Firestore precisam permitir leitura
// e escrita sem autenticação. Não use essas regras abertas em produção.
window.PARKING_CONFIG = {
  mode: "firebase",
  firebaseProjectId: "iotestacionamento-e2b70"
};
