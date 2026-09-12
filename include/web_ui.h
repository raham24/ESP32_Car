#pragma once

// Minimal control page served at "/". Lets you drive from a phone browser
// to verify the WiFi + API pipeline before a native app exists.
// It talks to the same endpoints a native app would (/drive, /stop, /status).
static const char WEB_UI[] PROGMEM = R"HTML(<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ESP32 Car</title>
<style>
  body{margin:0;font-family:system-ui,sans-serif;background:#111;color:#eee;
       display:flex;flex-direction:column;align-items:center;height:100vh;
       touch-action:none;user-select:none;-webkit-user-select:none}
  h1{font-size:18px;margin:16px 0 4px}
  #status{font-size:13px;color:#888;margin-bottom:16px}
  #pad{width:260px;height:260px;border-radius:50%;background:#222;
       border:2px solid #444;position:relative}
  #knob{width:90px;height:90px;border-radius:50%;background:#3b82f6;
        position:absolute;left:85px;top:85px;box-shadow:0 4px 12px #0008}
  #vals{margin-top:16px;font-family:monospace;font-size:15px}
  #stop{margin-top:20px;padding:14px 40px;font-size:16px;border:0;border-radius:8px;
        background:#dc2626;color:#fff}
</style></head><body>
<h1>ESP32 Car</h1>
<div id="status">connecting...</div>
<div id="pad"><div id="knob"></div></div>
<div id="vals">throttle 0 &nbsp; steering 0</div>
<button id="stop">STOP</button>
<script>
const pad=document.getElementById('pad'),knob=document.getElementById('knob');
const vals=document.getElementById('vals'),status=document.getElementById('status');
const R=130,K=45,MAX=R-K;
let throttle=0,steering=0,active=false,timer=null;

function setKnob(dx,dy){knob.style.left=(R-K+dx)+'px';knob.style.top=(R-K+dy)+'px';}

function onMove(e){
  const r=pad.getBoundingClientRect();
  let dx=e.clientX-r.left-R, dy=e.clientY-r.top-R;
  const d=Math.hypot(dx,dy);
  if(d>MAX){dx*=MAX/d;dy*=MAX/d;}
  setKnob(dx,dy);
  steering=Math.round(dx/MAX*100);
  throttle=Math.round(-dy/MAX*100);
  vals.textContent=`throttle ${throttle}   steering ${steering}`;
}

async function send(path,body){
  try{
    const res=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json'},
                         body:body?JSON.stringify(body):undefined});
    status.textContent=res.ok?'connected':'error '+res.status;
  }catch(err){status.textContent='disconnected';}
}

function start(e){
  active=true;pad.setPointerCapture(e.pointerId);onMove(e);
  // Send at 20 Hz while held (must be well under the 500 ms failsafe)
  timer=setInterval(()=>send('/drive',{throttle,steering}),50);
}
function stop(){
  if(!active)return;active=false;clearInterval(timer);
  throttle=steering=0;setKnob(0,0);
  vals.textContent='throttle 0   steering 0';
  send('/stop');
}

pad.addEventListener('pointerdown',start);
pad.addEventListener('pointermove',e=>{if(active)onMove(e);});
pad.addEventListener('pointerup',stop);
pad.addEventListener('pointercancel',stop);
document.getElementById('stop').addEventListener('click',stop);

// Poll status when idle so the header shows connection health
setInterval(async()=>{
  if(active)return;
  try{const s=await(await fetch('/status')).json();
      status.textContent=`connected · ${s.ip} · ${s.rssi} dBm`;}
  catch(e){status.textContent='disconnected';}
},2000);
</script></body></html>)HTML";
