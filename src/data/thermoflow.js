const currentTempElement = document.querySelector("#temperatureValue");
const currentHumidityElement = document.querySelector('#humidityValue');

async function updateTemp() {
  try {
    const response = await fetch('/api/dht');
    const data = await response.json();
    if (!data.ok) {
      console.error(data.message);
      return;
    }
    if (data.temp) {
      currentTempElement.innerText = `${data.temp.toFixed(1)}`;
    }
    if (data.humid) {
      currentHumidityElement.innerText = `${data.humid}`;
    }
  }
  catch (err) {
    console.error(`Erro ao buscar dados do ambiente: ${err}`);
  }
}

const fanToggle = document.querySelector('#fanToggle');

async function turnOnFan(action) {
  try {
    const response = await fetch(`/api/fan?action=${action}`);
    const data = await response.json();

    if(!data.ok) {
      console.error("Erro ao tentar desligar fan!");
      fanToggle.checked = false;
    }
    
    // Sucesso! Vamos atualizar os textos da interface
    if (action) {
      // Estado Ligado
      fanStatus.innerText = "ON";
      fanStatus.classList.remove('state-pill--off'); // Remove a cor cinza
      fanVisualLabel.innerText = "ACTIVE";
      fanPowerLabel.innerText = "Fan is on";
    } else {
      // Estado Desligado
      fanStatus.innerText = "OFF";
      fanStatus.classList.add('state-pill--off'); // Volta para a cor cinza
      fanVisualLabel.innerText = "STANDBY";
      fanPowerLabel.innerText = "Fan is off";
    }
  }
  catch (err) {
    console.error(`Erro ao tentar desligar fan: ${err}`);
    fanToggle.checked = !action;
  }
}

const targetTempInput = document.querySelector('#temperatureLimitInput');
const targetTempView = document.querySelector('#temperatureLimitValue');

async function alterTargetTemp(value) {
  try {
    const response = await fetch(`/api/targettemp?value=${value}`);
    const data = await response.json();

    if (!data.ok) {
      targetTempInput.value = `${data.temp}`;
      targetTempView.innerText = `${data.temp}`;
      console.error(data.message);
      return;
    }
  }
  catch (err) {
    console.error(`Erro ao alterar temperatura alvo: ${err}`);
  }
}

const autoModeToggle = document.querySelector('#autoModeToggle');

async function autoMode(action) {
  try {
    const response = await fetch(`/api/mode?action=${action}`);
    const data = await response.json();

    if(!data.ok) {
      console.error(data.message);
      autoModeToggle.checked = false;
    }
  }
  catch (err) {
    console.error(`Erro ao trocar de mode: ${err}`);
  }
}

fanToggle.addEventListener('change', async (e) => {
  await turnOnFan(e.target.checked);
});


let timer;

targetTempInput.addEventListener('input', async (e) => {
  const currentValue = e.target.value;
  
  // 1. Atualização visual instantânea enquanto arrasta!
  targetTempView.innerText = currentValue;
  
  // 2. Lógica de debounce para a API
  clearTimeout(timer);
  timer = setTimeout(() => {
    alterTargetTemp(currentValue);
  }, 2000);
});

autoModeToggle.addEventListener('change', async(e) => {
  
  await autoMode(e.target.checked);
});

// Busca a temperatura imediatamente ao carregar a página
updateTemp();

// Repete a busca a cada 5 segundos (5000 ms)
setInterval(updateTemp, 5000);

