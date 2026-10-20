/*
  ESP32 + DHT22 + RELE + VENTOINHA - CODIGO UNIFICADO

  Interface: Ventilador_Rele_Aut_Man.ino
  Leitura do DHT22: Ventilador_ESP32_Corrigido(1).ino

  LIGACOES (com o circuito DESLIGADO):
  DHT22: VCC -> 3V3, DATA -> GPIO 4, GND -> GND.
  Rele de 5 V compativel com entrada de 3,3 V:
    VCC -> 5 V, GND -> GND do ESP32, IN -> GPIO 26.
  Ventoinha DC alimentada por fonte separada adequada:
    Fonte (+) -> COM do rele; NO -> ventoinha (+);
    ventoinha (-) -> fonte (-).

  Nunca alimente a ventoinha com GPIO do ESP32 nem ligue 12 V no ESP32.
  O ESP32 apenas COMANDA o rele: o estado da pagina nao mede
  a corrente, a tensao ou se a ventoinha realmente esta girando.

  BIBLIOTECAS: DHT sensor library (Adafruit) + Adafruit Unified Sensor.
  Placa: ESP32 Dev Module (core esp32 by Espressif Systems).
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <math.h>

// ===== CONFIGURACAO =====
// Mantidos os dados de Wi-Fi presentes no projeto Rele_Aut_Man.
const char* WIFI_SSID = "teste";
const char* WIFI_SENHA = "1234567890";

constexpr uint8_t PINO_DHT = 4;
constexpr uint8_t PINO_RELE = 26;

// Mesmo acionamento de rele do arquivo "Corrigido": LOW = LIGADO.
// Caso o SEU modulo seja ativo em HIGH, troque true por false.
const bool RELE_ATIVO_EM_BAIXO = true;

// Preservado o limite de 17 C usado no projeto da interface.
// Altere para 25.0, 28.0, 30.0 etc. se desejar.
const float TEMPERATURA_LIGAR = 17.0;
const float HISTERESE = 0.5; // Desliga a 16,5 C (evita liga/desliga repetidos).

constexpr unsigned long INTERVALO_DHT_MS = 2500;
constexpr unsigned long INTERVALO_RECONEXAO_MS = 10000;
constexpr unsigned long DURACAO_TESTE_MS = 3000;
// ========================

DHT dht(PINO_DHT, DHT22); // IMPORTANTE: sensor e DHT22, nao DHT11.
WebServer server(80);

bool ventState = false;      // Estado enviado ao pino (nao sensor fisico da fan).
bool controlManual = true;   // Inicia em MANUAL/DESLIGADO, para teste seguro.
bool testeAtivo = false;
unsigned long inicioTeste = 0;
float temperatura = NAN;
float umidade = NAN;
unsigned long ultimaLeitura = 0;
unsigned long ultimaReconexao = 0;

// Todas as mudancas do rele passam por esta funcao.
void definirVentoinha(bool ligar) {
  const uint8_t nivel = ligar
    ? (RELE_ATIVO_EM_BAIXO ? LOW : HIGH)
    : (RELE_ATIVO_EM_BAIXO ? HIGH : LOW);
  digitalWrite(PINO_RELE, nivel);
  ventState = ligar;
  Serial.printf("[FAN] Comando %s | GPIO %d = %s | modo %s\n",
                ligar ? "LIGAR" : "DESLIGAR", (int)PINO_RELE,
                nivel == HIGH ? "HIGH" : "LOW",
                controlManual ? "MANUAL" : "AUTOMATICO");
}

void atualizarAutomatico() {
  if (controlManual) return;

  // Se o sensor falhar, desliga por seguranca, sem usar leitura invalida.
  if (isnan(temperatura)) {
    if (ventState) definirVentoinha(false);
    return;
  }

  if (!ventState && temperatura > TEMPERATURA_LIGAR) {
    definirVentoinha(true);
  } else if (ventState && temperatura <= TEMPERATURA_LIGAR - HISTERESE) {
    definirVentoinha(false);
  }
}

void lerDHT22() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    temperatura = NAN;
    umidade = NAN;
    Serial.println("[DHT22] Falha de leitura; confira conexoes.");
  } else {
    temperatura = t;
    umidade = h;
    Serial.printf("[DHT22] Temperatura %.1f C | Umidade %.1f %%\n", t, h);
  }
  atualizarAutomatico();
}

void enviarJSON(int codigo, const String& corpo) {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(codigo, "application/json; charset=utf-8", corpo);
}

void gestionarDatos() {
  String json = "{\"temperature\":";
  json += isnan(temperatura) ? "null" : String(temperatura, 1);
  json += ",\"humidity\":";
  json += isnan(umidade) ? "null" : String(umidade, 1);
  json += ",\"ventState\":";
  json += ventState ? "true" : "false";
  json += ",\"manual\":";
  json += controlManual ? "true" : "false";
  json += ",\"teste\":";
  json += testeAtivo ? "true" : "false";
  json += "}";
  enviarJSON(200, json);
}

void gestionarAlternarVenti() {
  if (!server.hasArg("state")) {
    enviarJSON(400, "{\"success\":false,\"message\":\"Use state=on ou state=off\"}");
    return;
  }
  String val = server.arg("state");
  if (val != "on" && val != "off") {
    enviarJSON(400, "{\"success\":false,\"message\":\"Estado invalido\"}");
    return;
  }

  testeAtivo = false; // Novo comando manual cancela teste temporario.
  controlManual = true; // Nao pode ser sobrescrito pelo controle automatico.
  definirVentoinha(val == "on");
  enviarJSON(200, "{\"success\":true}");
}

void gestionarControlAuto() {
  testeAtivo = false;
  controlManual = false;
  Serial.println("[MODO] Controle automatico ativado.");
  atualizarAutomatico(); // Aplica ultima leitura sem esperar nova consulta web.
  enviarJSON(200, "{\"success\":true}");
}

// Teste temporario para verificar se o modulo realmente comuta.
void gestionarTesteRele() {
  controlManual = true;
  testeAtivo = true;
  inicioTeste = millis();
  definirVentoinha(true);
  enviarJSON(200, "{\"success\":true,\"message\":\"Teste de 3 segundos iniciado\"}");
}

// Interface preservada do arquivo Rele_Aut_Man (cores, interruptor e botao automatico).
const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Control Ventilador ESP32</title>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background-color: #0f1117;
      color: #e2e8f0;
      text-align: center;
      padding: 40px;
    }
    h1 {
      color: #00c896;
      margin-bottom: 20px;
    }
    .card {
      background-color: #1a1c23;
      border-radius: 16px;
      padding: 30px;
      max-width: 400px;
      margin: 0 auto;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
    .data {
      font-size: 1.2rem;
      margin: 10px 0;
      color: #cbd5e1;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
      margin-top: 10px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #555;
      transition: .4s;
      border-radius: 34px;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 26px; width: 26px;
      left: 4px; bottom: 4px;
      background-color: #fff;
      transition: .4s;
      border-radius: 50%;
    }
    input:checked + .slider {
      background-color: #00c896;
    }
    input:checked + .slider:before {
      transform: translateX(26px);
    }
    .labelSwitch {
      margin-top: 10px;
      font-weight: bold;
      color: #94a3b8;
    }
    .status-container {
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
      margin-top: 20px;
    }
    .status-dot {
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background-color: #555;
      transition: background-color 0.3s ease;
      box-shadow: 0 0 6px rgba(0, 0, 0, 0.4);
    }
    .status-dot.on {
      background-color: #00c896;
      box-shadow: 0 0 10px #00c896aa;
    }
    .status-badge {
      padding: 6px 14px;
      border-radius: 20px;
      font-weight: 600;
      font-size: 0.95rem;
      transition: background-color 0.3s ease;
      background-color: #333;
      color: #ccc;
    }
    .status-badge.on {
      background-color: #00c896;
      color: #fff;
    }
    button#autoBtn {
      margin-top: 20px;
      padding: 10px 20px;
      background-color: #00c896;
      border: none;
      border-radius: 20px;
      color: white;
      font-weight: bold;
      cursor: pointer;
      transition: background-color 0.3s ease;
    }
    button#autoBtn:hover {
      background-color: #019e6a;
    }
    button#testBtn {
      margin-top: 12px;
      background-color: #303743;
      color: #e2e8f0;
      border: 1px solid #64748b;
      border-radius: 20px;
      padding: 9px 18px;
      cursor: pointer;
    }
    button:disabled { opacity: .6; cursor: wait; }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 Controle de fan</h1>
    <div class="data">🌡️ Temperatura: <span id="temp">--</span> °C</div>
    <div class="data">💧 Umidade: <span id="hum">--</span> %</div>

    <div class="status-container">
      <div id="fanDot" class="status-dot off"></div>
      <div id="fanStatus" class="status-badge off">Ventoinha desligada</div>
    </div>

    <label class="switch">
      <input type="checkbox" id="toggleSwitch">
      <span class="slider"></span>
    </label>
    <div class="labelSwitch">Controle Manual</div>

    <button id="autoBtn">Voltar ao Controle Automático</button>
    <div><button id="testBtn">Testar relé (3 segundos)</button></div>
    <div id="modeInfo" class="labelSwitch">Modo: aguardando ESP32</div>
    <div id="msg" style="color:#94a3b8;font-size:0.88rem;margin-top:9px;min-height:16px"></div>
  </div>

  <script>
    // O estado exibido sempre vem do ESP32 (fonte unica de verdade).
    const tempEl = document.getElementById('temp');
    const humEl = document.getElementById('hum');
    const fanStatus = document.getElementById('fanStatus');
    const fanDot = document.getElementById('fanDot');
    const toggleSwitch = document.getElementById('toggleSwitch');
    const modeInfo = document.getElementById('modeInfo');
    const msg = document.getElementById('msg');

    let enviando = false;

    function renderizar(dados) {
      tempEl.textContent = typeof dados.temperature === 'number'
        ? dados.temperature.toFixed(1) : '--';
      humEl.textContent = typeof dados.humidity === 'number'
        ? dados.humidity.toFixed(1) : '--';

      const ligado = dados.ventState === true;
      fanStatus.textContent = ligado ? 'Ventoinha ligada (comando)' : 'Ventoinha desligada (comando)';
      fanStatus.classList.toggle('on', ligado);
      fanDot.classList.toggle('on', ligado);
      if (!enviando) toggleSwitch.checked = ligado;
      modeInfo.textContent = 'Modo: ' + (dados.teste ? 'Teste temporário' : (dados.manual ? 'Manual' : 'Automático'));
    }

    async function fetchData() {
      try {
        const res = await fetch('/data', { cache: 'no-store' });
        if (!res.ok) throw new Error('Erro HTTP ' + res.status);
        const dados = await res.json();
        renderizar(dados);
      } catch (erro) {
        modeInfo.textContent = 'ESP32 sem comunicação';
        console.error('Falha ao atualizar dados:', erro);
      }
    }

    toggleSwitch.addEventListener('change', async (evento) => {
      const ligar = evento.target.checked;
      const state = ligar ? 'on' : 'off';
      enviando = true;
      toggleSwitch.disabled = true;
      msg.textContent = 'Enviando comando...';
      try {
        const res = await fetch(`/fan?state=${state}`, { cache: 'no-store' });
        const dados = await res.json();
        if (!res.ok || dados.success !== true) {
          throw new Error(dados.message || 'Comando recusado');
        }
        msg.textContent = ligar ? 'Comando para ligar enviado.' : 'Comando para desligar enviado.';
      } catch (erro) {
        msg.textContent = 'Erro: ' + erro.message;
      } finally {
        enviando = false;
        toggleSwitch.disabled = false;
        await fetchData();
      }
    });

    document.getElementById('autoBtn').addEventListener('click', async () => {
      msg.textContent = 'Ativando modo automático...';
      try {
        const res = await fetch('/auto', { cache: 'no-store' });
        const dados = await res.json();
        if (!res.ok || dados.success !== true) {
          throw new Error(dados.message || 'Falha ao mudar modo');
        }
        msg.textContent = 'Controle automático ativado.';
      } catch (erro) {
        msg.textContent = 'Erro: ' + erro.message;
      } finally {
        await fetchData();
      }
    });

    document.getElementById('testBtn').addEventListener('click', async () => {
      msg.textContent = 'Testando relé por 3 segundos...';
      try {
        const res = await fetch('/teste', { cache: 'no-store' });
        const dados = await res.json();
        if (!res.ok || dados.success !== true) {
          throw new Error(dados.message || 'Falha ao iniciar teste');
        }
        msg.textContent = 'Relé acionado por 3 s; observe o clique e a ventoinha.';
      } catch (erro) {
        msg.textContent = 'Erro: ' + erro.message;
      } finally {
        await fetchData();
      }
    });

    fetchData();
    setInterval(fetchData, 2500);
  </script>
</body>
</html>
  )rawliteral";

void gestionarRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGINA);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 DHT22 + VENTOINHA: PROJETO UNIFICADO ===");

  // Seleciona nivel de desligado ANTES de habilitar a saida.
  digitalWrite(PINO_RELE, RELE_ATIVO_EM_BAIXO ? HIGH : LOW);
  pinMode(PINO_RELE, OUTPUT);
  definirVentoinha(false);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  Serial.print("[WIFI] Conectando");
  unsigned long comeco = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - comeco < 15000UL) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WEB] Abra http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Ainda sem conexao. Confira credenciais e rede 2,4 GHz.");
  }

  server.on("/", HTTP_GET, gestionarRoot);
  server.on("/data", HTTP_GET, gestionarDatos);
  server.on("/fan", HTTP_GET, gestionarAlternarVenti);
  server.on("/auto", HTTP_GET, gestionarControlAuto);
  server.on("/teste", HTTP_GET, gestionarTesteRele);
  server.onNotFound([]() { server.send(404, "text/plain", "Rota nao encontrada"); });
  server.begin();

  // Primeira leitura assim que loop() iniciar.
  ultimaLeitura = millis() - INTERVALO_DHT_MS;
  ultimaReconexao = millis();
}

void loop() {
  server.handleClient();

  const unsigned long agora = millis();
  if (testeAtivo && (agora - inicioTeste >= DURACAO_TESTE_MS)) {
    testeAtivo = false;
    definirVentoinha(false);
    Serial.println("[TESTE] Teste finalizado: rele desligado.");
  }
  if (agora - ultimaLeitura >= INTERVALO_DHT_MS) {
    ultimaLeitura = agora;
    lerDHT22();
  }

  if (WiFi.status() != WL_CONNECTED &&
      agora - ultimaReconexao >= INTERVALO_RECONEXAO_MS) {
    ultimaReconexao = agora;
    Serial.println("[WIFI] Reconectando...");
    WiFi.reconnect();
  }
}
