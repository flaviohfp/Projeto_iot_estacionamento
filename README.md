# Estacionamento Inteligente

Sistema web completo para monitorar 4 vagas de estacionamento em modo de simulacao, preparado para receber dados reais de uma maquete com ESP32, sensores IR, RTC DS3231 e display OLED I2C.

## Arquitetura

Sensor IR Vaga 1 / 2 / 3 / 4 -> ESP32 -> Wi-Fi -> API Node.js/Express -> Firebase Firestore -> Socket.IO/Polling -> Dashboard Web

O RTC DS3231 fica conectado ao ESP32 para fornecer data e hora confiaveis. O OLED tambem fica no ESP32 e pode consumir `GET /api/status/display` para mostrar vagas livres.

## Instalar Node.js

1. Acesse https://nodejs.org/
2. Baixe a versao LTS.
3. Instale mantendo as opcoes padrao.
4. Confirme no terminal:

```bash
node -v
npm -v
```

## Instalar dependencias

```bash
npm install
```

## Configuracao

Crie um arquivo `.env` a partir do `.env.example`:

```env
PORT=3000
API_KEY=
ENABLE_API_KEY=false
FIREBASE_PROJECT_ID=
FIREBASE_CLIENT_EMAIL=
FIREBASE_PRIVATE_KEY=
```

Para projeto escolar em rede local, pode deixar `ENABLE_API_KEY=false`. Para exigir chave no futuro, use:

```env
ENABLE_API_KEY=true
API_KEY=sua-chave-aqui
```

Quando habilitado, envie a chave no cabecalho HTTP:

```http
X-API-Key: sua-chave-aqui
```

## Deploy na Vercel

O projeto ja possui `vercel.json` e `api/index.js`, entao pode ser importado na Vercel como um projeto Node.js simples.

Configuracao recomendada na Vercel:

- Framework Preset: `Other`.
- Install Command: `npm install`.
- Build Command: deixe vazio.
- Output Directory: deixe vazio.
- Node.js: 20 ou superior.

Para o deploy funcionar, configure o Firebase Firestore e adicione as variaveis do Firebase Admin na Vercel:

```env
FIREBASE_PROJECT_ID=seu-projeto
FIREBASE_CLIENT_EMAIL=firebase-adminsdk-xxxxx@seu-projeto.iam.gserviceaccount.com
FIREBASE_PRIVATE_KEY="-----BEGIN PRIVATE KEY-----\nSUA_CHAVE\n-----END PRIVATE KEY-----\n"
```

Esses dados ficam em `Firebase Console > Configuracoes do projeto > Contas de servico > Gerar nova chave privada`. Nao coloque essa chave no GitHub; use somente nas variaveis de ambiente da Vercel e no `.env` local.

Na Vercel, o painel usa polling automatico a cada 3 segundos como fallback de tempo real. Localmente, quando executado com `npm start`, ele tambem usa Socket.IO.

Depois do deploy, teste:

```http
GET https://SEU-PROJETO.vercel.app/api/health
GET https://SEU-PROJETO.vercel.app/api/vagas
```

O ESP32 devera enviar para:

```http
POST https://SEU-PROJETO.vercel.app/api/vagas/status
```

## Iniciar o servidor

```bash
npm start
```

Modo desenvolvimento, com reinicio automatico:

```bash
npm run dev
```

Acesse:

http://localhost:3000

## Como usar a simulacao

No painel `Simulacao / Testes`, alterne cada vaga entre livre e ocupada. O simulador chama a mesma API que o ESP32 usara futuramente:

```http
POST /api/vagas/status
```

Assim, a regra de entrada, saida, permanencia, banco de dados e atualizacao em tempo real e a mesma para simulacao e hardware real.

## API

### Listar status das vagas

```http
GET /api/vagas
```

Resposta:

```json
{
  "total": 4,
  "livres": 2,
  "ocupadas": 2,
  "taxaOcupacao": 50,
  "ultimaAtualizacao": "2026-08-11T14:32:00.000Z",
  "vagas": [
    {
      "numero": 1,
      "ocupada": false,
      "ultimaAtualizacao": "2026-08-11T14:30:00.000Z",
      "entradaAtual": null
    }
  ]
}
```

### Atualizar uma vaga

```http
POST /api/vagas/status
Content-Type: application/json
```

```json
{
  "vaga": 2,
  "ocupada": true,
  "timestamp": "2026-08-11T14:32:00"
}
```

O campo `timestamp` e opcional. Quando nao for enviado, o servidor usa a data e hora do computador.

Validacoes:

- aceita somente vagas 1, 2, 3 e 4;
- `ocupada` precisa ser booleano (`true` ou `false`);
- eventos repetidos nao sao gravados se o estado nao mudou.

### Historico

```http
GET /api/historico
```

Retorna os eventos mais recentes primeiro, com entrada, saida e duracao quando houver.

### Dados para OLED

```http
GET /api/status/display
```

Resposta:

```json
{
  "livres": 2,
  "vagasLivres": [1, 3]
}
```

## Banco de dados

O banco de dados usado e o Firebase Firestore.

Colecoes principais:

- `vagas`: documentos `1`, `2`, `3` e `4`, com estado atual, ultima atualizacao e entrada atual.
- `eventos`: historico persistente de entradas e saidas.
- `metadata/status`: ultima atualizacao geral recebida pela API.

Na primeira execucao, o sistema cria automaticamente os documentos das vagas 1, 2, 3 e 4 no Firestore.

## Regras de entrada e saida

- LIVRE -> OCUPADA registra evento de `ENTRADA`.
- OCUPADA -> LIVRE registra evento de `SAIDA`.
- Se o mesmo estado chegar varias vezes, apenas atualiza `ultima_atualizacao` e nao cria evento repetido.
- Quando ocorre saida, o sistema calcula a permanencia com base no horario de entrada salvo.

## Conectar o ESP32 futuramente

O ESP32 devera ler os 4 sensores IR, confirmar a estabilidade da leitura e enviar somente mudancas para:

```http
POST http://IP_DO_SERVIDOR:3000/api/vagas/status
```

Se estiver usando Vercel, troque pela URL publica do deploy:

```http
POST https://SEU-PROJETO.vercel.app/api/vagas/status
```

Veja o guia completo em `ESP32_INTEGRACAO.md`.
