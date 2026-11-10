#pragma once
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "Hardware.h"
#include "Routes.h"

AsyncWebServer server(80);
namespace WebServer {
  // -- Página HTML do AP
  const char* htmlSetupWifi = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="color-scheme" content="dark">
    <title>Configurar Wi-Fi - ThermoFlow</title>
    <style>
    :root{--bg:#1F1F1F;--text:#F2F2F2;--muted:#BFBFBF;--cyan:#29BADB;--line:rgba(255,255,255,.14)}
    *{box-sizing:border-box;margin:0}
    body{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px 16px;font:16px/1.4 'Inter',system-ui,-apple-system,'Segoe UI',Roboto,sans-serif;color:var(--text);background:radial-gradient(circle at 0 0,rgba(41,186,219,.35),transparent 45%),radial-gradient(circle at 100% 70%,rgba(0,108,133,.45),transparent 50%),var(--bg);-webkit-tap-highlight-color:transparent}
    .card{width:100%;max-width:400px;padding:28px 24px;border-radius:24px;background:rgba(255,255,255,.07);border:1px solid var(--line);-webkit-backdrop-filter:blur(12px);backdrop-filter:blur(12px)}
    .icon{display:grid;place-items:center;width:52px;height:52px;margin-bottom:16px;border-radius:50%;color:var(--cyan);background:rgba(41,186,219,.16);border:1px solid rgba(41,186,219,.45)}
    .icon svg{width:26px;height:26px;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
    .eyebrow{font-size:12px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted)}
    h1{font:500 28px/1.2 'Fraunces',Georgia,serif;margin-bottom:6px}
    .sub{font-size:14px;color:var(--muted);margin-bottom:20px}
    label{display:block;margin:14px 0 6px;font-size:14px;font-weight:600}
    input[type=text],input[type=password]{width:100%;min-height:50px;padding:0 14px;font:inherit;color:var(--text);background:rgba(255,255,255,.08);border:1px solid var(--line);border-radius:14px}
    input::placeholder{color:#8C8C8C}
    input:focus{outline:none;border-color:var(--cyan);box-shadow:0 0 0 3px rgba(41,186,219,.25)}
    .check{display:flex;align-items:center;gap:10px;min-height:44px;margin:4px 0 0;font-weight:400;color:var(--muted);cursor:pointer}
    .check input{width:20px;height:20px;margin:0;accent-color:var(--cyan)}
    button{width:100%;min-height:52px;margin-top:20px;font:inherit;font-weight:600;color:#fff;background:linear-gradient(135deg,#077A94,#00627A);border:0;border-radius:999px;cursor:pointer}
    button:active{opacity:.85}
    button:disabled{opacity:.6;cursor:wait}
    .note{margin-top:16px;font-size:13px;text-align:center;color:var(--muted)}
    </style>
    </head>
    <body>
    <main class="card">
      <div class="icon">
        <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M2.5 9a15 15 0 0 1 19 0M5.5 12.5a10.5 10.5 0 0 1 13 0M8.6 16a6 6 0 0 1 6.8 0"/><circle cx="12" cy="19.2" r="1.2" fill="currentColor" stroke="none"/></svg>
      </div>
      <p class="eyebrow">ThermoFlow</p>
      <h1>Conectar ao Wi-Fi</h1>
      <p class="sub">Informe a rede que o aparelho deve usar.</p>

      <form action="/api/conn" method="POST" onsubmit="var b=this.querySelector('button');b.disabled=true;b.textContent='Salvando…'">
        <label for="ssid">Nome da rede (SSID)</label>
        <input type="text" id="ssid" name="ssid" maxlength="32" required autocomplete="off" autocapitalize="none" autocorrect="off" spellcheck="false" placeholder="Ex.: Casa_5G">

        <label for="pass">Senha</label>
        <input type="password" id="pass" name="pass" maxlength="63" autocomplete="off" placeholder="Deixe em branco se a rede for aberta">

        <label class="check"><input type="checkbox" onchange="document.getElementById('pass').type=this.checked?'text':'password'">Mostrar senha</label>

        <button type="submit">Salvar e reiniciar</button>
      </form>
      <p class="note">O ThermoFlow vai reiniciar e tentar entrar nessa rede.</p>
    </main>
    </body>
</html>
  )rawliteral";

  // -- Inicia o servidor Web
  inline void initWebServer() {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("thermoflow.html");

    // -- Setando rotas
    Routes::setupRoutes(server);

    // -- Serve Página de Configuração
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", htmlSetupWifi);
    });
    
    server.begin();
  }

}
