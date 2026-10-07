#pragma once

const char* ssid = "Iphone de Flavio";
const char* password = "Flavio1000";
const char* FIREBASE_PROJECT_ID = "iotestacionamento-e2b70";
const char* FIREBASE_API_KEY = "AIzaSyBGEsf-JUPuUwmuvsife2hFNjZp7O8cisM";

// Crie este usuario em Firebase > Authentication > Usuarios.
// Habilite Email/senha e copie a senha e o UID para os campos abaixo.
const char* FIREBASE_EMAIL = "esp32@estacionamento.local";
const char* FIREBASE_PASSWORD = "PREENCHA_A_SENHA_DO_USUARIO_FIREBASE";
const char* FIREBASE_DEVICE_UID = "PREENCHA_O_UID_DO_USUARIO_FIREBASE";

const int pinoVerde[4] = {13, 27, 26, 33};
const int pinoVermelho[4] = {14, 12, 25, 32};
// Catodo comum: false (GND). Anodo comum: true (3,3 V).
constexpr bool RGB_ANODO_COMUM = false;
constexpr int LCD_SDA = 21;
constexpr int LCD_SCL = 22;
constexpr int LCD_COLUNAS = 16;
constexpr int LCD_LINHAS = 2;
constexpr uint32_t SENSORES_TIMEOUT_MS = 3000;
constexpr uint32_t INTERVALO_FIREBASE_MS = 10000;
