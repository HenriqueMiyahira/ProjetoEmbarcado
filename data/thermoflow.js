'use strict';

/* ThermoFlow - painel do ESP32 (rotas: /api/dht, /api/fan, /api/mode, /api/temp, /api/conn, /api/logs, /history.csv) */

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

const networkStatus = document.querySelector('#networkStatus');
const networkType = document.querySelector('#networkType');
const networkName = document.querySelector('#networkName');
const networkIp = document.querySelector('#networkIp');
const networkMac = document.querySelector('#networkMac');

const historyPeriod = document.querySelector('#historyPeriod');
const historyChart = document.querySelector('#historyChart');
const historyStatus = document.querySelector('#historyStatus');
const logList = document.querySelector('#logList');

/* ---------- Configurações ---------- */
const REFRESH_MS = 5000;          // um ciclo de atualização a cada 5 s
const TARGET_DEBOUNCE_MS = 2000;  // espera após mexer no slider antes de enviar
const LOGS_EVERY = 2;             // logs: a cada 2 ciclos (10 s)
const HISTORY_EVERY = 60;         // gráfico: a cada 60 ciclos (5 min)
// Campos do JSON onde o ESP32 pode mandar o valor (só usados se a resposta for JSON)
const TARGET_KEYS = ['temp', 'targetTemp', 'target', 'value', 'message'];
const MODE_KEYS = ['mode', 'autoMode', 'auto', 'value', 'message'];
const HISTORY_DAYS = 183;         // o gráfico mostra no máximo ~6 meses
const MIN_VALID_TIME = 946684800; // 01/01/2000 em segundos: antes disso o relógio do ESP32 não estava acertado
const HISTORY_MAX_POINTS = 300;   // pontos desenhados no gráfico (o restante vira média)
const CHART_HEIGHT = 220;


/* ---------- Funções de rede ---------- */

// POST com corpo form-urlencoded (é o que o ESP32 lê em hasParam("...", true))
async function postForm(url, params) {
  const response = await fetch(url, { method: 'POST', body: new URLSearchParams(params) });
  return response.json();
}

// Tira um número da resposta de um GET, seja ela "24.50" (texto puro) ou JSON como {"temp": 24.5, "ok": true}.
// `keys` são os campos do JSON que podem conter o valor, em ordem de prioridade.
function extractNumber(text, keys) {
  let data;
  try { data = JSON.parse(text); } catch (err) { data = text; }

  if (data !== null && typeof data === 'object') {
    if (data.ok === false) return NaN; // o ESP32 avisou que deu erro
    for (const key of keys) {
      const field = data[key];
      if (field === undefined || field === null || field === '') continue;
      const number = typeof field === 'string' ? parseFloat(field.replace(',', '.')) : Number(field);
      if (Number.isFinite(number)) return number;
    }
    return NaN;
  }
  if (typeof data === 'string') return parseFloat(data.replace(',', '.')); // "24,5" também vale
  return Number(data); // número (24.5) ou booleano (true/false)
}

async function getNumber(url, keys) {
  const response = await fetch(url);
  const text = await response.text();
  const value = extractNumber(text, keys);
  if (!response.ok || Number.isNaN(value)) {
    // mostra no console exatamente o que o ESP32 respondeu, para facilitar o diagnóstico
    throw new Error(`resposta inesperada de ${url} (HTTP ${response.status}): ${text.slice(0, 80)}`);
  }
  return value;
}


/* ---------- Status de conexão (vem do resultado do fetch) ---------- */
function setConnection(isOnline) {
  connectionPill.innerText = isOnline ? 'Conectado' : 'Sem conexão';
  connectionPill.classList.toggle('state-pill--off', !isOnline);
}


/* ---------- Temperatura e umidade: GET /api/dht ---------- */
async function updateTemp() {
  try {
    const response = await fetch('/api/dht'); // o ESP32 responde 500 + JSON se o sensor falhar
    const data = await response.json();
    setConnection(true);

    if (!data.ok) {
      console.error(data.message);
      currentTempElement.innerText = '--';
      currentHumidityElement.innerText = '--';
      return;
    }
    if (data.temp != null) {
      currentTempElement.innerText = Number(data.temp).toFixed(1);
    }
    if (data.humid != null) {
      currentHumidityElement.innerText = Math.round(Number(data.humid));
    }
  }
  catch (err) {
    setConnection(false);
    console.error(`Erro ao buscar dados do ambiente: ${err}`);
  }
}


/* ---------- Ventilador: GET e POST /api/fan ---------- */
function showFanState(isOn) {
  fanStatus.innerText = isOn ? 'LIGADO' : 'DESLIGADO';
  fanStatus.classList.toggle('state-pill--off', !isOn);
  fanVisual.classList.toggle('is-on', isOn);
  fanVisualLabel.innerText = isOn ? 'ATIVO' : 'EM ESPERA';
  fanPowerLabel.innerText = isOn ? 'Ventilador ligado' : 'Ventilador desligado';
}

// Lê o estado real do relé (no modo automático o próprio ESP32 liga/desliga)
async function updateFan() {
  try {
    const response = await fetch('/api/fan');
    const data = await response.json(); // { message: 1 ou 0, ok: true }
    if (fanToggle.disabled || !data.ok) return; // usuário está no meio de uma troca
    const isOn = Number(data.message) === 1;
    fanToggle.checked = isOn;
    showFanState(isOn);
  }
  catch (err) {
    console.error(`Erro ao ler o ventilador: ${err}`);
  }
}

async function turnOnFan(action) {
  fanToggle.disabled = true; // evita toques repetidos enquanto o ESP32 responde
  try {
    const data = await postForm('/api/fan', { action: String(action) });
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


/* ---------- Temperatura desejada: GET e POST /api/temp ---------- */
async function loadTargetTemp() {
  try {
    const value = await getNumber('/api/temp', TARGET_KEYS);
    targetTempInput.value = value;
    targetTempView.innerText = value;
  }
  catch (err) {
    console.error(`Erro ao ler temperatura alvo: ${err}`);
  }
}

async function alterTargetTemp(value) {
  try {
    // o controller lê "value" da query string (hasParam("value")), por isso vai na URL
    const data = await postForm(`/api/temp?value=${encodeURIComponent(value)}`, {});

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


/* ---------- Modo automático: GET e POST /api/mode ---------- */
async function loadAutoMode() {
  try {
    autoModeToggle.checked = (await getNumber('/api/mode', MODE_KEYS)) === 1;
  }
  catch (err) {
    console.error(`Erro ao ler o modo: ${err}`);
  }
}

async function autoMode(action) {
  autoModeToggle.disabled = true;
  try {
    const data = await postForm('/api/mode', { action: String(action) });
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


/* ---------- Conexão: GET /api/conn → { ok, status, mode, ip, mac, ssid? } ---------- */
async function updateNetwork() {
  try {
    const response = await fetch('/api/conn');
    const data = await response.json();
    if (!data.ok) {
      console.error(data.message);
      return;
    }
    networkStatus.innerText = data.status || '—';
    networkType.innerText = data.mode || '—';
    networkName.innerText = data.ssid || '—'; // só aparece se o ESP32 enviar "ssid"
    networkIp.innerText = data.ip || '—';
    networkMac.innerText = data.mac || '—';
  }
  catch (err) {
    console.error(`Erro ao buscar dados da rede: ${err}`);
  }
}


/* ---------- Logs: GET /api/logs devolve UMA STRING, um log por linha ---------- */
let lastLogs = null;

async function updateLogs() {
  try {
    const response = await fetch('/api/logs');
    // 404 = ainda não existe arquivo de log (o ESP32 responde "Nenhum log encontrado.")
    if (!response.ok && response.status !== 404) throw new Error(`HTTP ${response.status}`);
    const text = response.ok ? await response.text() : '';

    if (text === lastLogs) return; // nada novo, não mexe na tela
    lastLogs = text;

    const fragment = document.createDocumentFragment();
    text.split(/\r?\n/).forEach((line) => {
      if (line.trim() === '') return;
      const item = document.createElement('li');
      item.textContent = line; // textContent: o log nunca é interpretado como HTML
      fragment.appendChild(item);
    });
    logList.replaceChildren(fragment); // um log em cima do outro
  }
  catch (err) {
    console.error(`Erro ao buscar logs: ${err}`);
  }
}


/* ---------- Gráfico: /history.csv  →  "timestamp,temp,humid" (uma linha por leitura) ---------- */

// Aceita: epoch (segundos ou ms), "AAAA-MM-DD HH:MM:SS" e "DD/MM/AAAA HH:MM:SS". Devolve segundos ou NaN.
function parseTimestamp(text) {
  const t = text.trim();
  if (/^\d+$/.test(t)) {
    const n = Number(t);
    return n > 1e11 ? n / 1000 : n;
  }
  const br = t.match(/^(\d{2})\/(\d{2})\/(\d{4})[ T](\d{2}):(\d{2})(?::(\d{2}))?$/);
  if (br) return new Date(br[3], br[2] - 1, br[1], br[4], br[5], br[6] || 0).getTime() / 1000;
  return Date.parse(t.replace(' ', 'T')) / 1000; // ISO sem fuso = horário local
}

function toNumber(text) {
  const n = parseFloat(text);
  return Number.isFinite(n) ? n : null; // "nan" do sensor vira buraco na linha
}

// Converte o CSV no formato do uPlot: [tempos, temperaturas, umidades]
function parseHistory(csv) {
  const rows = [];
  csv.split(/\r?\n/).forEach((line) => {
    const [stamp, temp, humid] = line.split(',');
    const time = parseTimestamp(stamp || '');
    // ignora linha vazia, cabeçalho, escrita pela metade e data de 1970 (relógio do ESP32 ainda sem hora)
    if (!(time > MIN_VALID_TIME)) return;
    rows.push([time, toNumber(temp), toNumber(humid)]);
  });
  if (rows.length === 0) return null;

  rows.sort((a, b) => a[0] - b[0]); // o uPlot exige o tempo em ordem crescente
  const cutoff = rows[rows.length - 1][0] - HISTORY_DAYS * 86400;
  const kept = rows.filter((row) => row[0] >= cutoff); // limita a ~6 meses
  return [kept.map((r) => r[0]), kept.map((r) => r[1]), kept.map((r) => r[2])];
}

const dateTimeFormat = new Intl.DateTimeFormat('pt-BR', { day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit' });
const hourFormat = new Intl.DateTimeFormat('pt-BR', { hour: '2-digit', minute: '2-digit' });
const dayFormat = new Intl.DateTimeFormat('pt-BR', { day: '2-digit', month: '2-digit' });

function chartOptions() {
  const grid = { stroke: 'rgba(255,255,255,0.1)', width: 1 };
  return {
    width: historyChart.clientWidth,
    height: CHART_HEIGHT,
    cursor: { drag: { x: false, y: false } }, // sem zoom por arrasto: não atrapalha a rolagem no celular
    scales: { humid: { range: [0, 100] } },
    series: [
      { label: 'Data', value: (u, v) => (v == null ? '--' : dateTimeFormat.format(v * 1000)) },
      { label: 'Temp.', scale: 'temp', stroke: '#29BADB', width: 2, value: (u, v) => (v == null ? '--' : `${v.toFixed(1)} °C`) },
      { label: 'Umid.', scale: 'humid', stroke: '#D9D9D9', width: 2, dash: [4, 4], value: (u, v) => (v == null ? '--' : `${v.toFixed(0)} %`) },
    ],
    axes: [
      {
        stroke: '#BFBFBF', grid, ticks: grid,
        values: (u, ticks) => { // até 2 dias: mostra a hora; acima disso: o dia
          const format = ticks[ticks.length - 1] - ticks[0] <= 2 * 86400 ? hourFormat : dayFormat;
          return ticks.map((t) => format.format(t * 1000));
        },
      },
      { scale: 'temp', stroke: '#29BADB', grid, ticks: grid, size: 46, values: (u, ticks) => ticks.map((v) => `${v}°`) },
      { scale: 'humid', side: 1, stroke: '#BFBFBF', grid: { show: false }, ticks: grid, size: 46, values: (u, ticks) => ticks.map((v) => `${v}%`) },
    ],
  };
}

let historyPlot = null;
let historyData = null; // CSV já convertido (até ~6 meses), na memória do navegador
let lastHistory = null;

// Reduz para no máximo ~`maxPoints` pontos tirando a média de cada faixa de tempo
// (sem isso, meses de leituras viram uma faixa sólida no celular)
function downsample(data, maxPoints) {
  const times = data[0];
  if (times.length <= maxPoints) return data;

  let step = (times[times.length - 1] - times[0]) / maxPoints; // segundos por ponto
  if (step > 6 * 3600) step = Math.ceil(step / 86400) * 86400; // faixa longa: dias inteiros (some o ciclo dia/noite)

  const result = [[], [], []];
  let key = null;
  let time = 0;
  let count = 0;
  let sum = [0, 0];
  let n = [0, 0];

  function flush() {
    if (count === 0) return;
    result[0].push(time / count);
    result[1].push(n[0] ? sum[0] / n[0] : null);
    result[2].push(n[1] ? sum[1] / n[1] : null);
  }

  times.forEach((t, i) => {
    const bucket = Math.floor((t - times[0]) / step);
    if (bucket !== key) {
      flush();
      key = bucket; time = 0; count = 0; sum = [0, 0]; n = [0, 0];
    }
    time += t;
    count++;
    for (let c = 0; c < 2; c++) {
      const value = data[c + 1][i];
      if (value !== null) { sum[c] += value; n[c]++; }
    }
  });
  flush();
  return result;
}

function drawHistory(data) {
  if (typeof uPlot === 'undefined') {
    // A lib vem do CDN e pode demorar: quando a página terminar de carregar, o evento "load" (abaixo) desenha de novo.
    // Se já terminou e a lib não veio, o celular está sem internet.
    if (document.readyState === 'complete') historyStatus.innerText = 'Gráfico indisponível (sem internet no celular)';
    return;
  }
  historyStatus.innerText = '';
  historyChart.hidden = false;
  if (historyPlot) historyPlot.setData(data);
  else historyPlot = new uPlot(chartOptions(), data, historyChart);
}

// Filtra pelo período escolhido no <select> e desenha
function showHistory() {
  if (!historyData) return;
  const times = historyData[0];
  const cutoff = times[times.length - 1] - Number(historyPeriod.value) * 86400;
  const first = times.findIndex((t) => t >= cutoff);
  const slice = historyData.map((column) => column.slice(first));

  if (slice[0].length < 2) {
    historyChart.hidden = true;
    historyStatus.innerText = 'Sem dados neste período';
    return;
  }
  drawHistory(downsample(slice, HISTORY_MAX_POINTS));
}

async function updateHistory() {
  try {
    const response = await fetch('/history.csv', { cache: 'no-store' });
    if (response.status === 404) {
      if (!historyPlot) historyStatus.innerText = 'Sem dados no histórico';
      return;
    }
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const text = await response.text();

    if (text === lastHistory) return; // arquivo igual, não redesenha
    lastHistory = text;

    const data = parseHistory(text);
    if (!data || data[0].length < 2) {
      if (!historyPlot) historyStatus.innerText = 'Sem dados no histórico';
      return;
    }
    historyData = data;
    showHistory();
  }
  catch (err) {
    console.error(`Erro ao buscar histórico: ${err}`);
    if (!historyPlot) historyStatus.innerText = 'Erro ao carregar o histórico';
  }
}

window.addEventListener('load', showHistory); // a lib do CDN já chegou (ou falhou): tenta desenhar

window.addEventListener('resize', () => {
  if (historyPlot) historyPlot.setSize({ width: historyChart.clientWidth, height: CHART_HEIGHT });
});


/* ---------- Eventos ---------- */
fanToggle.addEventListener('change', (e) => turnOnFan(e.target.checked));
autoModeToggle.addEventListener('change', (e) => autoMode(e.target.checked));
historyPeriod.addEventListener('change', showHistory); // só filtra o que já foi baixado

let timer;
targetTempInput.addEventListener('input', (e) => {
  const currentValue = e.target.value;
  targetTempView.innerText = currentValue; // atualização visual instantânea

  clearTimeout(timer); // debounce: só envia quando parar de arrastar
  timer = setTimeout(() => alterTargetTemp(currentValue), TARGET_DEBOUNCE_MS);
});


/* ---------- Atualização periódica ----------
   Uma requisição por vez (o ESP32 não gosta de várias ao mesmo tempo) e só
   agenda o próximo ciclo depois que o anterior terminou. Pausa com a aba em segundo plano. */
let cycle = 0;

async function refresh() {
  if (!document.hidden) {
    await updateTemp();
    await updateFan();
    if (cycle % LOGS_EVERY === 0) await updateLogs();
    if (cycle % HISTORY_EVERY === 0) await updateHistory();
    cycle++;
  }
  setTimeout(refresh, REFRESH_MS);
}

// Ao abrir a página: lê o que muda pouco (uma vez só) e começa o ciclo
(async () => {
  await updateNetwork();
  await loadAutoMode();
  await loadTargetTemp();
  refresh();
})();
