# Instalação com duas ESP32 DevKit e LCD I2C 16×2

## 1. Divisão das placas

```text
4 HC-SR04 -> ESP32 Sensores --UART--> ESP32 Controlador -> LEDs RGB + LCD
                                               |
                                  Wi-Fi: Iphone de Flavio
                                               |
                                   Firebase Cloud Firestore
                                               |
                                           Site remoto

ESP32 Controlador também hospeda o site local em http://IP_DA_PLACA/
```

UART: **GPIO17/TX da placa Sensores → GPIO16/RX da placa Controlador**, além de **GND → GND**. A placa dos sensores não precisa conectar ao Wi-Fi. Não conecte TX com TX. Os pinos repetidos entre as tabelas pertencem a placas diferentes.

## 2. HC-SR04 — placa Sensores

| Vaga | TRIG | ECHO |
| --- | --- | --- |
| 1 | GPIO4 | GPIO2 |
| 2 | GPIO5 | GPIO18 |
| 3 | GPIO12 | GPIO14 |
| 4 | GPIO27 | GPIO26 |

Alimente os HC-SR04 com 5 V e compartilhe o GND. **Não conecte ECHO de 5 V diretamente à ESP32.** Use um divisor para cada sensor:

```text
ECHO --- resistor 1 kΩ ---+--- GPIO ECHO da ESP32
                         |
                    resistor 2 kΩ
                         |
                        GND
```

Posicione os sensores para que uma vaga vazia também produza eco válido (por exemplo, uma superfície de fundo). Ausência de eco significa falha, não vaga livre. Ajuste `DISTANCIA_OCUPADA_CM` e `DISTANCIA_LIVRE_CM` em `Sensores.ino` conforme a maquete: padrões 12 cm para ocupar e 16 cm para liberar. Entre os limites conserva o estado anterior. Três leituras consecutivas confirmam uma mudança; três falhas tornam a leitura desconhecida. Os sensores são disparados em sequência para reduzir interferência.

## 3. LEDs RGB e LCD — placa Controlador

| Vaga | Verde | Vermelho |
| --- | --- | --- |
| 1 | GPIO13 | GPIO14 |
| 2 | GPIO27 | GPIO12 |
| 3 | GPIO26 | GPIO25 |
| 4 | GPIO33 | GPIO32 |

Use resistor de 220–330 Ω em cada canal vermelho e verde. O canal azul fica desconectado. Padrão: **cátodo comum** ligado ao GND (`RGB_ANODO_COMUM = false`). Para ânodo comum ligado a 3,3 V, altere para `true`. Não aplique 5 V aos GPIOs.

Verde = livre; vermelho = ocupada; ambas apagadas = leitura desconhecida ou UART perdida. LCD: `V1L` livre, `V1X` ocupada, `V1?` falha.

LCD: SDA → GPIO21; SCL → GPIO22; GND comum. A biblioteca detecta o endereço I2C (incluindo os adaptadores usuais 0x27/0x3F). Para LCD/backpack alimentado em 5 V com resistores I2C ligados a 5 V, use conversor bidirecional de nível lógico em SDA/SCL entre LCD e ESP32; os GPIOs não toleram 5 V. Ajuste o potenciômetro de contraste se acender sem texto.

GPIO2, GPIO5 e GPIO12 são pinos de configuração de boot da ESP32 clássica. Se houver falha ao iniciar/gravar, confira os níveis ao ligar e desconecte as cargas durante a gravação. LED de ânodo comum pode puxar GPIO12 para HIGH: nesse caso substitua o GPIO12 do vermelho da vaga 2 por GPIO23 na montagem e em `Config.h`.

## 4. Configurar Firebase — necessário uma vez

A API key identifica o projeto, mas não autoriza a placa a gravar. No projeto **iotestacionamento-e2b70**:

1. Confirme a criação do **Cloud Firestore**, banco `(default)`.
2. Abra **Authentication → Começar → Método de login** e habilite **E-mail/senha**.
3. Em **Authentication → Usuários → Adicionar usuário**, crie `esp32@estacionamento.local` com uma senha própria para o dispositivo. Não use sua senha Google.
4. Copie o **UID** desse usuário.
5. Abra `Controlador/Config.h`. Substitua `PREENCHA_A_SENHA_DO_USUARIO_FIREBASE` pela senha criada e `PREENCHA_O_UID_DO_USUARIO_FIREBASE` pelo UID. Se usar outro e-mail, altere `FIREBASE_EMAIL`.
6. No arquivo `firestore.rules` da raiz, substitua `PREENCHA_O_UID_DO_USUARIO_FIREBASE` pelo **mesmo UID**. Copie o conteúdo para **Firestore Database → Regras** e clique em **Publicar**.

O navegador somente lê as informações de vagas. A senha do dispositivo fica no firmware, nunca em `data/config.js`. Não habilite escrita pública. Esta entrega prepara os programas/regras; não cria o usuário nem publica regras na sua conta.

O controlador usa HTTPS com certificados públicos do Google e login por e-mail/senha; refaz o login antes de expirar. Horários usam NTP e UTC. Aguarda horário válido para enviar ao Firebase. Sem NTP, LEDs/LCD e servidor continuam ativos; eventos observados antes de sincronizar têm horários `null`, sem inventar durações.

## 5. Gravar na Arduino IDE

1. No Gerenciador de Placas, instale **esp32 by Espressif Systems**, série 3.x.
2. No Gerenciador de Bibliotecas, instale **ArduinoJson by Benoit Blanchon**, versão 7.x, e **hd44780 by Bill Perry**. O programa dos sensores usa apenas bibliotecas do pacote ESP32.
3. Abra `Sensores/Sensores.ino`, selecione **ESP32 Dev Module**, escolha a porta da placa dos sensores e envie.
4. Abra `Controlador/Controlador.ino`; os `.h` da pasta aparecem como abas. Preencha senha/UID em `Config.h`, escolha a porta da placa dos LEDs/LCD e envie.
5. Se aparecer falta de espaço, escolha uma partição com mais espaço para o programa, como **Huge APP**. O site está em `SiteAssets.h`; não precisa carregar LittleFS.
6. Monitor Serial: **115200 baud**. Sensores mostram estados/distâncias; controlador mostra IP, falhas e confirmação de envio.

O Wi-Fi já está configurado:

```cpp
const char* ssid = "Iphone de Flavio";
const char* password = "Flavio1000";
```

O SSID diferencia maiúsculas/minúsculas e deve coincidir exatamente com o hotspot. No iPhone, ative Acesso Pessoal e **Maximizar Compatibilidade**, para rede de 2,4 GHz. Mantenha o hotspot ativo e com internet.

## 6. Abrir o site

**Local:** conecte um computador à mesma rede e abra o IP do controlador mostrado no Monitor Serial. O painel é servido pela ESP32 e consulta `/api/vagas` e `/api/historico`, sem Firebase. Hotspots podem restringir acesso entre clientes; se o IP não abrir, teste em um roteador 2,4 GHz comum ou use o painel remoto. O IP pode mudar ao reconectar.

**Remoto:** o site em `data/` lê o Firestore. Configure primeiro Authentication/UID/regras e publique usando a CLI Firebase autenticada:

```sh
npm install -g firebase-tools
firebase login
firebase deploy --only hosting,firestore:rules --project iotestacionamento-e2b70
```

URL após publicação: https://iotestacionamento-e2b70-8a237.web.app. O controlador não precisa de IP público ou encaminhamento de portas para alimentar o site remoto.

## 7. Conferir a montagem

1. Ligue ambas as placas e confira UART/GND. Após três leituras válidas, o LCD deve mostrar as quatro vagas.
2. Aproxime um objeto. Após três confirmações, o LED da vaga passa a vermelho e a contagem diminui.
3. Afaste além de 16 cm. LED verde; evento de saída com duração quando a entrada é conhecida.
4. Abra o IP e `/api/health`. Confira `wifi`, `sensores`, `ntp`, `lcd`, `firebase` e `mensagemFirebase`.
5. No Firestore aparecem `vagas/vaga1` até `vaga4`, `metadata/status` e eventos. O site remoto atualiza em aproximadamente 13 s (envio a cada 10 s, leitura a cada 3 s, mais latência).
6. Desconecte UART: após 3 s os LEDs apagam, LCD indica falha, API local retorna 503; remoto indica a falha no próximo envio ou dados desatualizados após 30 s.
7. Desligue internet: LEDs/LCD continuam; eventos são reenviados na reconexão.

## Persistência e limites

Estado, entradas conhecidas, últimos 32 eventos locais e até 32 pendentes são salvos na memória NVS. IDs fixos evitam duplicação em novas tentativas. As vagas, metadados e até dez eventos são enviados em lote atômico. A fila só é removida após sucesso HTTP.

Mais de 32 mudanças sem enviar excedem a fila remota: as adicionais incrementam `eventosDescartados`, disponível na API/Firestore, e geram aviso Serial. O histórico remoto consulta os últimos 100; o banco não apaga registros antigos automaticamente. O início de uma ocupação existente na primeira inicialização é desconhecido, sem duração; entradas previamente registradas são preservadas no reinício. Movimentações com placas desligadas não podem ser reconstruídas.

Após editar `data/`, execute `node tools/embed-site.cjs` na raiz e grave novamente o controlador. Node serve apenas para atualizar o firmware; não é necessário para operar o projeto.

## Validação desta entrega

Os dois sketches foram compilados para `esp32:esp32:esp32` (ESP32 Dev Module), com esp32 3.3.11, ArduinoJson 7.4.2 e hd44780 1.3.2. O controlador cabe na partição padrão de 1.310.720 bytes. Seis testes do provedor de dados passaram: API local, falha UART, operação local sem NTP, documentos Firestore, falha de sensor remoto e demonstração. Comando: `node --experimental-vm-modules --test tests/data.test.cjs`.

Esta validação não substitui o teste na montagem física. O login real e a escrita no Firestore dependem de criar o usuário e preencher senha/UID/publicar as regras. A consulta de configuração do Authentication retornou `CONFIGURATION_NOT_FOUND`; confirme a ativação desse serviço no console.

Fontes: [Arduino ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/), [Firestore REST](https://firebase.google.com/docs/firestore/use-rest-api), [Auth REST](https://firebase.google.com/docs/reference/rest/auth), [hd44780](https://github.com/duinoWitchery/hd44780), [certificados Google](https://pki.goog/repository/).
