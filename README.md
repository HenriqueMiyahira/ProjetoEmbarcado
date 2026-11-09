# ThermoFlow — WebServer com JSON e DHT22

Sistema de automação para controle de um ventilador usando ESP32, sensor DHT22 e interface Web, com acionamento automático (baseado na temperatura) ou manual.

Projeto da disciplina de Conectividade de Sistemas Ciberfísicos (Engenharia de Software, PUCPR).

## Funcionalidades

- Leitura de temperatura e umidade com o sensor DHT22
- Controle manual do ventilador (relé) pela interface Web
- Modo automático: liga o ventilador acima da temperatura alvo e desliga quando cai abaixo de `alvo - HYSTERESIS`
- Temperatura alvo e modo automático salvos na memória do ESP32 (`Preferences`), mantidos após reiniciar
- Histórico de temperatura/umidade gravado em `history.csv` (LittleFS)
- Registro de erros em `logs.txt` (LittleFS)
- Modo AP (`ThermoFlow_Setup`) quando o ESP32 não consegue se conectar à rede Wi-Fi salva

## Tecnologias

- ESP32
- Sensor DHT22 (temperatura e umidade)
- Relé para acionamento do ventilador
- [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) com API REST em JSON
- LittleFS para os arquivos da interface (HTML, CSS, JS) e para histórico/logs
- Frontend em HTML, CSS e JavaScript (pasta `data/`)

## Estrutura

```
ProjetoEmbarcado.ino   # setup/loop: leitura do sensor, histórico e modo automático
Hardware.h             # sensor, relé, Wi-Fi, NTP e logs
WebServer.h            # inicializa o LittleFS, arquivos estáticos e página /config
Routes.h               # mapeamento das rotas da API
src/controllers/       # um controller por recurso da API
data/                  # interface Web (thermoflow.html/.css/.js) e history.csv
Config_example         # modelo para o Config.h (ignorado pelo git)
```

## Como rodar

1. Instale o suporte ao ESP32 na Arduino IDE e as bibliotecas **DHT sensor library** e **ESPAsyncWebServer** (com **AsyncTCP**).
2. Crie o arquivo `Config.h` na raiz do projeto a partir do `Config_example`, ajustando os pinos e os tempos.
3. Envie a pasta `data/` para o LittleFS do ESP32 (plugin de upload de LittleFS da Arduino IDE).
4. Compile e envie o `ProjetoEmbarcado.ino`.
5. No primeiro uso, conecte-se à rede `ThermoFlow_Setup` (senha `12345678`), acesse `http://192.168.4.1/config` e informe a rede Wi-Fi. O ESP32 reinicia e passa a usar essa rede.
6. Acesse o IP mostrado no Monitor Serial (115200 baud) para abrir o painel.

## Ligações

| Componente      | Pino ESP32 (padrão do `Config_example`) |
|-----------------|-----------------------------------------|
| DHT22 (dados)   | GPIO 27                                  |
| Relé (sinal)    | GPIO 26                                  |

## API

| Método | Rota          | Descrição                                                        | Parâmetros               |
|--------|---------------|------------------------------------------------------------------|--------------------------|
| GET    | `/api/dht`    | Temperatura e umidade atuais (500 se o sensor falhar)            | —                        |
| GET    | `/api/fan`    | Estado atual do ventilador                                       | —                        |
| POST   | `/api/fan`    | Liga/desliga o ventilador                                        | `action=true\|false`     |
| GET    | `/api/mode`   | Estado do modo automático                                        | —                        |
| POST   | `/api/mode`   | Ativa/desativa o modo automático                                 | `action=true\|false`     |
| GET    | `/api/temp`   | Temperatura alvo                                                 | —                        |
| POST   | `/api/temp`   | Altera a temperatura alvo                                        | `?value=<número>` (query)|
| GET    | `/api/conn`   | Status da conexão (modo, IP, MAC, SSID)                          | —                        |
| POST   | `/api/conn`   | Salva a rede Wi-Fi e reinicia o ESP32                            | `ssid`, `pass`           |
| GET    | `/api/logs`   | Conteúdo do `logs.txt`                                           | —                        |
| GET    | `/history.csv`| Histórico no formato `data hora,temperatura,umidade`             | —                        |
| GET    | `/config`     | Página de configuração do Wi-Fi                                  | —                        |

Exemplo de resposta de `GET /api/dht`:

```json
{ "message": "Temperatura lida com sucesso!", "temp": 24.5, "humid": 61.0, "ok": true }
```

## Autores

- Bruno Ianzen de Oliveira
- Guilherme Eliúde Venâncio
- Gustavo da Silva Bertoncello
- Henrique Miyahira Prado
- Roberto Ariel Favarão Belo
