'use strict';

/* ---------- Elementos ---------- */
const connectionPill = document.querySelector('#connectionPill');

const currentTempElement = document.querySelector('#temperatureValue');
const currentHumidityElement = document.querySelector('#humidityValue');

const fanToggle = document.querySelector('#fanToggle');
const fanStatus = document.querySelector('#fanStatus');
const fanVisual = document.querySelector('#fanVisual');
const fanVisualLabel = document.querySelector('#fanVisualLabel');
const fanPowerLabel = document.querySelector('#fanPowerLabel');

const targetTempInput = document.querySelector('#temperatureLimitInput');
const targetTempView = document.querySelector('#temperatureLimitValue');
const autoModeToggle = document.querySelector('#autoModeToggle');

const logList = document.querySelector('#logList');

const REFRESH_MS = 5000;        // intervalo entre atualizações
const TARGET_DEBOUNCE_MS = 2000; // espera após mexer no slider antes de enviar


/* ---------- Status de conexão (vem do resultado do fetch) ---------- */
function setConnection(isOnline) {
  connectionPill.innerText = isOnline ? 'Conectado' : 'Sem conexão';
  connectionPill.classList.toggle('state-pill--off', !isOnline);
}


/* ---------- Temperatura e umidade ---------- */
async function updateTemp() {
  try {
    const response = await fetch('/api/dht');
    const data = await response.json();
    setConnection(true);

    if (!data.ok) {
      console.error(data.message);
      return;
    }
    if (data.temp != null) {
      currentTempElement.innerText = Number(data.temp).toFixed(1);
    }
    if (data.humid != null) {
      currentHumidityElement.innerText = data.humid;
    }
  }
  catch (err) {
    setConnection(false);
    console.error(`Erro ao buscar dados do ambiente: ${err}`);
  }
}


/* ---------- Ventilador ---------- */
function showFanState(isOn) {
  fanStatus.innerText = isOn ? 'LIGADO' : 'DESLIGADO';
  fanStatus.classList.toggle('state-pill--off', !isOn);
  fanVisual.classList.toggle('is-on', isOn);
  fanVisualLabel.innerText = isOn ? 'ATIVO' : 'EM ESPERA';
  fanPowerLabel.innerText = isOn ? 'Ventilador ligado' : 'Ventilador desligado';
}

async function turnOnFan(action) {
  fanToggle.disabled = true; // evita toques repetidos enquanto o ESP32 responde
  try {
    const response = await fetch(`/api/fan?action=${action}`);
    const data = await response.json();
    if (!data.ok) throw new Error(data.message || 'resposta inválida');

    showFanState(action); // só atualiza a tela depois que o ESP32 confirmou
  }
  catch (err) {
    console.error(`Erro ao alterar o ventilador: ${err}`);
    fanToggle.checked = !action; // volta a chave para o estado anterior
    showFanState(!action);
  }
  finally {
    fanToggle.disabled = false;
  }
}


/* ---------- Temperatura desejada ---------- */
async function alterTargetTemp(value) {
  try {
    const response = await fetch(`/api/targettemp?value=${value}`);
    const data = await response.json();

    if (!data.ok) {
      console.error(data.message);
      if (data.temp != null) { // volta para o valor que o ESP32 está usando
        targetTempInput.value = data.temp;
        targetTempView.innerText = data.temp;
      }
    }
  }
  catch (err) {
    console.error(`Erro ao alterar temperatura alvo: ${err}`);
  }
}


/* ---------- Modo automático ---------- */
async function autoMode(action) {
  autoModeToggle.disabled = true;
  try {
    const response = await fetch(`/api/mode?action=${action}`);
    const data = await response.json();
    if (!data.ok) throw new Error(data.message || 'resposta inválida');
  }
  catch (err) {
    console.error(`Erro ao trocar de modo: ${err}`);
    autoModeToggle.checked = !action;
  }
  finally {
    autoModeToggle.disabled = false;
  }
}


/* ---------- Logs: /api/logs devolve UMA STRING, um log por linha ---------- */
let lastLogs = null;

async function updateLogs() {
  try {
    const response = await fetch('/api/logs');
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const text = await response.text();

    if (text === lastLogs) return; // nada novo, não mexe na tela
    lastLogs = text;

    const fragment = document.createDocumentFragment();
    text.split(/\r?\n/).forEach((line) => {
      if (line.trim() === '') return;
      const item = document.createElement('li');
      item.textContent = line;      // textContent: o log nunca é interpretado como HTML
      fragment.appendChild(item);
    });
    logList.replaceChildren(fragment); // um log em cima do outro
  }
  catch (err) {
    console.error(`Erro ao buscar logs: ${err}`);
  }
}


/* ---------- Conexão: /api/network → { ok, type, ssid, ip, mac } ---------- */
async function updateNetwork() {
  try {
    const response = await fetch('/api/network');
    const data = await response.json();
    if (!data.ok) {
      console.error(data.message);
      return;
    }
    document.querySelector('#networkType').innerText = data.type || '—';
    document.querySelector('#networkName').innerText = data.ssid || '—';
    document.querySelector('#networkIp').innerText = data.ip || '—';
    document.querySelector('#networkMac').innerText = data.mac || '—';
  }
  catch (err) {
    console.error(`Erro ao buscar dados da rede: ${err}`);
  }
}


/* ---------- Eventos ---------- */
fanToggle.addEventListener('change', (e) => turnOnFan(e.target.checked));
autoModeToggle.addEventListener('change', (e) => autoMode(e.target.checked));

let timer;
targetTempInput.addEventListener('input', (e) => {
  const currentValue = e.target.value;
  targetTempView.innerText = currentValue; // atualização visual instantânea

  clearTimeout(timer); // debounce: só envia quando parar de arrastar
  timer = setTimeout(() => alterTargetTemp(currentValue), TARGET_DEBOUNCE_MS);
});


/* ---------- Atualização periódica ----------
   Uma requisição por vez (o ESP32 não gosta de várias ao mesmo tempo) e só
   agenda a próxima depois que a anterior terminou. Pausa com a aba em segundo plano.
   Os logs são buscados a cada 2 ciclos (10 s). */
let cycle = 0;

async function refresh() {
  if (!document.hidden) {
    await updateTemp();
    if (cycle % 2 === 0) await updateLogs();
    cycle++;
  }
  setTimeout(refresh, REFRESH_MS);
}

(async () => {
  await updateNetwork(); // só uma vez ao abrir a página
  refresh();
})();
