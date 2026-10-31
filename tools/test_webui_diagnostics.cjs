#!/usr/bin/env node
// Exercise the actual TS bridge and React action handlers against sandboxed shell commands.
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const { spawnSync } = require('node:child_process');
const { createRequire } = require('node:module');
const root = path.resolve(__dirname, '..');
const req = createRequire(path.join(root, 'package.json'));
const ts = req('typescript');
let hooks = [], hookIndex = 0;
const react = { ...req('react'), useMemo: fn => fn(), useCallback: fn => fn, useEffect: () => {}, useRef: initial => {
  const index = hookIndex++;
  if (!(index in hooks)) hooks[index] = { current: initial };
  return hooks[index];
}, useState: initial => {
  const index = hookIndex++;
  if (!(index in hooks)) hooks[index] = typeof initial === 'function' ? initial() : initial;
  return [hooks[index], next => { hooks[index] = typeof next === 'function' ? next(hooks[index]) : next; }];
} };
const cache = new Map();
function load(file) {
  if (cache.has(file)) return cache.get(file).exports;
  const mod = { exports: {} }; cache.set(file, mod);
  const code = ts.transpileModule(fs.readFileSync(file, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2022, jsx: ts.JsxEmit.ReactJSX }
  }).outputText;
  const localRequire = name => {
    if (name === 'react') return react;
    if (name === 'lucide-react') return new Proxy({}, { get: () => () => null });
    if (!name.startsWith('.')) return req(name);
    const resolved = path.resolve(path.dirname(file), name);
    const candidate = [resolved, resolved + '.ts', resolved + '.tsx', resolved + '.js'].find(p => fs.existsSync(p));
    return load(candidate);
  };
  new Function('require', 'module', 'exports', code)(localRequire, mod, mod.exports);
  return mod.exports;
}
function loadUmd(file) {
  const mod = { exports: {} };
  vm.runInNewContext(fs.readFileSync(file, 'utf8'), { module: mod, exports: mod.exports });
  return mod.exports;
}
const C = loadUmd(path.join(root, 'webui/public/control_logic.js'));
const D = load(path.join(root, 'webui/bridge/diagnostics.ts'));
const B = load(path.join(root, 'webui/bridge/daemonBridge.ts'));
const { LogcatStreamView } = load(path.join(root, 'webui/components/LogcatStreamView.tsx'));
let checks = 0;
function check(condition, label) { assert.ok(condition, label); checks++; }
const sandbox = fs.mkdtempSync(path.join(os.tmpdir(), 'coreflow-diagnostics-'));
const state = path.join(sandbox, 'state'), downloads = path.join(sandbox, 'Download');
let failWrites = false, failLogcat = false, errnoOverride, commands = [];
const bin = path.join(sandbox, 'bin');
fs.mkdirSync(bin);
fs.writeFileSync(path.join(bin, 'logcat'), `#!/bin/sh
if [ "$CF_FAIL_LOGCAT" = 1 ]; then echo "logcat permission denied" >&2; exit 1; fi
printf '%s\\n' 'LIVE_DAEMON_LOG'
`, { mode: 0o755 });
global.window = {
  CoreFlowControl: C,
  CoreFlowStatus: loadUmd(path.join(root, 'webui/public/engine_status.js')),
  setTimeout, clearTimeout,
  ksu: { exec(cmd, _opts, callback) {
    commands.push(cmd);
    const mapped = cmd.replaceAll('/data/adb/coreflow', state).replaceAll('/sdcard/Download', downloads);
    const result = failWrites && cmd.includes('base64 -d')
      ? { status: 1, stdout: '', stderr: 'No space left on device' }
      : spawnSync('sh', ['-c', mapped], { encoding: 'utf8', timeout: 5000, env: { ...process.env, PATH: bin + ':' + process.env.PATH, CF_FAIL_LOGCAT: failLogcat ? '1' : '0' } });
    // Return device paths as the real bridge would, including string errno variants.
    const stdout = String(result.stdout || '').replaceAll(state, '/data/adb/coreflow').replaceAll(downloads, '/sdcard/Download');
    queueMicrotask(() => window[callback](errnoOverride === undefined ? String(result.status ?? 1) : errnoOverride, stdout, result.stderr || ''));
  } },
};
function text(node) {
  if (Array.isArray(node)) return node.map(text).join('');
  if (node == null || typeof node === 'boolean') return '';
  if (typeof node === 'string' || typeof node === 'number') return String(node);
  return text(node.props?.children);
}
function all(node, predicate, out = []) {
  if (Array.isArray(node)) { node.forEach(n => all(n, predicate, out)); return out; }
  if (!node || typeof node !== 'object') return out;
  if (predicate(node)) out.push(node);
  all(node.props?.children, predicate, out);
  return out;
}
const rows = C.parseTrace([
  { event: 'mutation_journal_commit', result: 'FAILED', reason: 'journal_commit_failed_no_actuator_write', scope: 'resource' },
  { event: 'scope_gate_checked', result: 'DENIED', reason: 'blocked', scope: 'resource' },
  { event: 'actuator_apply_completed', result: 'OK', writes_verified: 1, target: '/sys/test', reason: 'verified' },
  { event: 'rollback_completed', result: 'ROLLED_BACK', reason: 'regression' },
  { event: 'actuator_apply_started', result: 'STARTED', reason: 'not yet verified' },
].map(JSON.stringify).join('\n'));
const props = { entries: [{ id: 'fake', message: 'SIMULATED_ONLY', level: 'I', tag: 'Mirror', sampleIndex: 1, timeFormatted: 't' }], traceRows: rows, nativeLogLines: ['line one', 'line two'], isNativeBridge: true, nativeHistory: [{ Sample: 9, UpdatedEpoch: 10, PID: 1, 'CPU(%)': null }], onRefreshLogs: async () => {}, onClearLogs: () => {} };
function render(next = props) { hookIndex = 0; return LogcatStreamView(next); }
function button(tree, label) {
  const found = all(tree, n => n.type === 'button' && text(n).includes(label));
  assert.equal(found.length, 1, label); return found[0];
}
(async () => {
  try {
    for (const name of ['control_logic.js', 'engine_status.js']) check(fs.readFileSync(path.join(root, 'webui/public', name), 'utf8') === fs.readFileSync(path.join(root, 'module/webroot', name), 'utf8'), `${name} is synchronized with the packaged helper`);
    for (const kind of ['block', 'change', 'safety', 'error']) check(C.filterTrace(rows, kind).length === 1, `exact ${kind} filter`);
    check(C.filterTrace(rows, 'change')[0].event === 'actuator_apply_completed', 'only verified writes classified as changes');
    const longReason = 'x'.repeat(900);
    check(C.parseTrace(JSON.stringify({ reason: longReason }))[0].reason === longReason, 'audit rationale is not cut to 160 characters');
    fs.mkdirSync(state);
    const tracePath = path.join(state, 'decision_trace.jsonl');
    const rawTrace = JSON.stringify({ event: 'cycle_completed', result: 'OK', reason: 'two words and \"quotes\"' });
    fs.writeFileSync(tracePath, rawTrace + '\n');
    const traceRead = await B.execNativeKsuShell(C.readTraceCmd());
    check(traceRead.errno === 0 && traceRead.stdout.trim() === rawTrace, 'native trace read preserves JSON whitespace and quoting');
    fs.rmSync(tracePath); fs.mkdirSync(tracePath);
    check((await B.execNativeKsuShell(C.readTraceCmd())).errno !== 0, 'failed journal read is not hidden by a successful tail pipeline');
    fs.rmSync(tracePath, { recursive: true });
    const logRead = await B.execNativeKsuShell(C.readLogCmd());
    check(logRead.errno === 0 && logRead.stdout.includes('\n=== Android daemon logcat ===\nLIVE_DAEMON_LOG'), 'actual tagged logcat is read with separate source headings');
    failLogcat = true;
    check((await B.execNativeKsuShell(C.readLogCmd())).errno !== 0, 'logcat errors propagate through the output bound');
    failLogcat = false;
    const failedSafety = C.parseTrace(JSON.stringify({ event: 'rollback_completed', result: 'FAILED', writes_verified: 1 }));
    check(C.filterTrace(failedSafety, 'safety').length === 1 && C.filterTrace(failedSafety, 'error').length === 1, 'failed recovery remains visible in both safety and error filters');
    check(C.filterTrace(failedSafety, 'change').length === 1, 'verified partial writes stay visible even if the overall operation failed');
    const payload = 'α日本語\n$HOME $(touch SHOULD_NOT_EXIST) `id`\n'.repeat(1800);
    const saved = await D.saveDiagnosticFile('roundtrip.txt', payload, 'text/plain');
    check(saved.includes('/sdcard/Download/roundtrip.txt'), 'native export reports destination');
    check(fs.readFileSync(path.join(downloads, 'roundtrip.txt'), 'utf8') === payload, 'multichunk Unicode export roundtrips byte-for-byte');
    check(!fs.existsSync(path.join(root, 'SHOULD_NOT_EXIST')), 'diagnostic content cannot execute shell');
    check(!fs.readdirSync(state).length, 'private staging files are cleaned');
    await assert.rejects(() => D.saveDiagnosticFile('../escape.txt', 'x', 'text/plain')); checks++;
    failWrites = true;
    await assert.rejects(() => D.saveDiagnosticFile('failed.txt', payload, 'text/plain'), /No space/); checks++;
    check(!fs.existsSync(path.join(downloads, 'failed.txt')) && !fs.readdirSync(state).length, 'failed staging cannot produce a successful export');
    failWrites = false;
    await assert.rejects(() => D.saveDiagnosticFile('roundtrip.txt', 'replacement', 'text/plain')); checks++;
    check(fs.readFileSync(path.join(downloads, 'roundtrip.txt'), 'utf8') === payload, 'existing downloads are never overwritten');
    errnoOverride = null;
    check((await B.execNativeKsuShell('true')).errno !== 0, 'malformed errno fails closed');
    errnoOverride = undefined;
    const csv = D.nativeHistoryToCsv([{ Sample: 2, CPU: null, Note: '=1+1', Surface: 'a,"b"\nc' }]);
    check(csv.includes("2,,'=1+1,\"a,\"\"b\"\"\nc\""), 'CSV keeps unknown values blank, escapes text and neutralizes formulas');
    let history = [];
    for (let i = 0; i < 450; i++) history = D.appendNativeTelemetry(history, { PID: 1, Sample: i });
    check(history.length === 300 && history[0].Sample === 150, 'native session history is bounded');
    history = D.appendNativeTelemetry(history, { PID: 1, Sample: 449 });
    check(history.length === 300, 'repeated polls do not duplicate samples');
    check(D.appendNativeTelemetry(history, { PID: 2, Sample: 1 }).length === 1, 'daemon restart separates history');
    check(D.nativeTelemetryRecord({ ok: false }) === null, 'invalid snapshots cannot become CSV measurements');
    let tree = render();
    await button(tree, 'Reload').props.onClick();
    check(text(render()).includes('Journal and logcat refreshed.'), 'Reload displays completion feedback');
    await button(render(), 'Export Logs').props.onClick();
    check(text(render()).includes('Saved: /sdcard/Download/coreflow-decision-trace-'), 'trace export handler uses native storage');
    await button(render(), 'Download History CSV').props.onClick();
    const csvFile = fs.readdirSync(downloads).find(n => n.endsWith('.csv'));
    check(csvFile && fs.readFileSync(path.join(downloads, csvFile), 'utf8').includes('9,10,1,'), 'CSV button saves measured session data');
    hooks = ['logcat', 'all', 'two'];
    tree = render();
    await button(tree, 'Export Logs').props.onClick();
    const logFile = fs.readdirSync(downloads).find(n => n.includes('daemon-logcat'));
    check(fs.readFileSync(path.join(downloads, logFile), 'utf8') === 'line two', 'log export respects search filter');
    hooks = ['logcat', 'all', ''];
    check(!text(render({ ...props, nativeLogLines: [] })).includes('SIMULATED_ONLY'), 'missing native logs never expose preview logs');
    hooks = [];
    await button(render({ ...props, onRefreshLogs: async () => { throw new Error('permission denied'); } }), 'Reload').props.onClick();
    check(text(render()).includes('permission denied'), 'Reload exposes read failures');
    const { App } = load(path.join(root, 'webui/App.tsx'));
    hooks = [];
    const renderApp = () => { hookIndex = 0; return App(); };
    const component = (tree, name) => all(tree, n => n.type?.name === name)[0];
    const now = Math.floor(Date.now() / 1000);
    fs.writeFileSync(path.join(state, 'status.json'), JSON.stringify({ schema: 1, version: '3.0.2', pid: 1234, sample: 9, updated_epoch: now, interval_s: 5, running: true, blocker: 'READY', eyes: { thermal_available: false, cpu_utilization: null, battery_percent: null }, brain: { mode: 'adaptive', armed: true }, hands: {} }));
    fs.writeFileSync(path.join(state, 'config.ini'), 'monitor_interval=5\nmin_confidence=0.60\nmutation_mode=adaptive\nmutation_armed=true\n');
    await component(renderApp(), 'TopNav').props.onRefreshNative();
    let appTree = renderApp();
    check(component(appTree, 'RuntimeConsoleView').props.isNativeBridge === true, 'live console is explicitly scoped to native data');
    const consoleNode = component(appTree, 'RuntimeConsoleView');
    const runtimeView = consoleNode.type({ ...consoleNode.props, nativeHistory: [
      { UpdatedEpoch: now - 10, 'CPU(%)': 10, 'Thermal(C)': 35 },
      { UpdatedEpoch: now - 5, 'CPU(%)': null, 'Thermal(C)': null },
      { UpdatedEpoch: now, 'CPU(%)': 20, 'Thermal(C)': 36 },
    ] });
    const cpuPath = all(runtimeView, n => n.type === 'path' && n.props.stroke === '#2563EB')[0].props.d;
    check((cpuPath.match(/M/g) || []).length === 2 && !cpuPath.includes('NaN'), 'native chart breaks the line across unavailable readings');
    check(!text(runtimeView).includes('43.0') && !text(runtimeView).includes('Stock Governor'), 'live view excludes illustrative thresholds and preview hardware');
    component(appTree, 'TopNav').props.onSelectTab('policy');
    appTree = renderApp();
    const policy = component(appTree, 'PolicyConfigView');
    policy.props.onUpdateForm({ ...policy.props.form, monitor_interval: '11' });
    component(renderApp(), 'TopNav').props.onSelectTab('logcat');
    let journalProps = component(renderApp(), 'LogcatStreamView').props;
    check(journalProps.nativeHistory.length === 1 && journalProps.nativeHistory[0]['CPU(%)'] === null, 'native history starts with real data and preserves unavailable sensors');
    await journalProps.onRefreshLogs();
    component(renderApp(), 'TopNav').props.onSelectTab('policy');
    check(component(renderApp(), 'PolicyConfigView').props.form.monitor_interval === '11', 'journal Reload cannot overwrite unsaved engine settings');
    await component(renderApp(), 'TopNav').props.onRefreshNative();
    component(renderApp(), 'TopNav').props.onSelectTab('logcat');
    check(component(renderApp(), 'LogcatStreamView').props.nativeHistory.length === 1, 'repeated Sync does not duplicate a daemon sample');
    component(renderApp(), 'TopNav').props.onSelectTab('sysfs');
    check(component(renderApp(), 'SysfsInspectorView').props.nodes.length === 0, 'no native changes cannot fall back to simulated sysfs mutations');
    fs.rmSync(path.join(state, 'config.ini'));
    await component(renderApp(), 'TopNav').props.onRefreshNative();
    component(renderApp(), 'TopNav').props.onSelectTab('policy');
    const blockedSave = await component(renderApp(), 'PolicyConfigView').props.onCommitConfig();
    check(blockedSave.error?.includes('config.ini'), 'failed config reads prevent saving invented defaults');
    console.log(`CoreFlow WebUI diagnostics: ${checks}/${checks} checks passed`);
  } finally {
    fs.rmSync(sandbox, { recursive: true, force: true });
  }
})().catch(err => { console.error(err); process.exitCode = 1; });
