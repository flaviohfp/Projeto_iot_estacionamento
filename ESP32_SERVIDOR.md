# ESP32 como servidor da maquete

Use `arduino/EstacionamentoServidor/EstacionamentoServidor.ino` para hospedar o próprio site na ESP32. O sketch antigo `EstacionamentoInteligente` continua sendo a alternativa que envia dados para Node.js; grave apenas um deles.

## Preparar e gravar

1. Configuração de referência: ESP32 DevKit com ESP32-WROOM clássico e quatro sensores IR digitais. Confirme o modelo antes de ligar: a pinagem abaixo não é universal.
2. Na Arduino IDE, instale o pacote de placas **esp32 by Espressif Systems** e selecione sua placa e porta USB.
3. Abra o novo sketch. Edite `config.h`: rede/senha do roteador (opcional), senha da rede da maquete, pinos e polaridade dos sensores.
4. O painel já está incorporado em `web_assets.h`; não é preciso enviar arquivos LittleFS separadamente. Depois de modificar `public/`, execute `npm run build:esp32` (no PowerShell com bloqueio de scripts, `npm.cmd run build:esp32`) antes de recompilar.
5. Grave o sketch. Abra o Monitor Serial em 115200 baud. Conecte o celular/computador à rede `Estacionamento-ESP32`, senha inicial `maquete123` (troque em `config.h`). Mantenha a conexão mesmo que o celular avise que não há internet.
6. Abra **http://192.168.4.1**. O site, API e leitura dos sensores funcionam na placa, sem Node.js ou internet. Se configurado, o roteador também permite acesso pelo IP atribuído à ESP32, consultável na lista de dispositivos do roteador.

## Ligações de referência

| Componente | GPIO |
| --- | --- |
| Saídas digitais dos sensores das vagas 1–4 | 13, 14, 27, 26 |
| Vermelho dos LEDs das vagas 1–4 | 16, 18, 23, 32 |
| Verde dos LEDs das vagas 1–4 | 17, 19, 25, 33 |
| SDA do OLED/RTC | 21 |
| SCL do OLED/RTC | 22 |

Use GND comum. As entradas da ESP32 recebem sinais de 3,3 V: se a saída do sensor for 5 V, use conversão de nível. LEDs precisam de resistor por canal (exemplo: 330 Ω, verificar corrente do LED); deixe o azul desconectado. Os GPIO 16/17 podem estar reservados em placas com PSRAM. Revise a pinagem nessas placas.

Os periféricos opcionais ficam desligados por padrão para permitir testar somente os sensores. Ative `USAR_LEDS`, `USAR_OLED` e/ou `USAR_RTC_DS3231` em `config.h` conforme a montagem. Para LED de ânodo comum, ajuste `LED_ANODO_COMUM`. Verde indica livre e vermelho ocupado; durante estabilização inicial ficam apagados.

Para OLED SSD1306 128×64, instale **Adafruit SSD1306** e **Adafruit GFX Library** com dependências. Endereço padrão: 0x3C. Para RTC, instale **RTClib**. OLED e RTC compartilham o I2C. O firmware detecta falhas de inicialização e informa no Serial.

## Horário e histórico

A placa usa NTP quando tem internet ou um DS3231 previamente ajustado em **UTC**. RTC com perda de alimentação exige ajuste, por exemplo pelo exemplo da RTClib, usando UTC. Este firmware não grava automaticamente NTP no RTC. Sem relógio válido, o painel mostra horários ausentes; não inventa data. A duração de uma permanência é medida com `millis()` (limite de aproximadamente 49 dias).

O histórico conserva as últimas 40 mudanças em RAM e é apagado ao reiniciar. A primeira leitura estável é uma fotografia inicial, não gera entrada fictícia. Para veículo já presente ao ligar, a permanência é observada a partir da inicialização. Histórico persistente continua disponível na arquitetura Node.js/Firebase anterior.

## API e teste de bancada

- `GET /api/health`: modo físico, sensores inicializados e relógio sincronizado.
- `GET /api/vagas`: quatro leituras confirmadas após 800 ms de estabilidade.
- `GET /api/historico`: últimas mudanças, mais recentes primeiro.
- `GET /api/status/display`: vagas livres para outros consumidores.
- Escritas/simulação são recusadas com HTTP 405. Os sensores são a fonte do estado físico.

Confira uma vaga de cada vez: coloque o veículo, aguarde pelo menos 800 ms mais o polling de até 3 s, confira ocupação no site, LED vermelho e OLED; retire e confira verde, saída e duração. Teste quatro ocupadas, oscilação rápida do sensor, reinício e perda de Wi-Fi. A perda de comunicação mantém a última fotografia e a identifica como desatualizada. Um sensor travado ou desconectado não é diagnosticável com certeza apenas pela entrada digital; verifique alimentação e fiação no teste físico.

O acesso direto foi projetado para a rede local da maquete. O site publicado na Vercel não passa a acessar a placa automaticamente: para essa arquitetura use o sketch cliente e servidor Node.js existentes.

Base da implementação: [Wi-Fi da Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html) e [WebServer da Espressif](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/examples/WebServer).
