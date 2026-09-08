# Estacionamento Inteligente

Site para acompanhar as quatro vagas de uma maquete de estacionamento com ESP32. O painel mostra a ocupação das vagas e o histórico de entradas e saídas, com atualização automática.

## Funcionalidades

- Mapa das quatro vagas, indicando livre ou ocupada.
- Quantidade de vagas livres, ocupadas e taxa de ocupação.
- Aviso quando o estacionamento está lotado.
- Histórico com data, hora e tempo de permanência observado.
- Status da conexão com a ESP32 e aviso de dados desatualizados.

## Integração com a maquete

A ESP32 lê os quatro sensores IR instalados no piso, atualiza os LEDs RGB (verde para livre e vermelho para ocupada) e mostra as vagas disponíveis no OLED. O RTC DS3231 fornece o horário dos registros.

A placa pode hospedar o próprio site na rede local. Nesse modo, os sensores controlam as vagas, a simulação fica bloqueada e os últimos 40 eventos são preservados mesmo após desligar a ESP32.

Após configurar e gravar o firmware, conecte-se à rede Wi-Fi da maquete e abra [http://192.168.4.1](http://192.168.4.1). As instruções de instalação e ligação estão no [guia da ESP32](ESP32_SERVIDOR.md).

## Executar no computador

Para conhecer o painel e testar a simulação, use Node.js 20 ou superior. Copie `.env.example` para `.env`, mantenha `USE_LOCAL_DATABASE=true` e execute:

```bash
npm install
npm start
```

Abra [http://localhost:3000](http://localhost:3000). Nesse modo, os dados ficam em memória e são apagados ao reiniciar o servidor.
