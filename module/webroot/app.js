/* CoreFlow Autonomous Console: curated diagnostics, atomic config changes, no arbitrary shell. */
(function(){"use strict";
const PATH={base:"/data/adb/coreflow",config:"/data/adb/coreflow/config.ini",logs:"/data/adb/coreflow/logs",pid:"/data/adb/coreflow/coreflowd.pid",report:"/data/adb/coreflow/install_report.txt",safe:"/data/adb/coreflow/SAFE_MODE",disable:"/data/adb/coreflow/DISABLE",module:"/data/adb/modules/coreflow_autonomous",trace:"/data/adb/coreflow/decision_trace.jsonl"};
const $=id=>document.getElementById(id);const els={};
["modeBadge","daemonStatus","lastRefresh","runtimeState","thermal","memory","load","caps","logBox","configBox","logPath","footerInfo","btnRefresh","modeHeadline","modeDescription","stateDot","chipArmed","chipCpuGov","chipInterval","eventList","thermalNote","memoryNote","thermalBar","memoryBar","loadBar","stateBar","confidenceNote","loadNote","cpuFreq","cpuFreqNote","traceSearch","traceFilter","traceSummary","traceList","requestedMode","effectiveArmed","effectiveCpu","safetyMarkers","safeModeBadge","reportBox","capabilityList","logSearch","logLines","errorsOnly","logSummary","terminalOutput","lastCommand","connectionLabel","connectionSub","allowCpuGov","observeCurrent","observeCard","adaptiveCard","cpuFreq","cpuFreqNote","traceSearch","traceFilter","traceSummary","traceList","historyStatus","histThermal","histMemory","histLoad","histCpu","sparkThermal","sparkMemory","sparkLoad","sparkCpu","btnClearHistory","safetyOverall","auditRequested","auditArmed","auditCpu","auditSafe","auditDisable","auditRestart","auditDaemon","safetyAuditNote"] .forEach(k=>els[k]=$(k));
let state={config:null,configRaw:"",log:"",logPath:"",report:"",daemon:"unknown",safe:false,disabled:false,busy:false,view:"overview",lastOutput:"",liveMetrics:{},traceEvents:[],history:[]};
const diagCommands={
 identity:{label:"device identity",cmd:"printf 'MODEL='; getprop ro.product.model; printf 'DEVICE='; getprop ro.product.device; printf 'ANDROID='; getprop ro.build.version.release; printf 'API='; getprop ro.build.version.sdk; printf 'KERNEL='; uname -r; printf 'ABI='; getprop ro.product.cpu.abi"},
 uptime:{label:"uptime & load",cmd:"printf '%s\\n' '--- uptime ---'; cat /proc/uptime; printf '%s\\n' '--- loadavg ---'; cat /proc/loadavg"},
 memory:{label:"memory snapshot",cmd:"printf '%s\\n' '--- meminfo ---'; head -n 18 /proc/meminfo; printf '%s\\n' '--- vmstat ---'; head -n 25 /proc/vmstat"},
 battery:{label:"battery status",cmd:"for f in /sys/class/power_supply/*/type /sys/class/power_supply/*/capacity /sys/class/power_supply/*/status /sys/class/power_supply/*/temp /sys/class/power_supply/*/voltage_now /sys/class/power_supply/*/current_now; do [ -r \"$f\" ] && { echo \"--- $f\"; cat \"$f\"; }; done"},
 thermal:{label:"thermal sensors",cmd:"n=0; for f in /sys/class/thermal/thermal_zone*/temp; do [ -r \"$f\" ] || continue; n=$((n+1)); echo \"--- $f\"; cat \"$f\"; z=${f%/temp}; [ -r \"$z/type\" ] && cat \"$z/type\"; done; [ \"$n\" -gt 0 ] || echo 'No readable thermal zone found.'"},
 process:{label:"CoreFlow process",cmd:"echo '--- pid file ---'; if [ -r /data/adb/coreflow/coreflowd.pid ]; then p=$(cat /data/adb/coreflow/coreflowd.pid); echo \"pid=$p\"; kill -0 \"$p\" 2>/dev/null && echo alive || echo not-alive; else echo 'PID file missing'; fi; echo '--- matching process ---'; ps -A 2>/dev/null | grep '[c]oreflowd' || true; echo '--- safety files ---'; for f in /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE; do [ -e \"$f\" ] && echo \"present: $f\" || echo \"absent: $f\"; done"},
 storage:{label:"storage capability",cmd:"n=0; for f in /sys/block/*/queue/read_ahead_kb /sys/block/*/queue/scheduler /sys/block/*/queue/nr_requests; do [ -r \"$f\" ] || continue; n=$((n+1)); echo \"--- $f\"; head -c 240 \"$f\"; echo; done; [ \"$n\" -gt 0 ] || echo 'No readable block queue nodes found.'"},
 safety:{label:"safety markers",cmd:"for f in /data/adb/coreflow/SAFE_MODE /data/adb/coreflow/DISABLE /data/adb/coreflow/coreflowd.lock /data/adb/coreflow/mutation_journal.txt /data/adb/coreflow_resource_mutation_journal.txt; do if [ -e \"$f\" ]; then echo \"PRESENT $f\"; [ -f \"$f\" ] && { ls -l \"$f\"; }; else echo \"ABSENT  $f\"; fi; done; echo '--- boot guard ---'; [ -r /data/adb/coreflow/boot_attempts ] && cat /data/adb/coreflow/boot_attempts || echo unavailable"}
};
function execShell(command){return new Promise(resolve=>{let done=false;const finish=(errno,stdout,stderr)=>{if(done)return;done=true;clearTimeout(timer);resolve({errno:Number(errno)||0,stdout:stdout==null?"":String(stdout),stderr:stderr==null?"":String(stderr)});};let timer=setTimeout(()=>finish(1,"","WebUI execution timed out"),15000);try{if(typeof ksu!=="undefined"&&typeof ksu.exec==="function"){const cb="cf_cb_"+Date.now()+"_"+Math.floor(Math.random()*1e6);window[cb]=(e,o,s)=>{try{delete window[cb]}catch(_){}finish(e,o,s)};try{ksu.exec(command,cb)}catch(e){try{ksu.exec(command,"{}",cb)}catch(e2){finish(1,"",String(e2||e))}}return;}finish(1,"","No KernelSU WebUI exec bridge. Open through a compatible KSU WebUI host.");}catch(e){finish(1,"",String(e))}})}
function toast(msg,error=false){const n=document.createElement("div");n.className="toast"+(error?" error":"");n.textContent=msg;$('toastRegion').appendChild(n);setTimeout(()=>n.remove(),3800)}
function iniParse(s){const out={};String(s||"").split(/\r?\n/).forEach(line=>{line=line.trim();if(!line||line[0]==="#"||line[0]===";")return;const i=line.indexOf("=");if(i>0)out[line.slice(0,i).trim()]=line.slice(i+1).trim()});return out}
function truth(v){return /^(1|true|yes|on)$/i.test(String(v||""))}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function setText(id,v){const x=$(id);if(x)x.textContent=v}
function showView(view){if(!$("view-"+view))return;state.view=view;document.querySelectorAll(".view").forEach(v=>v.classList.toggle("active",v.id==="view-"+view));document.querySelectorAll(".nav-item").forEach(b=>b.classList.toggle("active",b.dataset.view===view));const titles={overview:"Overview",control:"Mode & policy",diagnostics:"Diagnostics",trace:"Decision trace",terminal:"Shell console",about:"Project & AI"};setText("pageTitle",titles[view]||"CoreFlow");$('sidebar').classList.remove('open');if(view==="diagnostics")renderLog();if(view==="trace")renderTrace();}
function installNav(){document.querySelectorAll("[data-view]").forEach(b=>b.addEventListener("click",()=>showView(b.dataset.view)));document.querySelectorAll("[data-goto]").forEach(b=>b.addEventListener("click",()=>showView(b.dataset.goto)));$('menuToggle').addEventListener('click',()=>$('sidebar').classList.toggle('open'));}
function pickLogFile(){return execShell("if [ -r "+PATH.logs+"/coreflowd.log ]; then echo coreflowd.log; elif ls "+PATH.logs+"/*.log >/dev/null 2>&1; then ls -1 "+PATH.logs+"/*.log | tail -1 | xargs -n1 basename; else echo; fi").then(r=>{const name=(r.stdout||"").trim().split(/\r?\n/)[0]||"";return name&&/^[A-Za-z0-9_.-]+$/.test(name)?PATH.logs+"/"+name:""})}
function fromLog(text){const lines=String(text||"").split(/\r?\n/);const x={state:null,thermal:null,memory:null,load:null,caps:[],device:null};for(let i=lines.length-1;i>=0;i--){const l=lines[i];if(!x.state){const m=l.match(/\b(IDLE|NORMAL|WARMING|ELEVATED|PRESSURE|THERMAL_GUARD|THERMAL GUARD)\b/i);if(m)x.state=m[1]}if(!x.thermal){let m=l.match(/(\d+(?:\.\d+)?)\s*°?C\b/i)||l.match(/thermal[^\d]*(\d{4,6})/i);if(m){let v=Number(m[1]);x.thermal=(v>200&&String(m[1]).length>=4?(v/1000).toFixed(1):String(m[1]))+" °C"}}if(!x.memory){const m=l.match(/mem(?:ory)?[^\d]*(\d+(?:\.\d+)?)\s*%/i)||l.match(/(?:mem_available_ratio|available_ratio)[=: ]+(\d+(?:\.\d+)?)/i);if(m){let v=Number(m[1]);if(v<=1)v*=100;x.memory=v.toFixed(1)+"% free"}}if(!x.load){const m=l.match(/\bload(?:1)?[=: ]+(\d+(?:\.\d+)?)/i);if(m)x.load=m[1]}if(/CAPS\s+/.test(l)&&x.caps.length<4)x.caps.unshift(l.replace(/^.*CAPS\s+/,"CAPS ").trim());if(!x.device&&/DEVICE\s+/.test(l))x.device=l.replace(/^.*DEVICE\s+/,"DEVICE ").trim();if(x.state&&x.thermal&&x.memory&&x.load&&x.caps.length>=2)break}return x}
function updateMetric(id,noteId,barId,val,note,pct){setText(id,val||"Unavailable");setText(noteId,note);const bar=$(barId);if(bar)bar.style.width=(pct==null?0:Math.max(0,Math.min(100,pct)))+"%"}
function parseLiveMetrics(text){const out={};String(text||"").split(/\r?\n/).forEach(line=>{const i=line.indexOf("=");if(i>0)out[line.slice(0,i).trim()]=line.slice(i+1).trim()});return out}
function liveMetricCommand(){return atob("c2V0ICtlCnByaW50ZiAnU0FNUExFX1RJTUU9JXNcbicgIiQoZGF0ZSAnKyVZLSVtLSVkICVIOiVNOiVTICV6JyAyPi9kZXYvbnVsbCkiCnByaW50ZiAnTE9BRD0nOyBhd2sgJ3twcmludCAkMX0nIC9wcm9jL2xvYWRhdmcgMj4vZGV2L251bGwKYXdrICcvTWVtVG90YWw6LyB7dD0kMn0gL01lbUF2YWlsYWJsZTovIHthPSQyfSBFTkQge2lmKHQ+MCAmJiBhPj0wKSBwcmludGYgIk1FTU9SWT0lZCwlZCwlZFxuIix0LGEsMTAwKmEvdDsgZWxzZSBwcmludCAiTUVNT1JZPXVuYXZhaWxhYmxlIn0nIC9wcm9jL21lbWluZm8gMj4vZGV2L251bGwKbj0wOyBiZXN0PScnOyBiZXN0dHlwZT0nJwpmb3IgZiBpbiAvc3lzL2NsYXNzL3RoZXJtYWwvdGhlcm1hbF96b25lKi90ZW1wOyBkbyBbIC1yICIkZiIgXSB8fCBjb250aW51ZTsgdj0kKGNhdCAiJGYiIDI+L2Rldi9udWxsKTsgY2FzZSAiJHYiIGluICcnfCpbITAtOS1dKikgY29udGludWU7OyBlc2FjOyBjYXNlICIkdiIgaW4gLSopIGNvbnRpbnVlOzsgZXNhYzsgaWYgWyAiJHYiIC1ndCAwIF0gMj4vZGV2L251bGw7IHRoZW4gbj0kKChuKzEpKTsgaWYgWyAteiAiJGJlc3QiIF0gfHwgWyAiJHYiIC1ndCAiJGJlc3QiIF07IHRoZW4gYmVzdD0kdjsgej0ke2YlL3RlbXB9OyBiZXN0dHlwZT0kKGNhdCAiJHovdHlwZSIgMj4vZGV2L251bGwpOyBmaTsgZmk7IGRvbmUKaWYgWyAtbiAiJGJlc3QiIF07IHRoZW4gcHJpbnRmICdUSEVSTUFMX1JBVz0lc1xuVEhFUk1BTF9UWVBFPSVzXG5USEVSTUFMX1pPTkVTPSVzXG4nICIkYmVzdCIgIiRiZXN0dHlwZSIgIiRuIjsgZWxzZSBwcmludGYgJ1RIRVJNQUxfUkFXPXVuYXZhaWxhYmxlXG5USEVSTUFMX1pPTkVTPTBcbic7IGZpCmNwdT0nJzsgZm9yIGYgaW4gL3N5cy9kZXZpY2VzL3N5c3RlbS9jcHUvY3B1ZnJlcS9wb2xpY3kqL3NjYWxpbmdfY3VyX2ZyZXE7IGRvIFsgLXIgIiRmIiBdIHx8IGNvbnRpbnVlOyB2PSQoY2F0ICIkZiIgMj4vZGV2L251bGwpOyBjYXNlICIkdiIgaW4gJyd8KlshMC05XSopIGNvbnRpbnVlOzsgZXNhYzsgY3B1PSIkY3B1ICR7ZiUvc2NhbGluZ19jdXJfZnJlcX06JHYiOyBkb25lCmlmIFsgLW4gIiRjcHUiIF07IHRoZW4gcHJpbnRmICdDUFVfRlJFUT0lc1xuJyAiJHtjcHUjIH0iOyBlbHNlIHByaW50ZiAnQ1BVX0ZSRVE9dW5hdmFpbGFibGVcbic7IGZp")}function applyLiveMetrics(result){const m=parseLiveMetrics(result&&result.stdout||"");state.liveMetrics=m;const failed=!result||result.errno!==0;
 if(failed){updateMetric("thermal","thermalNote","thermalBar","Unavailable","Direct sensor read failed",null);updateMetric("memory","memoryNote","memoryBar","Unavailable","Direct /proc/meminfo read failed",null);updateMetric("load","loadNote","loadBar","Unavailable","Direct /proc/loadavg read failed",null);setText("cpuFreq","Unavailable");setText("cpuFreqNote","Direct cpufreq read failed");return}
 if(m.THERMAL_RAW&&m.THERMAL_RAW!=="unavailable"){let t=Number(m.THERMAL_RAW);if(t>=1000)t/=1000;const valid=Number.isFinite(t)&&t>=-20&&t<=200;updateMetric("thermal","thermalNote","thermalBar",valid?t.toFixed(1)+" °C":"Unavailable",valid?((m.THERMAL_TYPE||"Thermal zone")+" · direct read · "+(m.THERMAL_ZONES||"0")+" readable zones"):"Invalid sensor scale",null)}else updateMetric("thermal","thermalNote","thermalBar","Unavailable","No readable thermal zone",null);
 if(m.MEMORY&&m.MEMORY!=="unavailable"){const a=m.MEMORY.split(",").map(Number),total=a[0],avail=a[1],pct=a[2];if(total>0&&avail>=0&&pct>=0&&pct<=100)updateMetric("memory","memoryNote","memoryBar",pct.toFixed(1)+"% available",(avail/1024).toFixed(0)+" MiB available / "+(total/1024).toFixed(0)+" MiB",100-pct);else updateMetric("memory","memoryNote","memoryBar","Unavailable","Invalid memory sample",null)}else updateMetric("memory","memoryNote","memoryBar","Unavailable","Direct memory read unavailable",null);
 const load=Number(m.LOAD);updateMetric("load","loadNote","loadBar",m.LOAD&&Number.isFinite(load)?load.toFixed(2):"Unavailable","Direct /proc/loadavg · "+(m.SAMPLE_TIME||"sample time unavailable"),null);
 if(m.CPU_FREQ&&m.CPU_FREQ!=="unavailable"){const entries=m.CPU_FREQ.trim().split(/\s+/).map(part=>{const i=part.lastIndexOf(":");return i>0?{policy:part.slice(0,i).split("/").pop(),khz:Number(part.slice(i+1))}:null}).filter(x=>x&&Number.isFinite(x.khz)&&x.khz>0);if(entries.length){const vals=entries.map(x=>x.khz/1000);setText("cpuFreq",vals.length===1?vals[0].toFixed(0)+" MHz":Math.min(...vals).toFixed(0)+"–"+Math.max(...vals).toFixed(0)+" MHz");setText("cpuFreqNote",entries.length+" readable cpufreq policies · direct read")}else{setText("cpuFreq","Unavailable");setText("cpuFreqNote","No valid cpufreq sample")}}else{setText("cpuFreq","Unavailable");setText("cpuFreqNote","No readable cpufreq policy")}
}
function capabilityRows(report){
 const text=String(report||"");
 const kv=iniParse(text);
 const rows=[];
 const device=[kv.device_model,kv.soc_family,kv.soc_model].filter(Boolean).join(" · ");
 if(device)rows.push(["Device profile",device,"Reported","install report"]);
 if(kv.android)rows.push(["Android / API",kv.android,"Reported","install report"]);
 if(kv.kernel)rows.push(["Kernel",kv.kernel,"Reported","install report"]);
 if(kv.version)rows.push(["CoreFlow version",kv.version,"Reported","install report"]);
 if(kv.date)rows.push(["Probe report time",kv.date,"Timestamp","install report"]);
 const line=text.split(/\r?\n/).find(x=>/^\s*probe\s+/.test(x))||"";
 const p={};
 line.replace(/^\s*probe\s+/,"").replace(/([A-Za-z_][A-Za-z0-9_]*)=([^\s]+)/g,(_,k,v)=>{p[k]=v;return ""});
 const definitions=[
  ["thermal_read","Thermal zones readable","zones","count"],
  ["cpu_read","CPU policies readable","policies","count"],
  ["cpu_write","CPU policies reported writable","policies","count"],
  ["block","Block queue nodes readable","nodes","count"],
  ["battery","Battery telemetry","","flag"],
  ["swappiness","VM swappiness readable","","flag"]
 ];
 definitions.forEach(([key,label,unit,kind])=>{
  if(!(key in p))return;
  const v=p[key];
  let status="Reported";
  if(kind==="count"){
   const n=Number(v);
   status=Number.isFinite(n)?(n>0?(key==="cpu_write"?"Reported writable":"Available"):(key==="cpu_write"?"None reported writable":"Unavailable")):"Unknown";
  }else if(/^(yes|true|1)$/i.test(v))status="Available";
  else if(/^(no|false|0)$/i.test(v))status="Unavailable";
  rows.push([label,unit?`${v} ${unit}`:v,status,"install report / probe"]);
 });
 if(kv.model_digest)rows.push(["Model digest",String(kv.model_digest).slice(0,16)+"…","Reported","install report"]);
 return rows;
}
function renderCapabilityMatrix(){
 const host=$("capabilityList");if(!host)return;
 const rows=capabilityRows(state.report);
 host.replaceChildren();
 if(!rows.length){const d=document.createElement("div");d.className="empty-state";d.textContent="No structured capability data found in the install report. Refresh the report or inspect diagnostics.";host.appendChild(d);return}
 const wrap=document.createElement("div");wrap.className="cap-table-wrap";
 const table=document.createElement("table");table.className="capability-table";
 const thead=document.createElement("thead");thead.innerHTML="<tr><th>RESOURCE</th><th>VALUE</th><th>STATUS</th><th>SOURCE</th></tr>";table.appendChild(thead);
 const tbody=document.createElement("tbody");
 rows.forEach(r=>{const tr=document.createElement("tr");r.forEach((v,i)=>{const td=document.createElement("td");if(i===2){const b=document.createElement("span");b.className="cap-status "+(/available|writable|timestamp/i.test(v)?"available":/unavailable|none/i.test(v)?"limited":"unknown");b.textContent=v;td.appendChild(b)}else td.textContent=v;tr.appendChild(td)});tbody.appendChild(tr)});
 table.appendChild(tbody);wrap.appendChild(table);host.appendChild(wrap);
 const note=document.createElement("p");note.className="source-note";note.textContent="Capability values are from the install report/probe, not a live write test. “Reported writable” is informational and does not authorize mutation.";host.appendChild(note);
}
function classifyTrace(line){const l=line.toLowerCase();if(/error|fatal|failed|failure|exception|warn/.test(l))return "error";if(/safe_mode|disable|safety_hold|rollback|restore|boot.loop|journal/.test(l))return "safety";if(/mutation|applied|apply|write|verify|verified|decision|policy|skip|reject|adaptive|observe|confidence|effect.model/.test(l))return "decision";return "event"}

function parseStructuredTrace(raw){
 const out=[];
 String(raw||"").split(/\r?\n/).filter(Boolean).forEach(line=>{
  try{const event=JSON.parse(line);if(event&&event.schema==="coreflow.decision.v1")out.push(event)}catch(_){}
 });
 return out;
}
function renderStructuredTrace(events,host){
 const search=($("traceSearch")?.value||"").toLowerCase();
 const filter=$("traceFilter")?.value||"all";
 const rows=events.slice().reverse().filter(e=>{
  const blob=JSON.stringify(e).toLowerCase();
  if(!blob.includes(search))return false;
  if(filter==="decision")return !!e.decision;
  if(filter==="mutation")return /VERIFIED|FAILED|ROLLED_BACK|APPLIED/.test(String(e.cpu_mutation)+" "+String(e.resource_mutation));
  if(filter==="safety")return e.reason==="SAFETY_HOLD"||String(e.safety_hold||"").toLowerCase()!=="none";
  if(filter==="error")return /FAILED|ROLLED_BACK/.test(String(e.cpu_mutation)+" "+String(e.resource_mutation))||e.reason==="MUTATION_FAILED";
  return true;
 });
 host.replaceChildren();
 setText("traceSummary",rows.length+" structured event(s) shown · "+events.length+" loaded from C++ journal");
 if(!rows.length){const d=document.createElement("div");d.className="empty-state";d.textContent=events.length?"No structured events match this filter.":"No structured journal events yet. This can mean the updated daemon has not written its first cycle, or the journal is not readable.";host.appendChild(d);return}
 rows.slice(0,200).forEach(e=>{
  const row=document.createElement("div");row.className="trace-row "+(e.reason==="SAFETY_HOLD"?"safety":/FAILED|ROLLED_BACK/.test(String(e.cpu_mutation)+" "+String(e.resource_mutation))?"error":"");
  const dot=document.createElement("span");dot.className="trace-dot";
  const body=document.createElement("div");
  const title=document.createElement("b");title.textContent=`Cycle ${e.cycle ?? "?"} · ${e.decision||"Unknown decision"} · ${e.state||"Unknown state"}`;
  const p=document.createElement("p");const tag=document.createElement("span");tag.className="trace-tag";tag.textContent=e.reason||"EVALUATED";p.appendChild(tag);p.appendChild(document.createTextNode(" "+(e.timestamp||"timestamp unavailable")));
  const detail=document.createElement("p");detail.textContent=`confidence ${Number.isFinite(Number(e.confidence))?Number(e.confidence).toFixed(2):"unknown"} · workload ${e.workload||"unknown"} · eligible ${typeof e.mutation_eligible==="boolean"?(e.mutation_eligible?"yes":"no"):"unknown"} · candidates ${e.candidate_count??"unknown"}`;
  const result=document.createElement("p");result.textContent=`CPU: ${e.cpu_mutation||"unknown"} · Resource: ${e.resource_mutation||"unknown"} · hold: ${e.safety_hold||"unknown"} · mode: ${e.mode||"unknown"} · armed: ${typeof e.mutation_armed==="boolean"?(e.mutation_armed?"yes":"no"):"unknown"}`;
  body.append(title,p,detail,result);row.append(dot,body);host.appendChild(row);
 });
}
function renderTrace(){const host=$("traceList");if(!host)return;if(Array.isArray(state.traceEvents)&&state.traceEvents.length){renderStructuredTrace(state.traceEvents,host);return}const search=($("traceSearch")?.value||"").toLowerCase();const filter=$("traceFilter")?.value||"all";const all=String(state.log||"").split(/\r?\n/).filter(Boolean).slice(-500).reverse();const rows=all.map(line=>({line,type:classifyTrace(line)})).filter(x=>(filter==="all"||(filter==="decision"&&x.type==="decision")||(filter==="mutation"&&/mutation|applied|apply|write|verify|rollback|restore/i.test(x.line))||(filter==="safety"&&x.type==="safety")||(filter==="error"&&x.type==="error"))&&x.line.toLowerCase().includes(search));host.replaceChildren();setText("traceSummary",rows.length+" event(s) shown · based on "+all.length+" recent log lines");if(!rows.length){const d=document.createElement("div");d.className="empty-state";d.textContent="No matching recorded events. The trace will not synthesize decisions missing from the runtime log.";host.appendChild(d);return}rows.slice(0,200).forEach(x=>{const row=document.createElement("div");row.className="trace-row "+(x.type==="error"?"error":x.type==="safety"?"safety":"");const dot=document.createElement("span");dot.className="trace-dot";const body=document.createElement("div");const b=document.createElement("b");b.textContent=x.line;const p=document.createElement("p");const tm=x.line.match(/^\d{4}-\d\d-\d\d[ T]\d\d:\d\d:\d\d/);const tag=document.createElement("span");tag.className="trace-tag";tag.textContent=x.type.toUpperCase();p.appendChild(tag);p.appendChild(document.createTextNode(tm?tm[0]:"Recorded runtime log"));body.append(b,p);row.append(dot,body);host.appendChild(row)})}

function renderEvents(text){const lines=String(text||"").trim().split(/\r?\n/).filter(Boolean).slice(-6).reverse();const host=$('eventList');host.replaceChildren();if(!lines.length){const d=document.createElement('div');d.className='empty-state';d.textContent='No events in selected log.';host.appendChild(d);return}lines.forEach(line=>{const row=document.createElement('div');const low=line.toLowerCase();row.className='event-item'+(/error|fail|fatal/.test(low)?' error':/warn|thermal_guard|safety_hold/.test(low)?' warning':'');const bullet=document.createElement('span');bullet.className='event-bullet';const body=document.createElement('div');const b=document.createElement('b');b.textContent=line.length>190?line.slice(0,187)+'…':line;const sm=document.createElement('small');const tm=line.match(/^\d{4}-\d\d-\d\d[ T]\d\d:\d\d:\d\d/);sm.textContent=tm?tm[0]:'CoreFlow event';body.append(b,sm);row.append(bullet,body);host.appendChild(row)})}
function renderCapabilities(parsed,report){const all=[];if(parsed.device)all.push(['Device profile',parsed.device]);parsed.caps.forEach((v,i)=>all.push(['Capability '+(i+1),v]));if(report){report.split(/\r?\n/).filter(l=>/^probe\s|^device_model=|^soc_model=|^soc_family=|^android=|^kernel=/.test(l)).slice(0,8).forEach(l=>all.push([l.split(/[ =]/)[0],l]))}const host=$('capabilityList');host.replaceChildren();if(!all.length){const d=document.createElement('div');d.className='empty-state';d.textContent='No capability lines found. Run refresh or check install_report.txt.';host.appendChild(d);return}all.slice(0,12).forEach(([k,v])=>{const row=document.createElement('div');row.className='cap-item';const a=document.createElement('span'),b=document.createElement('b');a.textContent=k;b.textContent=v;row.append(a,b);host.appendChild(row)})}
function renderLog(){const search=($('logSearch').value||'').toLowerCase();const n=Number($('logLines').value)||80;let lines=state.log.split(/\r?\n/).filter(Boolean).slice(-n);if($('errorsOnly').checked)lines=lines.filter(x=>/error|fail|fatal|warn|safety_hold|thermal_guard/i.test(x));if(search)lines=lines.filter(x=>x.toLowerCase().includes(search));const out=lines.join('\n')||'(No matching lines)';setText('logBox',out);setText('logSummary',lines.length+' line(s) shown · '+(state.log.split(/\r?\n/).filter(Boolean).length)+' loaded');}
function updateConfigUI(){const c=state.config||{};const mode=(c.mutation_mode||'observe').toLowerCase();const armed=truth(c.mutation_armed);const cpu=truth(c.allow_cpu_governor);const active=mode==='adaptive'&&armed&&!state.safe&&!state.disabled;setText('modeHeadline',state.safe?'SAFE MODE — mutations blocked':state.disabled?'DISABLE safety hold active':state.restartPending?'Configuration saved · restart required':active?'Adaptive policy configured':'Observe mode configured');setText('modeDescription',state.safe?'Engine should hold Observe until SAFE_MODE is cleared through recovery.':state.disabled?'Mutation hold active. Adaptive requires an explicit resume confirmation.':state.restartPending?'Configuration changed; restart/reboot required for daemon to load the new mode.':active?'Eligible changes remain behind capability and policy gates.':'Diagnostics and discovery remain available; no mutation is armed.');$('stateDot').className='status-dot'+(state.safe||state.disabled?' unknown':active?' adaptive':'');setText('chipArmed','armed: '+(c.mutation_armed||'false'));setText('chipCpuGov','CPU governor: '+(c.allow_cpu_governor||'no'));setText('chipInterval','interval: '+(c.monitor_interval||'5')+'s');setText('requestedMode',mode.toUpperCase());setText('effectiveArmed',active?'YES':'NO');setText('effectiveCpu',active&&cpu?'YES':'NO');setText('safetyMarkers',(state.safe?'SAFE_MODE ':'')+(state.disabled?'DISABLE ':'')||'None detected');setText('safeModeBadge',state.safe?'SAFE_MODE ACTIVE':state.disabled?'DISABLE MARKER':'SAFETY CHECKED');$('safeModeBadge').style.color=state.safe||state.disabled?'var(--red)':'var(--green)';setText('observeCurrent',!active?'CURRENT MODE':'AVAILABLE');$('observeCard').classList.toggle('current-mode',!active);$('adaptiveCard').classList.toggle('current-mode',active);$('allowCpuGov').checked=cpu;$('allowCpuGov').disabled=state.safe;$('btnSetObserve').textContent=!active?'Observe is active':'Switch to Observe';$('btnSetAdaptive').textContent=active?'Adaptive is active':(state.disabled?'Review & resume Adaptive':'Review & enable Adaptive');$('btnSetAdaptive').disabled=state.safe;setText('footerInfo',(active?'Adaptive armed':'Observe-only')+' · KSU WebUI bridge');}

function finiteNumber(v){const n=Number(v);return Number.isFinite(n)?n:null}
function sampleHistory(){
 const m=state.liveMetrics||{},now=new Date();
 const thermal=(m.THERMAL_RAW&&m.THERMAL_RAW!=="unavailable")?Number(m.THERMAL_RAW):null;
 const t=thermal!==null?(thermal>=1000?thermal/1000:thermal):null;
 const mem=(m.MEMORY&&m.MEMORY!=="unavailable")?m.MEMORY.split(",").map(Number):null;
 const load=(m.LOAD&&m.LOAD!=="unavailable")?finiteNumber(m.LOAD):null;
 let cpu=null;
 if(m.CPU_FREQ&&m.CPU_FREQ!=="unavailable"){
  const vals=m.CPU_FREQ.trim().split(/\s+/).map(x=>Number(x.slice(x.lastIndexOf(":")+1))).filter(x=>Number.isFinite(x)&&x>0);
  if(vals.length)cpu={min:Math.min(...vals)/1000,max:Math.max(...vals)/1000};
 }
 const sample={time:now.toISOString(),label:now.toLocaleTimeString(),thermal:t,memory:mem&&mem.length>=3&&mem[0]>0?mem[2]:null,load,cpu};
 // Keep gaps as null and bound memory usage; don't interpolate missing values.
 state.history.push(sample);if(state.history.length>60)state.history=state.history.slice(-60);
 renderHistory();
}
function sparkline(id,key,project){
 const svg=$(id);if(!svg)return;svg.replaceChildren();
 const values=state.history.map(s=>project?project(s[key]):s[key]);
 const valid=values.map((v,i)=>({v,i})).filter(x=>Number.isFinite(x.v));
 const base=document.createElementNS("http://www.w3.org/2000/svg","line");base.setAttribute("x1","4");base.setAttribute("x2","296");base.setAttribute("y1","48");base.setAttribute("y2","48");svg.appendChild(base);
 if(valid.length<2){const p=document.createElementNS("http://www.w3.org/2000/svg","polyline");p.setAttribute("class","empty-history");p.setAttribute("points",valid.length?`${valid[0].i*292/Math.max(1,values.length-1)+4},27 ${valid[0].i*292/Math.max(1,values.length-1)+4},27`:"4,27 296,27");svg.appendChild(p);return}
 const nums=valid.map(x=>x.v),min=Math.min(...nums),max=Math.max(...nums),range=max-min||1;
 const pts=valid.map(x=>`${(x.i/Math.max(1,values.length-1))*292+4},${46-((x.v-min)/range)*36}`).join(" ");
 const line=document.createElementNS("http://www.w3.org/2000/svg","polyline");line.setAttribute("points",pts);svg.appendChild(line);
}
function renderHistory(){
 const h=state.history;setText("historyStatus",h.length+" sample(s) · latest "+(h.length?h[h.length-1].label:"—"));
 const last=h[h.length-1]||{};
 setText("histThermal",Number.isFinite(last.thermal)?last.thermal.toFixed(1)+" °C":"Unavailable");
 setText("histMemory",Number.isFinite(last.memory)?last.memory.toFixed(1)+"% available":"Unavailable");
 setText("histLoad",Number.isFinite(last.load)?last.load.toFixed(2):"Unavailable");
 setText("histCpu",last.cpu?Math.round(last.cpu.min)+"–"+Math.round(last.cpu.max)+" MHz":"Unavailable");
 sparkline("sparkThermal","thermal");sparkline("sparkMemory","memory");sparkline("sparkLoad","load");sparkline("sparkCpu","cpu",v=>v&&((v.min+v.max)/2));
}
function renderSafetyAudit(){
 const c=state.config||{},mode=(c.mutation_mode||"observe").toLowerCase(),armed=truth(c.mutation_armed),cpu=truth(c.allow_cpu_governor),daemon=(state.daemon||"").toLowerCase();
 const alive=daemon.includes("running"),adaptiveRequested=mode==="adaptive"&&armed;
 const known=!!state.configRaw&&!!state.daemon;
 const blocked=state.safe||state.disabled;
 setText("auditRequested",mode.toUpperCase());
 setText("auditArmed",armed?"YES":"NO");
 setText("auditCpu",cpu?"YES":"NO");
 setText("auditSafe",state.safe?"PRESENT":"ABSENT");
 setText("auditDisable",state.disabled?"PRESENT":"ABSENT");
 setText("auditRestart",state.restartPending?"PENDING":"NO");
 setText("auditDaemon",alive?"RUNNING":state.daemon||"UNKNOWN");
 const status=!known?"UNKNOWN":(blocked||!alive)?"REVIEW":(!adaptiveRequested&&mode==="observe"&&!armed&&!cpu)?"OBSERVE-ONLY":adaptiveRequested&&!blocked&&alive?"ADAPTIVE REQUESTED":"REVIEW";
 const pill=$("safetyOverall");if(pill){pill.textContent=status;pill.className="cap-status "+(status==="OBSERVE-ONLY"?"available":status==="UNKNOWN"?"unknown":"limited")}
 setText("safetyAuditNote",state.restartPending?"Configuration changes are pending restart; the running daemon may still use its previous configuration.":blocked?"A safety marker is present. Treat mutation as blocked and inspect engine logs.":!alive?"Daemon is not confirmed running. Effective runtime mode is unknown.":adaptiveRequested?"Adaptive is requested by configuration; this UI cannot independently prove all engine-side gates or mutations.":"Current config requests Observe-only. Marker absence alone is not proof of safety enforcement.");
}
function buildDiagnosticReport(){
 const m=state.liveMetrics||{},c=state.config||{},now=new Date().toISOString();
 return [
  "CoreFlow Autonomous — Diagnostic Report v2",
  "generated_at="+now,
  "engine_version="+((state.report.match(/^version=(.+)$/m)||[])[1]||"unknown"),
  "device="+((state.report.match(/^device_model=(.+)$/m)||[])[1]||"unknown"),
  "android="+((state.report.match(/^android=(.+)$/m)||[])[1]||"unknown"),
  "kernel="+((state.report.match(/^kernel=(.+)$/m)||[])[1]||"unknown"),
  "daemon_status="+(state.daemon||"unknown"),
  "config_mode="+(c.mutation_mode||"unknown"),
  "mutation_armed="+(c.mutation_armed||"unknown"),
  "allow_cpu_governor="+(c.allow_cpu_governor||"unknown"),
  "safe_mode_marker="+(state.safe?"present":"absent"),
  "disable_marker="+(state.disabled?"present":"absent"),
  "restart_pending="+(state.restartPending?"yes":"no"),
  "telemetry_sample_time="+(m.SAMPLE_TIME||"unavailable"),
  "thermal_raw="+(m.THERMAL_RAW||"unavailable"),
  "thermal_type="+(m.THERMAL_TYPE||"unavailable"),
  "memory_raw="+(m.MEMORY||"unavailable"),
  "load_1m="+(m.LOAD||"unavailable"),
  "cpu_freq_raw="+(m.CPU_FREQ||"unavailable"),
  "capability_report_time="+((state.report.match(/^date=(.+)$/m)||[])[1]||"unknown"),
  "capability_probe="+((state.report.match(/^probe\s+(.+)$/m)||[])[1]||"unavailable"),
  "structured_trace_events="+state.traceEvents.length,
  "history_samples_in_session="+state.history.length,
  "recent_log_lines="+String(state.log||"").split(/\r?\n/).filter(Boolean).length,
  "",
  "Note: this report separates current UI-read values from install-time probe data. Marker absence does not prove engine safety. Sensitive device identifiers should be reviewed before sharing."
 ].join("\n");
}
function refresh(){if(state.busy)return;state.busy=true;setText('daemonStatus','Refreshing…');Promise.all([execShell('cat '+PATH.config+' 2>/dev/null'),execShell('if [ -r '+PATH.pid+' ]; then p=$(cat '+PATH.pid+' 2>/dev/null); if [ -n "$p" ] && kill -0 "$p" 2>/dev/null; then echo "running pid=$p"; else echo "stale pid file"; fi; else pgrep -f coreflowd >/dev/null 2>&1 && echo "running (pgrep)" || echo "not running"; fi'),pickLogFile(),execShell('[ -f '+PATH.safe+' ] && echo present || echo absent'),execShell('[ -f '+PATH.disable+' ] && echo present || echo absent'),execShell('cat '+PATH.report+' 2>/dev/null'),execShell('[ -f '+PATH.base+'/webui_config_pending ] && echo present || echo absent'),execShell(liveMetricCommand()),execShell('tail -n 200 '+PATH.trace+' 2>/dev/null')]).then(async r=>{state.configRaw=r[0].stdout||'';state.config=iniParse(state.configRaw);state.daemon=(r[1].stdout||r[1].stderr||'unknown').trim();state.logPath=r[2]||'';state.safe=(r[3].stdout||'').trim()==='present';state.disabled=(r[4].stdout||'').trim()==='present';state.report=r[5].stdout||'';state.restartPending=(r[6].stdout||'').trim()==='present';state.traceEvents=parseStructuredTrace(r[8]?.stdout||'');if(state.logPath){const lr=await execShell('tail -n 500 '+state.logPath+' 2>/dev/null');state.log=lr.stdout||lr.stderr||'(Log is empty)'}else state.log='(No log file found in /data/adb/coreflow/logs)';setText('daemonStatus',state.daemon);setText('lastRefresh',new Date().toLocaleTimeString());const p=fromLog(state.log);setText('runtimeState',state.daemon.toLowerCase().includes('running')?'Daemon alive':'Unknown');applyLiveMetrics(r[7]);sampleHistory();renderSafetyAudit();setText('caps',[p.device,...p.caps].filter(Boolean).join('\n')||'No CAPS/DEVICE lines in recent log.');setText('logPath',state.logPath||PATH.logs);setText('configBox',state.configRaw.trim()||'(config.ini missing)');setText('reportBox',state.report.trim()||'(Install report unavailable)');renderEvents(state.log);renderCapabilityMatrix();updateConfigUI();renderSafetyAudit();renderLog();renderTrace();setText("reportBox",buildDiagnosticReport()+"\n\n--- ORIGINAL INSTALL REPORT ---\n"+(state.report||"Install report unavailable."));const bridgeReads=r.filter(x=>x&&typeof x==='object'&&Object.prototype.hasOwnProperty.call(x,'errno'));const bridgeOk=bridgeReads.length>0&&bridgeReads.every(x=>x.errno===0);setText('connectionLabel',bridgeOk?'Bridge connected':'Bridge partially available');setText('connectionSub',bridgeOk?'KSU WebUI execution ready':'Some reads failed — inspect diagnostics');}).catch(e=>toast('Refresh failed: '+e,true)).finally(()=>{state.busy=false})}
function quote(s){return "'"+String(s).replace(/'/g,"'\\''")+"'"}
function writeConfig(updates,options={}){if(!state.configRaw||!state.config||!state.config.mutation_mode){toast('Cannot change mode: config.ini was not read successfully or has no mutation_mode.',true);return Promise.resolve(false)}const clearDisable=!!options.clearDisable;if(state.safe){toast('SAFE_MODE is active. Adaptive remains blocked until the normal recovery procedure resolves it.',true);return Promise.resolve(false)}if(state.disabled&&updates.mutation_mode==='adaptive'&&!clearDisable){toast('DISABLE safety hold is active. Explicitly confirm resume to remove the operator kill switch.',true);return Promise.resolve(false)}const allowed=new Set(['mutation_mode','mutation_armed','allow_cpu_governor']);if(Object.keys(updates).some(k=>!allowed.has(k)))return Promise.resolve(false);let rows=state.configRaw.replace(/\r/g,'').split('\n');for(const [key,value] of Object.entries(updates)){let found=false;rows=rows.map(line=>{if(line.trim().startsWith(key+'=')){found=true;return key+'='+value}return line});if(!found)rows.push(key+'='+value)}const newRaw=rows.join('\n').replace(/\n*$/,'')+'\n';const mode=iniParse(newRaw);if(mode.mutation_mode==='adaptive'&&(!truth(mode.mutation_armed))){toast('Refusing invalid adaptive config without mutation_armed=true.',true);return Promise.resolve(false)}if(mode.mutation_mode!=='adaptive'&&truth(mode.mutation_armed)){toast('Refusing config that arms mutation outside adaptive mode.',true);return Promise.resolve(false)}const script='set -eu; CFG='+quote(PATH.config)+'; DIR='+quote(PATH.base)+'; TMP="$CFG.webui.tmp.$$"; [ -d "$DIR" ] || exit 21; umask 077; printf %s '+quote(newRaw)+' > "$TMP"; chmod 0600 "$TMP"; [ -s "$TMP" ]; grep -qx '+quote('mutation_mode='+mode.mutation_mode)+' "$TMP"; grep -qx '+quote('mutation_armed='+mode.mutation_armed)+' "$TMP"; grep -qx '+quote('allow_cpu_governor='+(mode.allow_cpu_governor||'no'))+' "$TMP"; if [ "'+mode.mutation_mode+'" = observe ]; then touch "$DIR/DISABLE"; fi; mv "$TMP" "$CFG"; chmod 0600 "$CFG"; if [ "'+mode.mutation_mode+'" = adaptive ] && [ "'+(clearDisable?'yes':'no')+'" = yes ]; then rm -f "$DIR/DISABLE"; fi; touch "$DIR/webui_config_pending"; chmod 0600 "$DIR/webui_config_pending"; echo CONFIG_COMMIT_OK';return execShell(script).then(r=>{if(r.errno!==0||!r.stdout.includes('CONFIG_COMMIT_OK')){toast('Config write rejected: '+(r.stderr||r.stdout||'unknown error'),true);return false}state.configRaw=newRaw;state.config=iniParse(newRaw);setText('configBox',newRaw.trim());updateConfigUI();toast(mode.mutation_mode==='observe'?'Observe saved; runtime DISABLE safety hold requested. Reboot to start cleanly in Observe.':'Configuration saved. Restart/reboot may be required before the daemon applies the selected mode.');return true})}
function openConfirm(title,message,detail,confirmLabel){return new Promise(resolve=>{const d=$('confirmDialog');setText('dialogTitle',title);setText('dialogMessage',message);setText('dialogDetail',detail);setText('dialogConfirm',confirmLabel||'Confirm');const btn=$('dialogConfirm');const handler=e=>{if(e.submitter&&e.submitter.value==='confirm'){e.preventDefault();d.close('confirm')} };d.querySelector('form').addEventListener('submit',handler,{once:true});d.addEventListener('close',()=>resolve(d.returnValue==='confirm'),{once:true});d.showModal()})}
async function setObserve(){const ok=await openConfirm('Return to Observe mode?','This saves Observe and creates CoreFlow’s DISABLE kill-switch so a running Adaptive daemon enters its own safety-hold/restore path. The WebUI does not write kernel tunables. A later Adaptive opt-in must explicitly remove that marker.','mutation_mode=observe\nmutation_armed=false\nallow_cpu_governor=no\ncreate /data/adb/coreflow/DISABLE','Switch to Observe');if(!ok)return;const done=await writeConfig({mutation_mode:'observe',mutation_armed:'false',allow_cpu_governor:'no'});if(done)refresh()}
async function setAdaptive(){if(state.safe){toast('Adaptive is blocked by SAFE_MODE until the normal recovery procedure resolves it.',true);return}const cpu=$('allowCpuGov').checked;const ok=await openConfirm(state.disabled?'Resume Adaptive mode?':'Enable Adaptive mode?',state.disabled?'This removes the operator DISABLE kill-switch and arms Adaptive. Do not proceed unless you intentionally want to resume; SAFE_MODE remains non-bypassable.':'You are authorizing CoreFlow to attempt bounded mutations in the supported, discovered resource domains. It does not authorize all writable kernel controls.','mutation_mode=adaptive\nmutation_armed=true\nallow_cpu_governor='+(cpu?'yes':'no')+'\n\nCapability / policy gates, journal, verification, rollback and safety hold remain active.','Arm Adaptive');if(!ok)return;const done=await writeConfig({mutation_mode:'adaptive',mutation_armed:'true',allow_cpu_governor:cpu?'yes':'no'},{clearDisable:true});if(done)refresh()}
function runDiagnostic(name){const item=diagCommands[name];if(!item)return;setText('lastCommand',item.label);setText('terminalOutput','Running '+item.label+'…');state.lastOutput='';execShell(item.cmd).then(r=>{const out=(r.stdout||'').trim();const err=(r.stderr||'').trim();state.lastOutput=[out,err?'[stderr]\n'+err:''].filter(Boolean).join('\n\n')||'(No output)';setText('terminalOutput',(r.errno?'[exit '+r.errno+']\n':'[read-only diagnostic]\n')+state.lastOutput);if(r.errno)toast('Diagnostic finished with an error.',true)}).catch(e=>setText('terminalOutput','Diagnostic failed: '+String(e)))}
function copyText(text){if(navigator.clipboard&&navigator.clipboard.writeText){navigator.clipboard.writeText(text).then(()=>toast('Copied to clipboard')).catch(()=>fallbackCopy(text))}else fallbackCopy(text)}function fallbackCopy(text){const a=document.createElement('textarea');a.value=text;a.style.position='fixed';a.style.opacity='0';document.body.appendChild(a);a.select();try{document.execCommand('copy');toast('Copied')}catch(e){toast('Copy unavailable in this WebView.',true)}a.remove()}
function refreshRuntime(){const cmd="p=$(pidof coreflowd 2>/dev/null); [ -n \"$p\" ] || { echo 'coreflowd PID not found'; exit 2; }; ok=0; for one in $p; do case \"$one\" in ''|*[!0-9]*) continue;; esac; if kill -USR1 \"$one\" 2>/dev/null; then ok=1; fi; done; [ \"$ok\" -eq 1 ] && echo REFRESH_SIGNAL_SENT || { echo 'Could not signal coreflowd'; exit 1; }";return execShell(cmd).then(r=>{if(r.errno===0&&r.stdout.includes('REFRESH_SIGNAL_SENT'))toast('Runtime discovery refresh signal sent to coreflowd. Check logs for completion.');else toast(r.stderr||r.stdout||'Refresh signal failed.',true)})}
function init(){installNav();$('btnRefresh').addEventListener('click',refresh);$('btnCheckBridge').addEventListener('click',()=>refresh());$('btnRefreshLog').addEventListener('click',refresh);$('btnRefreshReport').addEventListener('click',refresh);$('btnRefreshTrace').addEventListener('click',refresh);$('btnClearHistory').addEventListener('click',()=>{state.history=[];renderHistory();toast('Session telemetry history cleared')});$('traceSearch').addEventListener('input',renderTrace);$('traceFilter').addEventListener('change',renderTrace);$('logSearch').addEventListener('input',renderLog);$('logLines').addEventListener('change',renderLog);$('errorsOnly').addEventListener('change',renderLog);$('btnCopyLog').addEventListener('click',()=>copyText(state.log));$('btnCopyConfig').addEventListener('click',()=>copyText(state.configRaw));$('btnReadReport').addEventListener('click',()=>showView('diagnostics'));$('btnCopyReport').addEventListener('click',()=>copyText(buildDiagnosticReport()+'\n\n--- ORIGINAL INSTALL REPORT ---\n'+(state.report||'Install report unavailable.')));$('btnCopyTerminal').addEventListener('click',()=>copyText(state.lastOutput||$('terminalOutput').textContent));$('btnClearTerminal').addEventListener('click',()=>{state.lastOutput='';setText('terminalOutput','Console output cleared.');setText('lastCommand','select a diagnostic command')});document.querySelectorAll('[data-command]').forEach(b=>b.addEventListener('click',()=>runDiagnostic(b.dataset.command)));$('btnSetObserve').addEventListener('click',setObserve);$('btnRestoreObserve').addEventListener('click',setObserve);$('btnSetAdaptive').addEventListener('click',setAdaptive);$('btnRefreshLog').addEventListener('click',refreshRuntime);document.querySelectorAll('.nav-item').forEach(b=>b.addEventListener('click',()=>showView(b.dataset.view)));refresh();}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);else init();
})();