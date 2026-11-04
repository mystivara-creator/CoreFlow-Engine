import React, { useMemo, useState } from 'react';
import { X, Copy, Download, Activity, Cpu, Code2 } from 'lucide-react';
import blueprint from '../../docs/EQUILIBRIUM_BLUEPRINT.md?raw';
import rustController from '../../rust/src/controller.rs?raw';
import rustBridge from '../../rust/src/bridge.rs?raw';
import cppController from '../../src/kernel/frequency_controller.cpp?raw';
import powerBridge from '../../src/kernel/power_bridge.cpp?raw';

interface ArchitectureStudioModalProps {
  isOpen: boolean;
  onClose: () => void;
  socModel: string;
  kernelVersion: string;
  onlineCpus: string;
  gpuType: string;
}
const tabs = ['Architecture', 'Mutation Safety', 'HAL / Binder', 'Thermal simulator', 'Preservation', 'C++ / Rust', 'Export'] as const;
type Tab = typeof tabs[number];

export const ArchitectureStudioModal: React.FC<ArchitectureStudioModalProps> = (props) => {
  const [tab, setTab] = useState<Tab>('Architecture');
  const [load, setLoad] = useState(75);
  const [ambient, setAmbient] = useState(30);
  const [copied, setCopied] = useState(false);
  const [copyError, setCopyError] = useState('');
  const [source, setSource] = useState('Rust controller');
  const sources: Record<string, string> = { 'Rust controller': rustController, 'cxx bridge': rustBridge, 'CPU/GPU controller': cppController, 'Power HAL probe': powerBridge };
  const simulation = useMemo(() => {
    let temperature = ambient, integral = 0, previous = temperature, scale = 1;
    const samples: { temperature: number; scale: number }[] = [];
    for (let second = 0; second <= 120; second++) {
      const error = 42.5 - temperature;
      integral = Math.max(-50, Math.min(0, integral + error));
      const velocity = second === 0 ? 0 : temperature - previous;
      const target = Math.max(0.4, Math.min(1, 1 + (2.5 * error + 0.15 * integral - 1.2 * velocity) / 50));
      scale += Math.max(-0.02, Math.min(0.02, target - scale));
      previous = temperature;
      // An illustrative first-order thermal plant, not a model fitted to this phone.
      temperature += load / 100 * scale * 0.5 - (temperature - ambient) * 0.025;
      samples.push({ temperature, scale });
    }
    return samples;
  }, [load, ambient]);
  if (!props.isOpen) return null;
  const fullText = blueprint + `\n\n## CFAE System Framework v3.0.R\n## Current hardware\nSoC: ${props.socModel}\nKernel: ${props.kernelVersion}\nCPUs: ${props.onlineCpus}\nGPU: ${props.gpuType}\n`;
  const copy = async () => {
    try { await navigator.clipboard.writeText(fullText); setCopied(true); setCopyError(''); }
    catch { setCopyError('Clipboard unavailable. Use Download.'); }
  };
  const download = (extension: string) => {
    const url = URL.createObjectURL(new Blob([fullText], { type: 'text/plain;charset=utf-8' }));
    const anchor = document.createElement('a'); anchor.href = url; anchor.download = `cfae_framework_blueprint.${extension}`;
    document.body.appendChild(anchor); anchor.click(); anchor.remove();
    window.setTimeout(() => URL.revokeObjectURL(url), 1000);
  };
  const temperaturePoints = simulation.map((s, i) => `${i * 5},${200 - (s.temperature - 20) * 4}`).join(' ');
  const capacityPoints = simulation.map((s, i) => `${i * 5},${200 - s.scale * 160}`).join(' ');
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center p-3 sm:p-6 bg-slate-950/80 backdrop-blur-md animate-fade-in">
      <div role="dialog" aria-modal="true" aria-label="CFAE System Framework Architecture Studio" className="relative w-full max-w-5xl h-[90vh] flex flex-col rounded-3xl bg-[#090D16] border border-slate-800 text-slate-100 shadow-2xl overflow-hidden">
        <header className="p-5 border-b border-slate-800 flex justify-between items-center gap-3">
          <div><h2 className="font-semibold text-lg text-white">CFAE System Framework Studio</h2><p className="text-xs text-sky-400 font-mono">v3.0.R · Mutation Safety & Coordinated Optimization</p></div>
          <button onClick={props.onClose} aria-label="Close Studio" className="p-2 rounded-xl hover:bg-slate-800 text-slate-400 hover:text-white transition-colors"><X size={20} /></button>
        </header>
        <nav className="flex flex-wrap gap-2 p-3 border-b border-slate-800" aria-label="Studio sections">
          {tabs.map(item => <button key={item} onClick={() => setTab(item)} className={`px-3 py-2 rounded-xl text-xs font-medium transition-colors ${item === tab ? 'bg-sky-500 text-slate-950 shadow-sm font-semibold' : 'bg-slate-900 text-slate-300 hover:bg-slate-800'}`}>{item}</button>)}
        </nav>
        <div className="flex-1 min-h-0 overflow-auto p-5 sm:p-7 space-y-5 text-sm">
          {tab === 'Architecture' && <>
            <h3 className="font-semibold flex gap-2"><Cpu size={18} /> Runtime architecture</h3>
            <p className="text-slate-300">Discover → validate telemetry → Rust decision → journaled C++ actuation → read-back verification → retain or restore.</p>
            <div className="grid sm:grid-cols-2 gap-3">
              {[['C++23 runtime', 'epoll, timerfd, signalfd, sysfs discovery, OEM ownership and durable rollback journal.'], ['Rust decision engine', 'Thermal PID, hysteresis, pressure priority, sample validation and bounded capacity budget.'], ['CPU / GPU control', 'Only exposed frequency tables and limits. OEM ceilings remain authoritative.'], ['Workbench', 'Real hardware identity in native mode. Simulations and source explorer are separate from live telemetry.']].map(([title, text]) => <div key={title} className="p-4 rounded-2xl bg-slate-900 border border-slate-800"><h4 className="text-sky-300 mb-2">{title}</h4><p className="text-xs text-slate-400">{text}</p></div>)}
            </div>
            <dl className="font-mono text-xs space-y-2 text-slate-400"><div>SoC: {props.socModel}</div><div>Kernel: {props.kernelVersion}</div><div>Online CPUs: {props.onlineCpus}</div><div>GPU: {props.gpuType}</div></dl>
          </>}
          {tab === 'Mutation Safety' && <>
            <h3 className="font-semibold text-base text-white">Coordinated Optimization & Mutation Safety (5 Layers)</h3>
            <p className="text-xs text-slate-400">Unit pengendalian utama adalah <strong className="text-sky-400">Mutation Transaction</strong>: satu tujuan optimasi, satu rencana perubahan dengan batas risiko yang dipahami, readback verify per write, dan rollback aman.</p>
            <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-3">
              {[
                ['1. Reasoning Engine', 'Menggabungkan observasi workload, PSI CPU/Mem/IO, status daya & thermal. Menghasilkan optimization intent terukur.'],
                ['2. Dependency Analyzer', 'Membangun dependency graph (Required-before, Requires, Coupled-with, Conflicts-with). Default relasi unknown = terisolasi.'],
                ['3. Mutation Planner', 'Menyusun mutation group, menetapkan baseline, nilai target, urutan dependensi, batas risk budget, dan titik abort.'],
                ['4. Safety Admission Gate', 'Memeriksa semantik, range, baseline valid, ketiadaan konflik ownership, dan kepatuhan kebijakan thermal OEM.'],
                ['5. Coordinator & Readback', 'Serialisasi transaksi yang berpotensi konflik, write-ahead durability, readback verify segera setelah write.'],
                ['6. Outcome Learning', 'Menyimpan bukti hasil: retain konfigurasi bermanfaat, defer kandidat yang berulang kali netral/gagal tanpa retry buta.']
              ].map(([t, d]) => (
                <div key={t} className="p-3.5 rounded-2xl bg-slate-900/90 border border-slate-800/80">
                  <h4 className="text-xs font-bold text-sky-300 mb-1">{t}</h4>
                  <p className="text-[11px] text-slate-400 leading-relaxed">{d}</p>
                </div>
              ))}
            </div>
            <div className="p-4 rounded-2xl bg-amber-950/20 border border-amber-900/40 text-xs space-y-2">
              <span className="font-semibold text-amber-300">12 Invarian Keselamatan Wajib:</span>
              <p className="text-slate-300 leading-relaxed">
                Tidak ada write tanpa baseline valid · Writable ≠ Safe · Readback verifikasi setiap write · Rollback tidak menimpa modifikasi eksternal (ownership guard) · Kebijakan thermal OEM tetap batas keselamatan tertinggi · Fail-safe default: tidak menulis.
              </p>
            </div>
          </>}
          {tab === 'HAL / Binder' && <>
            <h3 className="font-semibold">Platform integration status</h3>
            <table className="w-full text-xs text-left"><thead><tr><th className="p-3">Capability</th><th className="p-3">Implementation</th></tr></thead><tbody>
              {[['AIDL Power HAL discovery', 'Nonblocking libbinder_ndk lookup; capability only.'], ['Qualcomm / MediaTek interception', 'Unavailable: requires a registered, versioned OEM/AOSP proxy backend.'], ['Binder device access', 'Does not expose or reroute other processes’ transactions.'], ['NPU / APU offload', 'Unavailable: requires a compatible compute workload and runtime.']].map(([a,b]) => <tr key={a} className="border-t border-slate-800"><td className="p-3 text-sky-300">{a}</td><td className="p-3 text-slate-400">{b}</td></tr>)}
            </tbody></table>
            <p className="text-slate-400">OEM thermal services stay active. The daemon does not replace Power HAL services, patch Binder, or inject undocumented vendor transactions.</p>
          </>}
          {tab === 'Thermal simulator' && <>
            <h3 className="font-semibold flex gap-2"><Activity size={18} /> Illustrative thermal equilibrium</h3>
            <p className="text-xs text-amber-300">Synthetic first-order thermal plant. These curves are not phone measurements, a performance forecast, or an OEM benchmark.</p>
            <label className="block text-xs">Workload: {load}%<input aria-label="Simulated workload" className="block w-full mt-2" type="range" min="10" max="100" value={load} onChange={e => setLoad(Number(e.target.value))} /></label>
            <label className="block text-xs">Ambient: {ambient}°C<input aria-label="Simulated ambient temperature" className="block w-full mt-2" type="range" min="20" max="40" value={ambient} onChange={e => setAmbient(Number(e.target.value))} /></label>
            <svg viewBox="0 0 620 235" role="img" aria-label="Temperature and capacity scale over 120 simulated seconds" className="w-full bg-slate-900 rounded-2xl border border-slate-800">
              <line x1="10" y1="200" x2="610" y2="200" stroke="#475569" />
              <polyline points={temperaturePoints} fill="none" stroke="#fb923c" strokeWidth="2" transform="translate(10,0)" />
              <polyline points={capacityPoints} fill="none" stroke="#38bdf8" strokeWidth="2" transform="translate(10,0)" />
              <text x="12" y="222" fill="#94a3b8" fontSize="11">0 s</text><text x="565" y="222" fill="#94a3b8" fontSize="11">120 s</text>
            </svg>
            <p className="text-xs"><span className="text-orange-400">Orange: temperature (20–70°C)</span> · <span className="text-sky-400">Blue: capacity scale (0–100%)</span></p>
            <p className="font-mono text-xs">Final: {simulation[120].temperature.toFixed(1)}°C · {(simulation[120].scale * 100).toFixed(0)}% capacity budget</p>
            <p className="text-xs text-slate-400">Native decisions additionally use PSI, thermal hysteresis and emergency exit. Hardware frequency steps are quantized and can exceed 2%.</p>
          </>}
          {tab === 'Preservation' && <>
            <h3 className="font-semibold">Bounded resource controls</h3>
            <ul className="space-y-4 text-slate-300">
              <li>Battery: reduce a standard charging-current ceiling only while charging, with valid battery temperature. Never increase an OEM current limit.</li>
              <li>Storage: optional cgroup-v2 relative I/O weights; no claim of measured UFS wear reduction.</li>
              <li>Recovery: write-ahead journal records before/after values; external OEM changes retire ownership. Restoration is verified and best-effort.</li>
              <li>Unknown telemetry, safety markers or a failed transaction stop control. Component longevity, power savings and frame pacing require device measurements.</li>
            </ul>
          </>}
          {tab === 'C++ / Rust' && <>
            <h3 className="font-semibold flex gap-2"><Code2 size={18} /> Source used by this build</h3>
            <select aria-label="Source file" value={source} onChange={e => setSource(e.target.value)} className="bg-slate-800 rounded-xl p-2">{Object.keys(sources).map(name => <option key={name}>{name}</option>)}</select>
            <pre className="overflow-auto text-xs font-mono p-4 rounded-2xl bg-slate-950 border border-slate-800 whitespace-pre">{sources[source]}</pre>
          </>}
          {tab === 'Export' && <>
            <h3 className="font-semibold">Specification and engineering handoff</h3>
            <div className="flex gap-3 flex-wrap">
              <button className="flex items-center gap-2 bg-sky-500 text-slate-950 px-4 py-2 rounded-xl" onClick={() => void copy()}><Copy size={15} />{copied ? 'Copied' : 'Copy'}</button>
              {['md', 'txt'].map(ext => <button key={ext} className="flex items-center gap-2 border border-slate-700 px-4 py-2 rounded-xl" onClick={() => download(ext)}><Download size={15} />Download .{ext}</button>)}
            </div>
            {copyError && <p role="alert" className="text-amber-300">{copyError}</p>}
            <pre className="whitespace-pre-wrap font-mono text-xs text-slate-400 bg-slate-950 p-4 rounded-2xl">{fullText}</pre>
          </>}
        </div>
      </div>
    </div>
  );
};
