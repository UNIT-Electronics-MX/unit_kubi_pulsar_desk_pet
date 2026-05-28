#pragma once

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>KubiDeskPet</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:Arial,Helvetica,sans-serif;background:linear-gradient(180deg,#0b1020 0%,#111827 100%);color:white;min-height:100vh;padding:20px}
.header{text-align:center;margin-bottom:30px}
.logo{font-size:42px;font-weight:900;background:linear-gradient(90deg,#00d4ff,#6c63ff);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:10px}
.subtitle{color:#9ca3af;font-size:15px}
.card{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.08);border-radius:24px;padding:20px;margin-bottom:20px;backdrop-filter:blur(10px);box-shadow:0 8px 30px rgba(0,0,0,0.3)}
.card h2{margin-bottom:16px;font-size:20px;color:#e5e7eb}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:14px}
button{border:none;border-radius:18px;padding:16px;font-size:16px;font-weight:bold;cursor:pointer;color:white;transition:0.2s;box-shadow:0 4px 15px rgba(0,0,0,0.3)}
button:hover{transform:translateY(-2px);opacity:0.92}
button:active{transform:scale(0.98)}
.happy{background:linear-gradient(135deg,#00c853,#64dd17)}
.sad{background:linear-gradient(135deg,#546e7a,#78909c)}
.angry{background:linear-gradient(135deg,#ff1744,#ff5252)}
.sleepy{background:linear-gradient(135deg,#5e35b1,#7e57c2)}
.vomit{background:linear-gradient(135deg,#43a047,#c0ca33)}
.surprised{background:linear-gradient(135deg,#ff9100,#ffca28)}
.clock{background:linear-gradient(135deg,#1565c0,#42a5f5)}
.pomo-start{background:linear-gradient(135deg,#b71c1c,#ef5350)}
.pomo-pause{background:linear-gradient(135deg,#e65100,#ff9800)}
.pomo-reset{background:linear-gradient(135deg,#37474f,#78909c)}
.inputBox{width:100%;padding:16px;border:none;border-radius:16px;margin-bottom:14px;background:#1f2937;color:white;font-size:16px;outline:none}
.inputBox::placeholder{color:#9ca3af}
.switchContainer{display:flex;align-items:center;justify-content:space-between;background:#1f2937;padding:18px;border-radius:18px}
.switchContainer span{font-size:18px;font-weight:bold}
.switch{position:relative;display:inline-block;width:64px;height:34px}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ef5350;transition:.3s;border-radius:34px}
.slider:before{position:absolute;content:"";height:26px;width:26px;left:4px;bottom:4px;background:white;transition:.3s;border-radius:50%}
input:checked+.slider{background:#00c853}
input:checked+.slider:before{transform:translateX(30px)}
/* Pomodoro */
.pomo-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
.pomo-header h2{margin:0}
.pomo-cycle{font-size:13px;color:#9ca3af;background:#1f2937;padding:4px 12px;border-radius:20px}
.pomo-phase-badge{text-align:center;font-size:13px;font-weight:bold;padding:6px 0;border-radius:12px;margin-bottom:14px;letter-spacing:1px}
.phase-focus{background:rgba(239,83,80,0.2);color:#ef5350;border:1px solid rgba(239,83,80,0.3)}
.phase-break{background:rgba(0,200,83,0.2);color:#00c853;border:1px solid rgba(0,200,83,0.3)}
.phase-idle{background:rgba(255,255,255,0.05);color:#9ca3af;border:1px solid rgba(255,255,255,0.1)}
.pomo-display{background:#1f2937;border-radius:16px;padding:20px;margin-bottom:14px;text-align:center}
.pomo-time{font-size:56px;font-weight:900;font-family:monospace;letter-spacing:4px;transition:color .3s}
.pomo-bar-bg{background:#374151;border-radius:8px;height:8px;margin:14px 0 0}
.pomo-bar{height:8px;border-radius:8px;width:0%;transition:width .9s linear}
.bar-focus{background:linear-gradient(90deg,#ef5350,#ff9800)}
.bar-break{background:linear-gradient(90deg,#00c853,#00bcd4)}
.range-row{display:flex;align-items:center;gap:12px;margin-bottom:12px}
.range-row label{font-size:14px;color:#9ca3af;min-width:80px}
.range-row input[type=range]{flex:1;accent-color:#ef5350}
.range-val{font-size:15px;font-weight:bold;min-width:36px;text-align:right}
.pomo-controls{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
.footer{text-align:center;margin-top:30px;color:#6b7280;font-size:13px}
</style>
</head>
<body>

<div class="header">
  <div class="logo">KubiDeskPet</div>
  <div class="subtitle">UNIT Electronics · Smart Desktop Robot</div>
</div>

<!-- EMOTIONS -->
<div class="card">
  <h2>Emotions</h2>
  <div class="grid">
    <button class="happy"     onclick="emotion('happy')">Happy</button>
    <button class="sad"       onclick="emotion('sad')">Sad</button>
    <button class="angry"     onclick="emotion('angry')">Angry</button>
    <button class="sleepy"    onclick="emotion('sleepy')">Sleepy</button>
    <button class="vomit"     onclick="emotion('vomit')">Vomit</button>
    <button class="surprised" onclick="emotion('surprised')">Surprise</button>
  </div>
</div>

<!-- CONTROLS -->
<div class="card">
  <h2>Controls</h2>
  <div class="grid" style="margin-bottom:16px">
    <button class="clock" onclick="showClock()">Clock</button>
  </div>
  <div class="switchContainer">
    <span>Auto Mode</span>
    <label class="switch">
      <input type="checkbox" id="autoSwitch" checked onchange="toggleAuto()">
      <span class="slider"></span>
    </label>
  </div>
</div>

<!-- MESSAGE -->
<div class="card">
  <h2>Message</h2>
  <input class="inputBox" type="text" id="msg" placeholder="Write message...">
  <button class="clock" style="width:100%" onclick="sendMessage()">Send Message</button>
</div>

<!-- POMODORO -->
<div class="card">
  <div class="pomo-header">
    <h2>Pomodoro</h2>
    <span class="pomo-cycle" id="pomo-cycle">Cycle #1</span>
  </div>

  <div class="pomo-phase-badge phase-idle" id="pomo-phase">READY</div>

  <div class="pomo-display">
    <div class="pomo-time" id="pomo-time" style="color:#9ca3af">25:00</div>
    <div class="pomo-bar-bg">
      <div class="pomo-bar bar-focus" id="pomo-bar"></div>
    </div>
  </div>

  <div class="range-row">
    <label>Focus:</label>
    <input type="range" min="1" max="60" value="25" id="r-focus"
      oninput="document.getElementById('v-focus').textContent=this.value+'m';syncIfIdle()">
    <span class="range-val" id="v-focus">25m</span>
  </div>
  <div class="range-row">
    <label>Break:</label>
    <input type="range" min="1" max="15" value="5" id="r-break"
      oninput="document.getElementById('v-break').textContent=this.value+'m';syncIfIdle()">
    <span class="range-val" id="v-break">5m</span>
  </div>

  <div class="pomo-controls">
    <button class="pomo-start" onclick="pomoAction('start')">Start</button>
    <button class="pomo-pause" onclick="pomoAction('pause')">Pause</button>
    <button class="pomo-reset" onclick="pomoAction('reset')">Reset</button>
  </div>
</div>

<div class="footer">Powered by UNIT Electronics</div>

<script>
// ---- Auto sync time ----
fetch('/setTime?epoch=' + Math.floor(Date.now() / 1000));

// ---- Emotions / Clock / Message ----
function emotion(name) { fetch('/emotion?name=' + name); }
function showClock()   { fetch('/clock'); }
function toggleAuto()  { fetch('/auto?state=' + (document.getElementById('autoSwitch').checked ? 'on' : 'off')); }
function sendMessage() {
  const t = document.getElementById('msg').value;
  if (t) fetch('/message?text=' + encodeURIComponent(t));
}

// ---- Pomodoro state (mirror del ESP32) ----
let pomoState = { phase: 0, running: false, remaining: 25*60, cycle: 1 };
let pomoTick  = null;

function getFocus() { return parseInt(document.getElementById('r-focus').value); }
function getBreak() { return parseInt(document.getElementById('r-break').value); }

function syncIfIdle() {
  // Solo sincroniza duraciones si el pomodoro está en idle
  if (pomoState.phase === 0)
    fetch('/pomodoro?action=show&focus=' + getFocus() + '&brk=' + getBreak());
}

async function pomoAction(action) {
  const url = '/pomodoro?action=' + action +
              '&focus=' + getFocus() + '&brk=' + getBreak();
  const r   = await fetch(url);
  const data = await r.json();
  applyState(data);

  if (action === 'start' || action === 'resume') {
    startLocalTick();
  } else if (action === 'pause' || action === 'reset') {
    clearInterval(pomoTick); pomoTick = null;
  }
}

function applyState(data) {
  pomoState = data;
  const mm = String(Math.floor(data.remaining / 60)).padStart(2,'0');
  const ss = String(data.remaining % 60).padStart(2,'0');

  const timeEl  = document.getElementById('pomo-time');
  const barEl   = document.getElementById('pomo-bar');
  const phaseEl = document.getElementById('pomo-phase');
  const cycleEl = document.getElementById('pomo-cycle');

  timeEl.textContent = mm + ':' + ss;
  cycleEl.textContent = 'Cycle #' + data.cycle;

  const focusSec = getFocus() * 60;
  const breakSec = getBreak() * 60;
  const total    = (data.phase === 2) ? breakSec : focusSec;
  const pct      = total > 0 ? Math.round((1 - data.remaining / total) * 100) : 0;
  barEl.style.width = pct + '%';

  if (data.phase === 0) {
    timeEl.style.color = '#9ca3af';
    barEl.className    = 'pomo-bar bar-focus';
    phaseEl.className  = 'pomo-phase-badge phase-idle';
    phaseEl.textContent = data.remaining === 0 ? 'DONE ✓' : 'READY';
  } else if (data.phase === 1) {
    timeEl.style.color = '#ef5350';
    barEl.className    = 'pomo-bar bar-focus';
    phaseEl.className  = 'pomo-phase-badge phase-focus';
    phaseEl.textContent = data.running ? '🔴 FOCUS' : '⏸ FOCUS PAUSED';
  } else {
    timeEl.style.color = '#00c853';
    barEl.className    = 'pomo-bar bar-break';
    phaseEl.className  = 'pomo-phase-badge phase-break';
    phaseEl.textContent = data.running ? '🟢 BREAK' : '⏸ BREAK PAUSED';
  }
}

function startLocalTick() {
  if (pomoTick) clearInterval(pomoTick);
  pomoTick = setInterval(() => {
    if (!pomoState.running) return;
    if (pomoState.remaining > 0) {
      pomoState.remaining--;
      applyState(pomoState);
    } else {
      // Fase terminó — pedir estado real al ESP32
      clearInterval(pomoTick); pomoTick = null;
      fetch('/pomodoro?action=show').then(r => r.json()).then(d => {
        applyState(d);
        if (d.running) startLocalTick();
      });
    }
  }, 1000);
}
</script>
</body>
</html>
)rawliteral";