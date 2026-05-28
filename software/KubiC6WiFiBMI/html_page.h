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
.weather-btn{background:linear-gradient(135deg,#0277bd,#4fc3f7)}
.pomo-btn{background:linear-gradient(135deg,#b71c1c,#ef5350)}
.pomo-pause{background:linear-gradient(135deg,#e65100,#ff9800)}
.pomo-reset{background:linear-gradient(135deg,#37474f,#78909c)}
.inputBox{width:100%;padding:16px;border:none;border-radius:16px;margin-bottom:14px;background:#1f2937;color:white;font-size:16px;outline:none}
.inputBox::placeholder{color:#9ca3af}
.inputRow{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
.switchContainer{display:flex;align-items:center;justify-content:space-between;background:#1f2937;padding:18px;border-radius:18px}
.switchContainer span{font-size:18px;font-weight:bold}
.switch{position:relative;display:inline-block;width:64px;height:34px}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#ef5350;transition:.3s;border-radius:34px}
.slider:before{position:absolute;content:"";height:26px;width:26px;left:4px;bottom:4px;background:white;transition:.3s;border-radius:50%}
input:checked+.slider{background:#00c853}
input:checked+.slider:before{transform:translateX(30px)}
.pomo-display{background:#1f2937;border-radius:16px;padding:16px;margin-bottom:14px;text-align:center}
.pomo-time{font-size:48px;font-weight:900;font-family:monospace;color:#ef5350;letter-spacing:4px}
.pomo-bar-bg{background:#374151;border-radius:8px;height:10px;margin:10px 0}
.pomo-bar{background:linear-gradient(90deg,#ef5350,#ff9800);height:10px;border-radius:8px;transition:width 1s linear}
.pomo-status{font-size:13px;color:#9ca3af;margin-top:6px}
.pomo-controls{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
.range-row{display:flex;align-items:center;gap:12px;margin-bottom:14px}
.range-row label{font-size:14px;color:#9ca3af;white-space:nowrap}
.range-row input[type=range]{flex:1;accent-color:#ef5350}
.range-val{font-size:16px;font-weight:bold;min-width:40px;text-align:right}
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
  <h2>Pomodoro</h2>

  <div class="range-row">
    <label>Minutes:</label>
    <input type="range" min="1" max="60" value="25" step="1" id="pomo-min"
           oninput="document.getElementById('pomo-min-val').textContent=this.value">
    <span class="range-val" id="pomo-min-val">25</span>
  </div>

  <div class="pomo-display">
    <div class="pomo-time" id="pomo-time">25:00</div>
    <div class="pomo-bar-bg">
      <div class="pomo-bar" id="pomo-bar" style="width:0%"></div>
    </div>
    <div class="pomo-status" id="pomo-status">Ready</div>
  </div>

  <div class="pomo-controls">
    <button class="pomo-btn"   onclick="pomoAction('start')">Start</button>
    <button class="pomo-pause" onclick="pomoAction('pause')">Pause</button>
    <button class="pomo-reset" onclick="pomoAction('reset')">Reset</button>
  </div>
</div>

<div class="footer">Powered by UNIT Electronics</div>

<script>

// ---- State ----
let pomoTotal   = 25 * 60;
let pomoRemain  = 25 * 60;
let pomoRunning = false;
let pomoTick    = null;

// ---- Auto sync time on load ----
fetch('/setTime?epoch=' + Math.floor(Date.now() / 1000));

// ---- Emotion ----
function emotion(name) { fetch('/emotion?name=' + name); }

// ---- Clock ----
function showClock() { fetch('/clock'); }

// ---- Auto Mode ----
function toggleAuto() {
  fetch('/auto?state=' + (document.getElementById('autoSwitch').checked ? 'on' : 'off'));
}

// ---- Message ----
function sendMessage() {
  const t = document.getElementById('msg').value;
  if (t) fetch('/message?text=' + encodeURIComponent(t));
}

// ---- Pomodoro UI ----
function updatePomoDisplay() {
  const mm = String(Math.floor(pomoRemain / 60)).padStart(2, '0');
  const ss = String(pomoRemain % 60).padStart(2, '0');
  document.getElementById('pomo-time').textContent = mm + ':' + ss;
  const pct = Math.round((1 - pomoRemain / pomoTotal) * 100);
  document.getElementById('pomo-bar').style.width = pct + '%';
  document.getElementById('pomo-status').textContent =
    pomoRemain === 0 ? 'Time up! Well done!' :
    pomoRunning ? 'Focus mode active' : (pomoRemain < pomoTotal ? 'Paused' : 'Ready');
}

function startTick() {
  if (pomoTick) clearInterval(pomoTick);
  pomoTick = setInterval(() => {
    if (pomoRemain > 0) {
      pomoRemain--;
      updatePomoDisplay();
    } else {
      clearInterval(pomoTick);
      pomoRunning = false;
      updatePomoDisplay();
    }
  }, 1000);
}

function pomoAction(action) {
  const minutes = parseInt(document.getElementById('pomo-min').value);
  if (action === 'start') {
    if (!pomoRunning && pomoRemain > 0) {
      if (pomoRemain === pomoTotal) {
        fetch('/pomodoro?action=start&minutes=' + minutes);
      } else {
        fetch('/pomodoro?action=resume');
      }
      pomoRunning = true;
      startTick();
    }
  } else if (action === 'pause') {
    if (pomoRunning) {
      clearInterval(pomoTick);
      pomoRunning = false;
      fetch('/pomodoro?action=pause');
      updatePomoDisplay();
    }
  } else if (action === 'reset') {
    clearInterval(pomoTick);
    pomoRunning = false;
    pomoTotal   = minutes * 60;
    pomoRemain  = pomoTotal;
    fetch('/pomodoro?action=reset&minutes=' + minutes);
    updatePomoDisplay();
  }
}

updatePomoDisplay();

</script>
</body>
</html>
)rawliteral";
