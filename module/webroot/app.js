/* CoreFlow Material 3 UI adapter.
 * Uses only the project's fixed KernelSU command builders and status contracts.
 * Unknown telemetry stays unknown; UI never writes /proc or /sys.
 */
(() => {
"use strict";
const $ = s => document.querySelector(s), $$ = s => [...document.querySelectorAll(s)];
const C = window.CoreFlowControl, S = window.CoreFlowStatus;
const state = { parsed:null, probe:null, configRaw:"", config:null, busy:false, timer:null, pending:null, trace:[], logText:"", selectedMode:"observe" };
const val = (v,d="Unknown") => v===undefined || v===null || v==="" ? d : String(v);
const pick = (...xs) => xs.find(x=>x!==undefined && x!==null && x!=="");
function toast(s){const e=$("#toast");e.textContent=s;e.classList.add("show");clearTimeout(state.timer);state.timer=setTimeout(()=>e.classList.remove("show"),2800)}
function execShell(command, timeout=15000){return new Promise(resolve=>{let done=false;const finish=(errno,stdout,stderr)=>{if(done)return;done=true;clearTimeout(t);resolve({errno:Number(errno)||0,stdout:String(stdout||""),stderr:String(stderr||"")})};const t=setTimeout(()=>finish(1,"","WebUI execution timed out"),timeout);try{if(typeof ksu==="undefined"||typeof ksu.exec!=="function"){finish(1,"","No KernelSU WebUI exec bridge.");return}const cb="cf_m3_"+Date.now()+"_"+Math.floor(Math.random()*1e6);window[cb]=(e,o,s)=>{try{delete window[cb]}catch(_){}finish(e,o,s)};try{ksu.exec(command,cb)}catch(e){try{ksu.exec(command,"{}",cb)}catch(e2){finish(1,"",String(e2||e))}}}catch(e){finish(1,"",String(e))}})}
function page(n){if(!$("#page-"+n))return;$$(".page").forEach(e=>e.classList.toggle("active",e.id==="page-"+n));$$("[data-page]").forEach(e=>e.classList.toggle("active",e.dataset.page===n));$("#current-section").textContent=({overview:"Overview",resources:"Resource explorer",decisions:"Decision center",policy:"Policy & modes",journal:"Journal",system:"System profile",about:"About"})[n]||n;location.hash=n;if(n==="journal")loadJournal();if(n==="policy")loadConfig()}
function metric(k,v,caption,pct){$("#metric-"+k).textContent=v==null?"—":Math.round(v*10)/10;$("#"+k+"-caption").textContent=caption;$("#"+k+"-track").style.width=Math.max(0,Math.min(100,pct||0))+"%"}
function node(tag, className, text) {
  const e = document.createElement(tag);
  if (className) e.className = className;
  if (text !== undefined && text !== null) e.textContent = String(text);
  return e;
}
function clear(e) { while (e.firstChild) e.removeChild(e.firstChild); }
function empty(target, text, className="empty") { clear(target); target.appendChild(node("div", className, text)); }
function detailRows(target, rows) {
  clear(target);
  rows.forEach(([k,v]) => {
    const row=node("div","detail-row"), label=node("span","",k), value=node("b","",val(v));
    row.append(label,value); target.appendChild(row);
  });
}
function timelineRows(target, rows, compact=false) {
  clear(target);
  if (!rows.length) { empty(target, compact ? "Waiting for runtime data." : "No decision pipeline available."); return; }
  rows.forEach(x => {
    const item=node("div","timeline-item"); item.appendChild(node("span","timeline-node"));
    const body=node("div"); body.appendChild(node("b","",val(x.id)));
    body.appendChild(node("p","",val(x.note,x.state))); item.appendChild(body);
    if (!compact) item.appendChild(node("time","",val(x.state)));
    target.appendChild(item);
  });
}
function renderResources(p){
  const d=p&&p.data;const rows=d?S.deviceRows(d):[];let discovered=null,ready=null;
  for(const r of rows){if(/capabilities/i.test(r[0])){const m=/(\d+)\s+discovered/i.exec(r[1]);if(m)discovered=Number(m[1]);const n=/(\d+)\s+policy-ready/i.exec(r[1]);if(n)ready=Number(n[1]);}}
  $("#nav-resource-count").textContent=discovered==null?"—":discovered;$("#discovered-total").textContent=discovered==null?"—":discovered;$("#readable-total").textContent="—";$("#writable-total").textContent="—";$("#authorized-total").textContent=ready==null?"—":ready;
  $("#bar-readable").style.width="0%";$("#bar-writable").style.width="0%";$("#bar-authorized").style.width=discovered&&ready!=null?(ready/discovered*100)+"%":"0%";
  ["resource-count-all","resource-count-readable","resource-count-writable","resource-count-authorized"].forEach((id,i)=>$("#"+id).textContent=[discovered??"—","—","—",ready??"—"][i]);
  $("#resource-table-caption").textContent="This daemon status contract reports aggregate capabilities, not a per-resource inventory.";
  const body=$("#resource-table-body");clear(body);const tr=node("tr"),td=node("td","empty","Per-resource records are not exposed by the current status schema. Aggregate counts are shown above; no resources are fabricated.");td.setAttribute("colspan","7");tr.appendChild(td);body.appendChild(tr);
  $("#resource-visible-count").textContent="No per-resource inventory";
}
function renderDecision(p){
  const d=p&&p.data;const sum=S.summary(p);const pipe=d?S.pipeline(d):[];
  detailRows($("#decision-snapshot"),[["Summary",sum.title],["Code",sum.code],["Tone",sum.tone],["Status age",p&&p.age!=null?p.age+" s":null],["Freshness",p?(p.stale?"Stale":"Fresh"):null]]);
  const consult=d&&d.brain&&d.brain.consultation&&d.brain.consultation.reasons;const cs=consult&&typeof consult==="object"?Object.entries(consult):[];
  const ledger=$("#consultation-ledger");if(cs.length)detailRows(ledger,cs.slice(0,10));else empty(ledger,"No consultation ledger reported.");
  $("#decision-trace-count").textContent=pipe.length+" pipeline stages";
  timelineRows($("#decision-timeline"),pipe,false);timelineRows($("#overview-timeline"),pipe.slice(0,3),true);
}
function renderSystem(p){
  const d=p&&p.data;const rows=d?S.deviceRows(d):[];
  const find=(re)=>{const r=rows.find(x=>re.test(x[0]));return r?r[1]:null;};
  $("#system-device").textContent=val(find(/manufacturer \/ model/i));$("#system-android").textContent=val(find(/^android$/i));$("#system-kernel").textContent=val(find(/kernel/i));$("#system-soc").textContent=val(find(/^soc \/ hardware$/i));
  detailRows($("#system-details"),rows);
  const cap=$("#system-capabilities");clear(cap);
  [["Capability discovery",val(find(/capabilities/i),"Not reported")],["Block I/O policy",val(find(/block i\/o/i),"Not reported")]].forEach(([k,v])=>{const tile=node("div","capability-tile");tile.append(node("b","",k),node("span","",v));cap.appendChild(tile);});
}
function render(p){
state.parsed=p;const sum=S.summary(p),d=p&&p.data;const running=!!(state.probe&&state.probe.pids&&state.probe.pids.length);
const ds=state.probe?(running?"RUNNING":"STOPPED"):(p&&p.ok?"STATUS RECEIVED":"UNAVAILABLE");
$("#sidebar-daemon-state").textContent=ds;$("#sidebar-last-seen").textContent="Updated "+new Date().toLocaleTimeString([],{hour:"2-digit",minute:"2-digit"});
$("#connection-label").textContent=p&&p.ok?(p.stale?"Stale status":"Status received"):"Status unavailable";$("#connection-chip").classList.toggle("connected",!!(p&&p.ok&&!p.stale));
$("#engine-state-pill").textContent=sum.code||ds;$("#engine-state-title").textContent=sum.title||"Status unavailable";$("#engine-state-description").textContent=sum.detail||p?.error||"Runtime status has not been reported.";
const brain=d&&d.brain||{};const mode=val(brain.mode,"unknown");$("#engine-mode-label").textContent="Mode: "+mode;$("#policy-mode-pill").textContent=mode.toUpperCase();$("#engine-version-label").textContent="Version: "+val(d&&d.version,"—");
const armed=typeof brain.armed==="boolean"?brain.armed:null;$("#mutation-state").textContent=armed===false?"DISARMED":armed===true&&mode==="adaptive"?"ARMED":"UNKNOWN";$("#mutation-summary").textContent=armed===false?"Mutation is disarmed":armed===true&&mode==="adaptive"?"Adaptive configured; runtime gates still apply":"Authority not confirmed";$("#mutation-detail").textContent="Discovery is not authorization. Every mutation remains subject to daemon-side gates.";$("#safety-gates").textContent="SAFE_MODE: "+val(state.probe&&state.probe.safe)+" · DISABLE: "+val(state.probe&&state.probe.disable);
const eyes=d&&d.eyes||{};const temp=Number.isFinite(eyes.thermal)?eyes.thermal:null;const cpu=Number.isFinite(eyes.cpu)?eyes.cpu*100:null;const mem=Number.isFinite(eyes.mem)?eyes.mem*100:null;const bat=Number.isFinite(eyes.battery)?eyes.battery:null;
metric("thermal",temp,temp==null?"No valid thermal sample":temp>=43?"Elevated temperature":"Reported by daemon",temp);metric("cpu",cpu,cpu==null?"No CPU sample":"Reported CPU activity",cpu);metric("memory",mem,mem==null?"No memory sample":"Available memory ratio",mem);metric("battery",bat,bat==null?"No battery sample":"Battery level reported",bat);
renderResources(p);renderDecision(p);renderSystem(p);
const pairs=[["Mutation mode",brain.mode],["Mutation armed",brain.armed],["CPU governor permission",brain.allowCpu],["Status age",p&&p.age!=null?p.age+" s":null],["Status freshness",p?(p.stale?"Stale":"Fresh"):null],["Safety hold",brain.hold]];
detailRows($("#policy-details"),pairs);const selected=mode==="adaptive"?"adaptive":"observe";$$(".mode-option").forEach(x=>x.classList.toggle("selected",x.dataset.mode===selected));state.selectedMode=selected;
}
async function refresh(){
try{const r=await execShell(C.snapshotCmd(),15000);const parts=C.splitSnapshot(r.stdout);state.probe=parts.probe?C.parseProbe(parts.probe):null;state.parsed=parts.status?S.parse(parts.status):{ok:false,error:r.stderr||"No status payload"};if(r.errno!==0&&state.parsed.ok===false)state.parsed.error=r.stderr||state.parsed.error;render(state.parsed);if(!r.errno)toast("Runtime status updated.");else toast("Status read failed: "+(r.stderr||"bridge error"));}catch(e){state.parsed={ok:false,error:String(e)};render(state.parsed);toast("Unable to read runtime status.")}}
async function rediscover(){const r=await execShell(C.refreshCmd());if(r.errno===0){toast("Re-discovery signal sent.");setTimeout(refresh,700)}else toast("Re-discovery failed: "+(r.stderr||r.stdout||"unknown error"))}
async function loadConfig(){const r=await execShell(C.readConfigCmd(),8000);if(r.errno!==0||!r.stdout.trim()){toast("config.ini unavailable; policy was not changed.");return false}state.configRaw=r.stdout;state.config=C.parseIni(r.stdout);return true}
async function applyMode(){
if(state.busy)return;state.busy=true;
try{
if(!await loadConfig())return;
const form=C.formFromConfig(state.config);form.mutation_mode=state.selectedMode;
const v=C.validate(form);if(!v.ok){toast("Policy validation failed: "+Object.values(v.errors).join("; "));return}
const probe=state.probe||{};const g=C.gate(v.values,{currentMode:state.config.mutation_mode==="adaptive"?"adaptive":"observe",safe:!!probe.safe,disable:!!probe.disable});
if(!g.ok){toast(g.reason||"Policy gate rejected this change.");return}
if(g.confirm){const message=g.confirm==="resume"?"Adaptive may resume after the disable marker is cleared. Continue?":g.confirm==="arm"?"Adaptive mode requires explicit arming. Continue?":"Switch to Observe mode?";if(!window.confirm(message))return}
const raw=C.applyValues(state.configRaw,v.values);const w=await execShell(C.writeConfigCmd(raw,v.values,{clearDisable:g.clearDisable}),15000);
if(w.errno===0&&w.stdout.includes("CONFIG_COMMIT_OK")){state.configRaw=raw;state.config=C.parseIni(raw);$("#mode-apply-status").textContent="Saved. Restart the engine to apply it.";toast("Policy saved. Restart the engine to apply it.");await refresh()}else toast("Policy save rejected: "+(w.stderr||w.stdout||"unknown error"));
} catch(e){toast("Policy change failed: "+String(e))} finally{state.busy=false}
}
async function loadJournal(){
try{const r=await Promise.all([execShell(C.readTraceCmd(),10000),execShell(C.readLogCmd(),10000)]);state.trace=r[0].errno===0?C.parseTrace(r[0].stdout,400):[];state.logText=r[1].errno===0?r[1].stdout:"";const q=$("#journal-search").value.toLowerCase();const list=state.trace.filter(e=>!q||JSON.stringify(e).toLowerCase().includes(q));const target=$("#journal-list");clear(target);if(list.length){list.slice(0,100).forEach(e=>{const item=node("div","journal-entry");item.appendChild(node("time","",val(pick(e.timestamp,e.time),"—")));const body=node("div");body.append(node("b","",val(pick(e.event,e.type,e.result),"Runtime event")),node("p","",val(pick(e.reason,e.message,e.decision_reason),"No detail reported")));item.appendChild(body);item.appendChild(node("span","mini-pill",val(pick(e.result,e.level),"event")));target.appendChild(item);});}else{const fallback=!list.length&&state.logText.trim()?state.logText.split("\n").filter(x=>x.toLowerCase().includes(q)).slice(-100).reverse().join("\n"):"No matching journal entries.";target.appendChild(node("div","empty",fallback));}}catch(e){$("#journal-list").textContent="Journal unavailable: "+String(e)}}
async function engineAction(kind){
const cmd=kind==="start"?C.startCmd():kind==="stop"?C.stopCmd():C.restartCmd();
const question=kind==="start"?"Request CoreFlow engine start?":kind==="stop"?"Stop coreflowd?":"Restart CoreFlow engine?";
if(!confirm(question))return;
const r=await execShell(cmd,15000);
if(r.errno===0){toast(kind.charAt(0).toUpperCase()+kind.slice(1)+" request sent.");setTimeout(refresh,800)}
else toast(kind+" failed: "+(r.stderr||r.stdout||"unknown error"));
}
function wire(){
$$("[data-page]").forEach(b=>b.addEventListener("click",()=>page(b.dataset.page)));$$("[data-goto]").forEach(b=>b.addEventListener("click",()=>page(b.dataset.goto)));
$("#theme-toggle").addEventListener("click",()=>document.documentElement.dataset.theme=document.documentElement.dataset.theme==="dark"?"light":"dark");
$("#btn-start-engine").addEventListener("click",()=>engineAction("start"));$("#btn-stop-engine").addEventListener("click",()=>engineAction("stop"));$("#btn-restart-engine").addEventListener("click",()=>engineAction("restart"));
["btn-refresh","btn-resources-refresh","btn-decision-refresh","btn-journal-refresh","btn-system-refresh"].forEach(id=>$("#"+id).addEventListener("click",id==="btn-journal-refresh"?loadJournal:refresh));
$("#btn-rediscover").addEventListener("click",()=>{if(confirm("Request capability re-discovery? This sends SIGUSR1 and does not write tunables."))rediscover()});
$("#btn-resources-discover").addEventListener("click",()=>{if(confirm("Request capability re-discovery? This does not write tunables."))rediscover()});
["resource-search","resource-domain","resource-state"].forEach(id=>$("#"+id).addEventListener(id==="resource-search"?"input":"change",()=>renderResources(state.parsed)));
$("#journal-search").addEventListener("input",loadJournal);
$$(".mode-option").forEach(b=>b.addEventListener("click",()=>{state.selectedMode=b.dataset.mode;$$(".mode-option").forEach(x=>x.classList.toggle("selected",x===b))}));
$("#btn-apply-mode").addEventListener("click",()=>{if(confirm("Apply "+state.selectedMode+" mode through validated config policy?"))applyMode()});
$("#dialog-cancel").addEventListener("click",()=>$("#confirm-dialog").close());$("#dialog-confirm").addEventListener("click",()=>{const d=$("#confirm-dialog");d.close();if(state.pending)state.pending();state.pending=null});
document.addEventListener("coreflow:material-ready",()=>$("#footer-material-status").textContent="Material 3 modules loaded; native controls active");document.addEventListener("coreflow:material-fallback",()=>$("#footer-material-status").textContent="Local fallback active");
let h=location.hash.slice(1);if(h&&$("#page-"+h))page(h);refresh();setInterval(()=>{if(document.visibilityState!=="hidden")refresh()},5000);
}
window.CoreFlowUI={...(window.CoreFlowUI||{}),refresh,rediscover,showPage:page,getState:()=>state};
document.addEventListener("DOMContentLoaded",wire);
})();
