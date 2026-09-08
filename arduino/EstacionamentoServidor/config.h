#pragma once
// Pinagem para ESP32 DevKit/WROOM clássico. Revise para outras placas.
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";
const char* AP_SSID = "Estacionamento-ESP32";
const char* AP_PASSWORD = "maquete123"; // Troque antes de gravar (mínimo 8 caracteres).
const int SENSOR_PINS[4] = {13, 14, 27, 26};
const int SENSOR_DETECTADO = LOW;
// RGB: somente canais vermelho e verde; deixe azul desconectado.
const int LED_RED[4] = {16, 18, 23, 32};
const int LED_GREEN[4] = {17, 19, 25, 33};
const bool LED_ANODO_COMUM = false;
#define USAR_LEDS 0
#define USAR_OLED 0
#define USAR_RTC_DS3231 0
const unsigned long DEBOUNCE_MS = 800;
