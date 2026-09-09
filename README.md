# Estacionamento Inteligente

Projeto de um sistema de estacionamento inteligente utilizando **ESP32, sensores IR, LEDs, LCD 16x2, Wi-Fi e Firebase Cloud Firestore**.

A ESP32 identifica quais vagas estão ocupadas, mostra as informações no LCD, controla os LEDs e envia os dados para o Cloud Firestore. O site é hospedado diretamente pela ESP32 utilizando o **LittleFS**.

## Estrutura do projeto

```text
Estacionamento/
│
├── Estacionamento.ino
├── firestore.rules
│
└── data/
    ├── index.html
    ├── style.css
    ├── script.js
    ├── data.js
    └── config.js
```

### Arquivos

* `Estacionamento.ino` → código principal da ESP32.
* `firestore.rules` → regras de acesso do Cloud Firestore.
* `data/index.html` → página principal do sistema.
* `data/style.css` → estilos do site.
* `data/script.js` → funcionamento e atualização da interface.
* `data/data.js` → leitura dos dados do Firestore.
* `data/config.js` → configuração do projeto Firebase.

## Hardware utilizado

* ESP32
* 4 sensores IR
* 4 LEDs
* LCD I2C 16x2
* Cabos para conexão
* Wi-Fi

## Funcionamento

Os sensores IR verificam o estado das quatro vagas.

Quando uma vaga é ocupada:

1. O sensor identifica a presença do veículo.
2. O LED correspondente é acionado.
3. O LCD é atualizado.
4. A ESP32 registra o horário de entrada.
5. A informação da vaga é enviada para o Firestore.
6. Um evento de `ENTRADA` é registrado no histórico.

Quando o veículo sai:

1. O sensor identifica que a vaga está livre.
2. O LED correspondente é desligado.
3. O LCD é atualizado.
4. A ESP32 registra o horário de saída.
5. A informação da vaga é atualizada no Firestore.
6. Um evento de `SAIDA` é registrado no histórico.

A ESP32 utiliza a API REST do Cloud Firestore para realizar a comunicação com o banco. A API REST é adequada para dispositivos com recursos limitados, incluindo dispositivos IoT.

## Configuração do Wi-Fi

Abra o arquivo:

```text
Estacionamento.ino
```

Localize:

```cpp
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";
```

Substitua pelos dados da rede Wi-Fi que será utilizada pela ESP32.

Exemplo:

```cpp
const char* ssid = "NomeDaRede";
const char* password = "SenhaDaRede";
```

## Configuração do Firebase

O projeto utiliza:

```text
iotestacionamento-e2b70
```

O Firestore deve estar ativado no projeto Firebase.

As regras utilizadas estão no arquivo:

```text
firestore.rules
```

Depois de configurar o Firestore, publique essas regras no console do Firebase.

## Estrutura dos dados no Firestore

O sistema utiliza três partes principais:

```text
vagas
metadata
eventos
```

### Coleção `vagas`

São utilizados quatro documentos:

```text
vagas/vaga1
vagas/vaga2
vagas/vaga3
vagas/vaga4
```

Cada documento possui informações semelhantes a:

```json
{
  "numero": 1,
  "ocupada": false,
  "entradaAtual": null
}
```

Quando a vaga está ocupada:

```json
{
  "numero": 1,
  "ocupada": true,
  "entradaAtual": "2026-09-09T12:00:00"
}
```

### Documento `metadata/status`

Armazena informações gerais do estacionamento, como:

```json
{
  "ultimaAtualizacao": "2026-09-09T12:00:00",
  "livres": 3,
  "ocupadas": 1
}
```

### Coleção `eventos`

Armazena o histórico de entradas e saídas.

Exemplo de entrada:

```json
{
  "vaga": 2,
  "tipo": "ENTRADA",
  "dataHora": "2026-09-09T12:00:00",
  "entrada": "2026-09-09T12:00:00",
  "saida": null
}
```

Exemplo de saída:

```json
{
  "vaga": 2,
  "tipo": "SAIDA",
  "dataHora": "2026-09-09T13:00:00",
  "entrada": "2026-09-09T12:00:00",
  "saida": "2026-09-09T13:00:00"
}
```

## Site

Os arquivos do site ficam dentro da pasta:

```text
data/
```

A ESP32 utiliza o **LittleFS** para armazenar esses arquivos.

O arquivo principal é:

```text
data/index.html
```

O site apresenta:

* Estado das quatro vagas.
* Quantidade de vagas livres.
* Quantidade de vagas ocupadas.
* Taxa de ocupação.
* Estado do LCD.
* Informações da integração com o Firebase.
* Histórico de entradas e saídas.
* Tempo de permanência dos veículos.

## API local da ESP32

A ESP32 também disponibiliza endpoints locais.

### Status

```text
GET /api/status
```

Retorna informações sobre as vagas.

Exemplo:

```json
{
  "ultimaAtualizacao": "2026-09-09T12:00:00",
  "livres": 3,
  "ocupadas": 1,
  "taxaOcupacao": 25,
  "vagas": [
    {
      "numero": 1,
      "ocupada": false,
      "entradaAtual": null
    },
    {
      "numero": 2,
      "ocupada": true,
      "entradaAtual": "2026-09-09T11:55:00"
    },
    {
      "numero": 3,
      "ocupada": false,
      "entradaAtual": null
    },
    {
      "numero": 4,
      "ocupada": false,
      "entradaAtual": null
    }
  ]
}
```

### Saúde da ESP32

```text
GET /api/health
```

Esse endpoint informa se a ESP32 está conectada ao Wi-Fi.

## Como instalar o site na ESP32

Os arquivos:

```text
index.html
style.css
script.js
data.js
config.js
```

devem permanecer dentro da pasta:

```text
data/
```

Depois, os arquivos da pasta `data` devem ser enviados para o **LittleFS** da ESP32.

## Inicialização

Ao ligar a ESP32:

1. Os sensores são configurados.
2. Os LEDs são configurados.
3. O LCD é inicializado.
4. O LittleFS é montado.
5. A ESP32 tenta conectar ao Wi-Fi.
6. O horário é obtido por NTP.
7. O servidor web é iniciado.
8. O estado inicial das vagas é identificado.
9. Os dados são enviados ao Firestore.
10. O sistema começa a monitorar as vagas.

## Observações

Este projeto foi desenvolvido para fins educacionais.

O arquivo `firestore.rules` deve ser configurado de acordo com a forma de acesso escolhida para o projeto. A API REST do Firestore utiliza autenticação/autorização e pode ser controlada pelas regras do Firestore quando a requisição é feita de forma não autenticada ou com um token Firebase.

Não devem ser armazenadas informações pessoais desnecessárias no banco de dados.

## Resumo da arquitetura

```text
                 ┌──────────────┐
                 │ Sensores IR  │
                 └──────┬───────┘
                        │
                        ▼
                 ┌──────────────┐
                 │    ESP32     │
                 └──────┬───────┘
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
      ┌────────┐   ┌──────────┐  ┌───────────┐
      │  LEDs  │   │ LCD 16x2 │  │ LittleFS  │
      └────────┘   └──────────┘  └─────┬─────┘
                                       │
                                       ▼
                                  ┌──────────┐
                                  │  Site    │
                                  └──────────┘

                        ESP32
                          │
                          ▼
                 ┌─────────────────┐
                 │ Firebase        │
                 │ Cloud Firestore │
                 └────────┬────────┘
                          │
              ┌───────────┼───────────┐
              ▼           ▼           ▼
           vagas      metadata     eventos

Esse README agora corresponde à estrutura que você está usando, **sem `public/`, Vercel, `.firebaserc`, `firebase.json` ou Node.js**.
```
