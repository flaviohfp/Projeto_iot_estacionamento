# Estacionamento Inteligente

Painel estático em HTML, CSS e JavaScript, sem Node.js, dependências ou compilação.
Abra `public/index.html` no navegador para usar a demonstração: quatro vagas, indicadores, aviso de lotação, histórico e tempo de permanência. A simulação fica salva neste navegador; se o armazenamento estiver bloqueado, dura apenas enquanto a página estiver aberta.

## Vercel e Firebase Hosting

Na Vercel, importe o repositório com diretório raiz do projeto e preset Other. O `vercel.json` publica `public`, sem instalação nem build. Remova eventuais overrides antigos de build nas configurações do projeto.

Para Firebase Hosting, selecione seu projeto com a CLI Firebase e execute `firebase deploy --only hosting --project SEU_PROJETO`. A CLI é apenas uma ferramenta de publicação; o site não depende dela para funcionar.

## Fonte dos dados

Edite `public/config.js`:

- `mode: "demo"`: funciona imediatamente, com simulação local.
- `mode: "firebase"`: preencha `firebaseProjectId`. Leitura do Cloud Firestore a cada três segundos. Usa as coleções do projeto antigo: `vagas`, `eventos` e `metadata/status`. Não grava simulações no banco real.
- `mode: "esp32"`: preencha `esp32Url` com o endereço da placa, sem barra final. Deixe vazio quando a própria placa hospedar os arquivos de `public`.

Para leitura pública do Firestore, revise e publique `firestore.rules` usando `firebase deploy --only firestore:rules --project SEU_PROJETO`. As regras tornam públicos somente os registros de ocupação dessas coleções e bloqueiam escrita pelo cliente. Não armazene dados pessoais nelas. A autorização de escrita da ESP32 será definida na integração futura. Nunca coloque credenciais administrativas ou chaves privadas no site.

## Contrato para a futura ESP32

A placa será responsável pelos sensores, horários, histórico e persistência. O firmware foi removido desta versão e pode ser recuperado pelo histórico Git. Não há servidor Node ou função da Vercel.

`GET /api/vagas` retorna:

```json
{
  "ultimaAtualizacao": "2026-09-09T12:00:00Z",
  "vagas": [
    { "numero": 1, "ocupada": false, "entradaAtual": null },
    { "numero": 2, "ocupada": true, "entradaAtual": "2026-09-09T11:55:00Z" },
    { "numero": 3, "ocupada": false, "entradaAtual": null },
    { "numero": 4, "ocupada": false, "entradaAtual": null }
  ]
}
```

`GET /api/historico` retorna uma lista, mais recente primeiro:

```json
[{ "vaga": 2, "tipo": "ENTRADA", "dataHora": "2026-09-09T11:55:00Z", "entrada": "2026-09-09T11:55:00Z", "saida": null }]
```

Na saída use `tipo: "SAIDA"` e preencha `saida`. O painel calcula a duração. Limite: 100 eventos. Atualize `ultimaAtualizacao` periodicamente mesmo sem mudança de ocupação; após 30 segundos o painel indica leitura desatualizada.

No Firestore, cada documento `vagas/1` até `vagas/4` usa os campos do exemplo. `metadata/status` contém `ultimaAtualizacao`; documentos em `eventos` usam os campos do histórico. Horários podem ser strings ISO ou timestamps Firestore.

Um site HTTPS na Vercel não consegue consultar livremente uma placa HTTP na rede local por restrições do navegador. Para acesso remoto, a ESP32 deverá enviar os dados ao Firebase. Para uso local, hospede o painel na placa; se as origens forem diferentes, configure CORS na API da ESP32.

Documentação: [Firestore REST](https://firebase.google.com/docs/firestore/use-rest-api) e [configuração Vercel](https://vercel.com/docs/project-configuration/vercel-json).
