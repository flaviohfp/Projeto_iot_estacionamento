#pragma once
// Pinagem para ESP32 DevKit/WROOM clássico. Revise para outras placas.
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";
const char* AP_SSID = "Estacionamento-ESP32";
const char* AP_PASSWORD = "maquete123"; // Troque antes de gravar (mínimo 8 caracteres).
const int SENSOR_PINS[4] = {32, 33, 25, 26};
const int SENSOR_DETECTADO = LOW;
// GPIO dos LEDs propostos: a imagem não identifica seus números.
// Azul permanece apagado: resistor de 220 ohms ao GND (catodo comum)
// ou a 3,3 V (anodo comum). Um resistor por canal: 12 no total.
const int LED_RED[4] = {13, 16, 18, 23};
const int LED_GREEN[4] = {14, 17, 19, 27};
const bool LED_ANODO_COMUM = false;
#define USAR_LEDS 1
#define USAR_OLED 1
#define USAR_RTC_DS3231 1
const unsigned long DEBOUNCE_MS = 800;
