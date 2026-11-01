import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import {
  buildMirrorStatusSnapshot,
  ConscienceTraceRow,
  createInitialVirtualDaemonStore,
  DaemonProbe,
  execNativeKsuShell,
  hasNativeKernelSuBridge,
  ParsedEngineStatus,
  PipelineStage,
  PolicyFormValues,
  VirtualDaemonStore,
} from './bridge/daemonBridge';
import { appendNativeTelemetry, nativeTelemetryRecord, TelemetryRecord } from './bridge/diagnostics';
import { BaselineThermalView } from './components/BaselineThermalView';
import { ControlSidebar } from './components/ControlSidebar';
import { LogcatStreamView } from './components/LogcatStreamView';
import { PolicyConfigView } from './components/PolicyConfigView';
import { RuntimeConsoleView } from './components/RuntimeConsoleView';
import { SysfsInspectorView } from './components/SysfsInspectorView';
import { ActiveTab, TopNav } from './components/TopNav';
import {
  DEFAULT_CAPABILITIES_FOR_PRESET,
  HARDWARE_PROFILES,
  SCENARIO_PRESETS,
} from './engine/presets';
import {
  advanceBanditArms,
  advanceBaselineExperiment,
  buildRuntimeSample,
  computeSysfsNodes,
  computeThermalPrediction,
  createInitialBanditArms,
  decidePolicyAction,
  evaluateAdaptivePolicy,
  evaluateCoreFlowPillar,
  evaluateHybridBrain,
  formatStartupLogs,
  HISTORY_WINDOW_SIZE,
  selectNotificationEvent,
} from './engine/simulator';
import {
  BanditArmState,
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  CapabilityOverrides,
  Decision,
  DevicePresetId,
  EngineMode,
  LogcatEntry,
  NotificationEvent,
  RuntimeSample,
  RuntimeState,
  SysfsNodeEntry,
  Trend,
  WorkloadInputs,
} from './engine/types';

function formatClockTime(d = new Date()): string {
  return d.toTimeString().slice(0, 8);
}

function createInitialExperiment(stockGovernor: string): BaselineExperimentState {
  const alt = stockGovernor === 'schedutil' ? 'walt' : 'schedutil';
  return {
    active: false,
    candidateGovernor: alt,
    injectRegression: false,
    baselineSamples: [],
    mutatedSamples: [],
    baselineMeanScore: null,
    mutatedMeanScore: null,
    deltaPct: null,
    outcome: BaselineOutcome.Pending,
    action: BaselineAction.CollectingBaseline,
    summary: 'Ready to verify candidate CPUFreq governor against 4-sample factory baseline.',
  };
}

function getDefaultForm(): PolicyFormValues {
  const C = window.CoreFlowControl;
  if (C) {
    return C.formFromConfig(C.DEFAULTS);
  }
  return {
    mutation_mode: 'observe',
    runtime_refresh: true,
    interactivity_mode: 'observe',
    monitor_interval: '5',
    min_confidence: '0.60',
    launch_window_ms: '2500',
    jank_miss_rate: '0.05',
    boost_readahead_kb: '512',
    boost_uclamp_launch: '20',
    boost_uclamp_jank: '10',
    boost_gpu_steps: '1',
    boost_window_ms: '3000',
  };
}

function mapStateFromEngine(raw: string): RuntimeState {
  const u = (raw || '').toUpperCase();
  if (u.includes('THERMAL')) return RuntimeState.ThermalGuard;
  if (u.includes('PRESSURE')) return RuntimeState.Pressure;
  if (u.includes('WARM')) return RuntimeState.Warming;
  if (u.includes('ELEVATED')) return RuntimeState.Elevated;
  if (u.includes('IDLE')) return RuntimeState.Idle;
  return RuntimeState.Normal;
}

export const App: React.FC = () => {
  const [isNativeBridge] = useState<boolean>(() => hasNativeKernelSuBridge());
  const [activeTab, setActiveTab] = useState<ActiveTab>('console');
  const [isRunning, setIsRunning] = useState<boolean>(true);

  const [deviceId, setDeviceId] = useState<DevicePresetId>('kunzite_parrot');
  const [capabilities, setCapabilities] = useState<CapabilityOverrides>(
    () => DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot
  );
  const [activeScenarioId, setActiveScenarioId] = useState<string | null>('daily_balanced');
  const [inputs, setInputs] = useState<WorkloadInputs>(() => ({
    ...SCENARIO_PRESETS[0].inputs,
    monitorIntervalSec: 5,
  }));

  // Virtual C++ daemon store for interactive browser preview
  const [mirrorStore, setMirrorStore] = useState<VirtualDaemonStore>(() =>
    createInitialVirtualDaemonStore()
  );
  const mirrorStoreRef = useRef(mirrorStore);
  mirrorStoreRef.current = mirrorStore;

  // Policy config state synced with /data/adb/coreflow/config.ini via CoreFlowControl
  const [rawConfigText, setRawConfigText] = useState<string>(
    () => createInitialVirtualDaemonStore().configRaw
  );
  const [policyForm, setPolicyForm] = useState<PolicyFormValues>(() => getDefaultForm());

  const [savedPolicyForm, setSavedPolicyForm] = useState<PolicyFormValues>(() => getDefaultForm());

  // Native KernelSU live probe & status states (populated only when isNativeBridge is true)
  const [nativeProbe, setNativeProbe] = useState<DaemonProbe | null>(null);
  const [nativeParsedStatus, setNativeParsedStatus] = useState<ParsedEngineStatus | null>(null);
  const [nativeTraceRows, setNativeTraceRows] = useState<ConscienceTraceRow[]>([]);
  const [nativeLogLines, setNativeLogLines] = useState<string[]>([]);
  const [daemonBusy, setDaemonBusy] = useState<boolean>(false);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);

  const [nativeHistory, setNativeHistory] = useState<TelemetryRecord[]>([]);
  const [nativeConfigReady, setNativeConfigReady] = useState(!isNativeBridge);
  const [configReadError, setConfigReadError] = useState<string | null>(null);
  const [logsLoading, setLogsLoading] = useState(false);
  const [logsError, setLogsError] = useState<string | null>(null);
  const [logsRefreshedAt, setLogsRefreshedAt] = useState<number | null>(null);
  const [nativeSyncError, setNativeSyncError] = useState<string | null>(null);
  const logsInFlight = useRef<Promise<void> | null>(null);
  const snapshotInFlight = useRef<Promise<void> | null>(null);

  // Telemetry state
  const profile = HARDWARE_PROFILES[deviceId];
  const [history, setHistory] = useState<RuntimeSample[]>(() => {
    const initial = buildRuntimeSample(
      1,
      120,
      { ...SCENARIO_PRESETS[0].inputs, monitorIntervalSec: 5 },
      HARDWARE_PROFILES.kunzite_parrot,
      DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot,
      [],
      0
    );
    return [initial];
  });
  const [policyState, setPolicyState] = useState<RuntimeState>(RuntimeState.Normal);
  const [prevPolicyState, setPrevPolicyState] = useState<RuntimeState>(RuntimeState.Normal);
  const [lastNotification, setLastNotification] = useState<NotificationEvent>(
    NotificationEvent.None
  );
  const [banditArms, setBanditArms] = useState<BanditArmState[]>(() =>
    createInitialBanditArms()
  );
  const [experiment, setExperiment] = useState<BaselineExperimentState>(() =>
    createInitialExperiment(HARDWARE_PROFILES.kunzite_parrot.cpuPolicies[0].stockGovernor)
  );
  const [logs, setLogs] = useState<LogcatEntry[]>(() =>
    formatStartupLogs(
      HARDWARE_PROFILES.kunzite_parrot,
      DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot,
      formatClockTime()
    )
  );

  const logSeqRef = useRef<number>(10);
  const nextLogId = (prefix: string) => {
    logSeqRef.current += 1;
    return `${prefix}-${logSeqRef.current}-${Date.now()}`;
  };

  const currentSample = history[history.length - 1];
  const decision: Decision = useMemo(() => decidePolicyAction(policyState), [policyState]);
  const pillarMode: EngineMode = useMemo(
    () => evaluateCoreFlowPillar(inputs, currentSample, capabilities),
    [inputs, currentSample, capabilities]
  );

  const prediction = useMemo(
    () => computeThermalPrediction(currentSample, history, inputs.monitorIntervalSec),
    [currentSample, history, inputs.monitorIntervalSec]
  );

  const mutationArmed = useMemo(() => {
    if (isNativeBridge) {
      return Boolean(nativeParsedStatus?.data?.brain?.armed);
    }
    const C = window.CoreFlowControl;
    const cfg = C ? C.parseIni(mirrorStore.configRaw) : {};
    return (
      mirrorStore.running &&
      cfg.mutation_mode === 'adaptive' &&
      cfg.mutation_armed === 'true' &&
      !mirrorStore.safeMode &&
      !mirrorStore.disableMarker
    );
  }, [isNativeBridge, nativeParsedStatus, mirrorStore]);

  const hybridBrain = useMemo(
    () =>
      evaluateHybridBrain(
        profile,
        capabilities,
        inputs,
        currentSample,
        policyState,
        prediction,
        banditArms,
        mutationArmed
      ),
    [
      profile,
      capabilities,
      inputs,
      currentSample,
      policyState,
      prediction,
      banditArms,
      mutationArmed,
    ]
  );

  const mirrorSysfsNodes: SysfsNodeEntry[] = useMemo(
    () =>
      computeSysfsNodes(
        profile,
        capabilities,
        pillarMode,
        policyState,
        currentSample,
        experiment,
        mutationArmed,
        hybridBrain
      ),
    [
      profile,
      capabilities,
      pillarMode,
      policyState,
      currentSample,
      experiment,
      mutationArmed,
      hybridBrain,
    ]
  );

  const sysfsNodes: SysfsNodeEntry[] = useMemo(() => {
    if (isNativeBridge && nativeParsedStatus?.data) {
      const d = nativeParsedStatus.data;
      const liveRows: SysfsNodeEntry[] = [];
      for (const c of d.hands.cpuApplied) {
        liveRows.push({
          path: c.key.endsWith('/scaling_governor') ? c.key : c.key + '/scaling_governor',
          subsystem: 'CPU & WALT',
          baselineValue: 'stock',
          currentValue: c.value,
          status: 'MUTATED',
          rationale: d.brain.observing
            ? 'Owned verified governor; measuring the prior write'
            : d.hands.lastCpu === 'SKIPPED'
              ? 'Held verified governor; no new CPU write this cycle'
              : `Owned verified governor; latest controller outcome: ${d.hands.lastCpu || 'not supplied'}`,
        });
      }
      for (const r of d.hands.resourceApplied) {
        const sub = r.key.includes('/vm/')
          ? 'VM / Sysctl'
          : r.key.includes('/net/')
            ? 'Network'
            : r.key.includes('/queue/')
              ? 'Block I/O'
              : r.key.includes('kgsl') || r.key.includes('devfreq') || r.key.includes('mali')
                ? 'GPU & DSP'
                : 'CPU & WALT';
        liveRows.push({
          path: r.key,
          subsystem: sub,
          baselineValue: 'captured_baseline',
          currentValue: r.value,
          status: d.hands.stabilizingEpoch ? 'CLAMPED' : 'MUTATED',
          rationale: d.hands.stabilizingEpoch
            ? 'Held stabilizing value; new writes require a justified cooling plan'
            : d.brain.observing
              ? 'Owned verified resource; measuring the prior write'
              : d.hands.lastResource === 'SKIPPED'
                ? 'Held verified resource; no new resource write this cycle'
                : `Owned verified resource; latest controller outcome: ${d.hands.lastResource || 'not supplied'}`,
        });
      }
      return liveRows;
    }
    return isNativeBridge ? [] : mirrorSysfsNodes;
  }, [isNativeBridge, nativeParsedStatus, mirrorSysfsNodes]);

  // Pure synchronous derivation of Mirror snapshot via control_logic.js + engine_status.js
  const mirrorDerived = useMemo(() => {
    if (isNativeBridge) {
      return {
        probe: null as DaemonProbe | null,
        parsedStatus: null as ParsedEngineStatus | null,
        traceRows: [] as ConscienceTraceRow[],
      };
    }
    const C = window.CoreFlowControl;
    const S = window.CoreFlowStatus;
    if (!C || !S) {
      return {
        probe: null as DaemonProbe | null,
        parsedStatus: null as ParsedEngineStatus | null,
        traceRows: [] as ConscienceTraceRow[],
      };
    }

    const rawSnapshot = buildMirrorStatusSnapshot(
      mirrorStore,
      profile,
      currentSample,
      policyState,
      prevPolicyState,
      decision,
      pillarMode,
      mirrorSysfsNodes,
      experiment.baselineSamples.length,
      experiment.mutatedSamples.length
    );
    const parts = C.splitSnapshot(rawSnapshot);
    return {
      probe: C.parseProbe(parts.probe),
      parsedStatus: S.parse(parts.status),
      traceRows: C.parseTrace(mirrorStore.traceLines.join('\n'), 80),
    };
  }, [
    isNativeBridge,
    mirrorStore,
    profile,
    currentSample,
    policyState,
    prevPolicyState,
    decision,
    pillarMode,
    mirrorSysfsNodes,
    experiment.baselineSamples.length,
    experiment.mutatedSamples.length,
  ]);

  const probe = isNativeBridge ? nativeProbe : mirrorDerived.probe;
  const parsedStatus = isNativeBridge ? nativeParsedStatus : mirrorDerived.parsedStatus;
  const traceRows = isNativeBridge ? nativeTraceRows : mirrorDerived.traceRows;

  const currentDaemonMode = useMemo(() => {
    const status = parsedStatus?.data;
    if (!status) return 'unknown';
    if (!status.running || probe?.running === false) return 'stopped';
    return status.brain.mode === 'adaptive' && status.brain.armed && !status.brain.holdActive
      ? 'adaptive' : 'held';
  }, [parsedStatus, probe?.running]);

  const effectiveSleepSec = useMemo(() => {
    const base = Number(policyForm.monitor_interval) || inputs.monitorIntervalSec || 5;
    if (!inputs.screenOn || policyState === RuntimeState.Idle) {
      return Math.max(15, base * 3);
    }
    return base;
  }, [policyForm.monitor_interval, inputs.monitorIntervalSec, inputs.screenOn, policyState]);

  const statusSummary = useMemo(() => {
    if (!parsedStatus || !window.CoreFlowStatus) return null;
    return window.CoreFlowStatus.summary(parsedStatus);
  }, [parsedStatus]);

  const pipelineStages: PipelineStage[] = useMemo(() => {
    if (!parsedStatus?.ok || !parsedStatus.data || !window.CoreFlowStatus) return [];
    return window.CoreFlowStatus.pipeline(parsedStatus.data);
  }, [parsedStatus]);

  const refreshNativeLogs = useCallback(async () => {
    if (!isNativeBridge) return;
    if (logsInFlight.current) return logsInFlight.current;
    const C = window.CoreFlowControl;
    if (!C) throw new Error('WebUI control module is unavailable.');
    const work = (async () => {
      setLogsLoading(true); setLogsError(null);
      try {
        const [trace, log] = await Promise.all([
          execNativeKsuShell(C.readTraceCmd()), execNativeKsuShell(C.readLogCmd()),
        ]);
        const errors: string[] = [];
        if (trace.errno === 0) setNativeTraceRows(C.parseTrace(trace.stdout, 400));
        else errors.push(`Journal: ${trace.stderr || trace.stdout || `exit ${trace.errno}`}`);
        if (log.errno === 0) setNativeLogLines(log.stdout.split(/\r?\n/).filter(Boolean));
        else errors.push(`Logcat: ${log.stderr || log.stdout || `exit ${log.errno}`}`);
        if (errors.length) throw new Error(errors.join(' · '));
        setLogsRefreshedAt(Date.now());
      } catch (err) {
        setLogsError(err instanceof Error ? err.message : String(err));
        throw err;
      } finally { setLogsLoading(false); }
    })();
    logsInFlight.current = work;
    try { await work; } finally { logsInFlight.current = null; }
  }, [isNativeBridge]);

  // Native KernelSU polling & sync
  const syncFromNativeDaemon = useCallback(
    async (includeConfigAndLogs = false) => {
      const C = window.CoreFlowControl;
      const S = window.CoreFlowStatus;
      if (!isNativeBridge || !C || !S) return;
      while (snapshotInFlight.current) {
        await snapshotInFlight.current;
        if (!includeConfigAndLogs) return;
      }
      const work = (async () => {
        const snapRes = await execNativeKsuShell(C.snapshotCmd());
        if (snapRes.errno === 0 && snapRes.stdout) {
          const parts = C.splitSnapshot(snapRes.stdout);
          const nextProbe = C.parseProbe(parts.probe);
          const nextParsed = S.parse(parts.status);
          setNativeProbe(nextProbe);
          setNativeParsedStatus(nextParsed);
          setNativeSyncError(nextParsed.ok ? null : nextParsed.error || 'Daemon status is unavailable.');
          const record = nativeTelemetryRecord(nextParsed);
          if (record && !nextParsed.stale) setNativeHistory(prev => appendNativeTelemetry(prev, record));

          if (nextParsed.ok && nextParsed.data) {
            const d = nextParsed.data;
            const mappedState = mapStateFromEngine(d.brain.state);
            setPrevPolicyState(mapStateFromEngine(d.brain.previous));
            setPolicyState(mappedState);
          }
        } else {
          setNativeSyncError(snapRes.stderr || snapRes.stdout || `Status read failed (exit ${snapRes.errno}).`);
        }

        if (includeConfigAndLogs) {
          const cfgRes = await execNativeKsuShell(C.readConfigCmd());
          if (cfgRes.errno === 0 && cfgRes.stdout.trim()) {
            setNativeConfigReady(true); setConfigReadError(null);
            setRawConfigText(cfgRes.stdout);
            const parsedCfg = C.parseIni(cfgRes.stdout);
            const loadedForm = C.formFromConfig({ ...C.DEFAULTS, ...parsedCfg });
            setPolicyForm(loadedForm);
            setSavedPolicyForm(loadedForm);
          } else {
            setNativeConfigReady(false);
            setConfigReadError(cfgRes.stderr || 'Unable to read config.ini. Use Sync before saving settings.');
          }
          await refreshNativeLogs().catch(() => {});
        }
      })();
      snapshotInFlight.current = work;
      try { await work; } finally { snapshotInFlight.current = null; }
    },
    [isNativeBridge, refreshNativeLogs]
  );

  useEffect(() => {
    if (!isNativeBridge) return;
    syncFromNativeDaemon(true).catch(err => setNativeSyncError(String(err)));
    const id = window.setInterval(() => {
      syncFromNativeDaemon(false).catch(err => setNativeSyncError(String(err)));
    }, 3000);
    return () => window.clearInterval(id);
  }, [isNativeBridge, syncFromNativeDaemon]);

  useEffect(() => {
    if (!isNativeBridge || activeTab !== 'logcat') return;
    refreshNativeLogs().catch(() => {});
    const id = window.setInterval(() => refreshNativeLogs().catch(() => {}), 5000);
    return () => window.clearInterval(id);
  }, [isNativeBridge, activeTab, refreshNativeLogs]);

  // Step 1 deterministic tick in Mirror mode
  const stepEngineTick = useCallback(() => {
    if (isNativeBridge || !mirrorStoreRef.current.running) return;

    setHistory((prevHistory) => {
      const last = prevHistory[prevHistory.length - 1];
      const nextIndex = last.sampleIndex + 1;
      const nextUptime = last.uptimeSeconds + effectiveSleepSec;
      const jitter = Math.sin(nextIndex * 1.37);

      const nextSample = buildRuntimeSample(
        nextIndex,
        nextUptime,
        inputs,
        profile,
        capabilities,
        prevHistory,
        jitter
      );

      setPolicyState((prevState) => {
        const nextState = evaluateAdaptivePolicy(nextSample, prevState);
        setPrevPolicyState(prevState);

        const notif = selectNotificationEvent(nextSample, nextState, prevState);
        if (notif !== NotificationEvent.None) {
          setLastNotification(notif);
        }

        if (nextState !== prevState) {
          const ts = formatClockTime();
          const act = decidePolicyAction(nextState);
          setLogs((prevLogs) => [
            {
              id: nextLogId('state'),
              sampleIndex: nextIndex,
              timeFormatted: ts,
              level:
                nextState === RuntimeState.ThermalGuard || nextState === RuntimeState.Pressure
                  ? 'W'
                  : 'I',
              tag:
                nextState === RuntimeState.ThermalGuard
                  ? 'ThermalGuard'
                  : 'CoreFlowAutonomous',
              message: `State transition ${prevState} -> ${nextState} | decision=${act} temp=${(
                nextSample.thermalMillidegrees / 1000
              ).toFixed(1)}°C conf=${nextSample.confidence.toFixed(2)}`,
            },
            ...prevLogs.slice(0, 149),
          ]);

          setMirrorStore((s) => ({
            ...s,
            traceLines: [
              ...s.traceLines.slice(-79),
              JSON.stringify({
                timestamp: ts,
                event: 'policy_evaluated',
                scope: 'balance',
                result: nextState,
                reason: `Transitioned ${prevState} -> ${nextState} (confidence ${nextSample.confidence.toFixed(
                  2
                )})`,
                state: nextState,
                decision: act,
              }),
            ],
          }));
        }

        return nextState;
      });

      setExperiment((prevExp) => {
        if (!prevExp.active) return prevExp;
        const { next, logMessage } = advanceBaselineExperiment(prevExp, nextSample);
        if (logMessage) {
          const ts = formatClockTime();
          const isRollback = logMessage.includes('ROLLED_BACK');
          const isBeneficial = logMessage.includes('EFFICIENCY_BENEFICIAL');
          setLogs((prevLogs) => [
            {
              id: nextLogId('exp'),
              sampleIndex: nextIndex,
              timeFormatted: ts,
              level: isRollback ? 'W' : 'I',
              tag: 'CoreFlowConscience',
              message: logMessage,
            },
            ...prevLogs.slice(0, 149),
          ]);
          setMirrorStore((s) => ({
            ...s,
            verifiedCount: isBeneficial ? s.verifiedCount + 1 : s.verifiedCount,
            rolledBackCount: isRollback ? s.rolledBackCount + 1 : s.rolledBackCount,
            traceLines: [
              ...s.traceLines.slice(-79),
              JSON.stringify({
                timestamp: ts,
                event: isRollback
                  ? 'rollback_completed'
                  : isBeneficial
                    ? 'actuator_apply_completed'
                    : 'policy_evaluated',
                scope: 'cpu_freq_coarse',
                result: isRollback ? 'ROLLED_BACK' : 'VERIFIED',
                reason: logMessage,
                target: `${profile.cpuPolicies[0].policyPath}/scaling_governor`,
                requested: prevExp.candidateGovernor,
                baseline_value: profile.cpuPolicies[0].stockGovernor,
              }),
            ],
          }));
        }
        return next;
      });

      const nextPred = computeThermalPrediction(
        nextSample,
        prevHistory,
        inputs.monitorIntervalSec
      );
      setBanditArms((prevArms) => {
        const evalStep = evaluateHybridBrain(
          profile,
          capabilities,
          inputs,
          nextSample,
          evaluateAdaptivePolicy(nextSample, policyState),
          nextPred,
          prevArms,
          mutationArmed
        );
        return advanceBanditArms(prevArms, evalStep.selectedArm, nextSample, nextPred);
      });

      return [...prevHistory.slice(-(HISTORY_WINDOW_SIZE - 1)), nextSample];
    });
  }, [
    isNativeBridge,
    effectiveSleepSec,
    inputs,
    profile,
    capabilities,
    policyState,
    mutationArmed,
  ]);

  useEffect(() => {
    if (isNativeBridge || !isRunning || !mirrorStore.running) return;
    const id = window.setInterval(stepEngineTick, 2400);
    return () => window.clearInterval(id);
  }, [isNativeBridge, isRunning, mirrorStore.running, stepEngineTick]);

  // Daemon control handlers (Start / Stop / Restart / SIGUSR1 Scan)
  const handleDaemonAction = async (
    action: 'start' | 'stop' | 'restart' | 'rediscover'
  ) => {
    const C = window.CoreFlowControl;
    if (!C) return;

    setDaemonBusy(true);
    setStatusMessage(null);
    try {
      if (isNativeBridge) {
        const cmd =
          action === 'start'
            ? C.startCmd()
            : action === 'stop'
              ? C.stopCmd()
              : action === 'restart'
                ? C.restartCmd()
                : C.refreshCmd();
        const res = await execNativeKsuShell(cmd);
        const out = (res.stdout || res.stderr).trim();
        setStatusMessage(out || `Executed ${action} (exit ${res.errno})`);
        await syncFromNativeDaemon(true);
      } else {
        const ts = formatClockTime();
        if (action === 'stop') {
          setMirrorStore((s) => ({
            ...s,
            running: false,
            traceLines: [
              ...s.traceLines,
              JSON.stringify({
                timestamp: ts,
                event: 'shutdown_completed',
                scope: 'runtime',
                result: 'STOPPED',
                reason: 'SIGTERM received via WebUI control; baseline tunables restored.',
              }),
            ],
          }));
          setStatusMessage('ENGINE_STOPPED (baseline restored via MutationJournal)');
        } else if (action === 'start' || action === 'restart') {
          setMirrorStore((s) => ({
            ...s,
            running: true,
            pid: action === 'restart' ? s.pid + 17 : s.pid,
            configPending: false,
            traceLines: [
              ...s.traceLines,
              JSON.stringify({
                timestamp: ts,
                event: 'cycle_completed',
                scope: 'runtime',
                result: 'STARTED',
                reason: `service.sh launched coreflowd (${action.toUpperCase()}).`,
              }),
            ],
          }));
          setStatusMessage(
            action === 'restart'
              ? 'ENGINE_RESTART_OK (config.ini reloaded)'
              : 'ENGINE_START_OK'
          );
        } else if (action === 'rediscover') {
          setMirrorStore((s) => ({
            ...s,
            traceLines: [
              ...s.traceLines,
              JSON.stringify({
                timestamp: ts,
                event: 'discovery_refreshed',
                scope: 'observe',
                result: 'OK',
                reason: `SIGUSR1 re-probed ${profile.discoveredResources} kernel paths (${profile.mutationReadyResources} policy-ready).`,
              }),
            ],
          }));
          setStatusMessage('REDISCOVER_SIGNAL_SENT (SIGUSR1 capability refresh complete)');
        }
      }
    } finally {
      setDaemonBusy(false);
    }
  };

  // Config commit handler wired through CoreFlowControl.validate -> gate -> applyValues -> writeConfigCmd
  const handleCommitConfig = async (
    confirmedGate = false
  ): Promise<{
    needsConfirm?: 'resume' | 'arm' | 'observe';
    error?: string;
    saved?: boolean;
  }> => {
    if (isNativeBridge && !nativeConfigReady) return { error: configReadError || 'Read the current config using Sync before saving.' };
    const C = window.CoreFlowControl;
    if (!C) {
      return { error: 'CoreFlowControl contract module is not loaded.' };
    }

    const validation = C.validate(policyForm);
    if (!validation.ok) {
      const firstErr = Object.values(validation.errors)[0] || 'Invalid configuration value.';
      return { error: firstErr };
    }

    const gateRes = C.gate(validation.values, {
      currentMode: currentDaemonMode,
      safe: Boolean(probe?.safe),
      disable: Boolean(probe?.disable),
    });

    if (!gateRes.ok) {
      return { error: gateRes.reason || 'Blocked by SAFE_MODE safety gate.' };
    }

    if (gateRes.confirm && !confirmedGate) {
      return { needsConfirm: gateRes.confirm };
    }

    const updatedIni = C.applyValues(rawConfigText, validation.values);
    const writeShellCmd = C.writeConfigCmd(updatedIni, validation.values, {
      clearDisable: gateRes.clearDisable,
    });

    if (isNativeBridge) {
      const res = await execNativeKsuShell(writeShellCmd);
      if (res.errno !== 0 || !res.stdout.includes('CONFIG_COMMIT_OK')) {
        return {
          error: (res.stderr || res.stdout || 'Failed to write config.ini').trim(),
        };
      }
      setRawConfigText(updatedIni);
      setSavedPolicyForm(C.formFromConfig({ ...C.DEFAULTS, ...validation.values }));
      await syncFromNativeDaemon(true);
      return { saved: true };
    }

    const savedDraft = C.formFromConfig({ ...C.DEFAULTS, ...validation.values });
    setSavedPolicyForm(savedDraft);
    setPolicyForm(savedDraft);
    // Mirror mode: apply updated config.ini and clear/set DISABLE marker identically to writeConfigCmd()
    const ts = formatClockTime();
    setRawConfigText(updatedIni);
    setMirrorStore((s) => ({
      ...s,
      configRaw: updatedIni,
      configPending: true,
      disableMarker:
        validation.values.mutation_mode === 'observe'
          ? true
          : gateRes.clearDisable
            ? false
            : s.disableMarker,
      traceLines: [
        ...s.traceLines,
        JSON.stringify({
          timestamp: ts,
          event: 'scope_gate_checked',
          scope: 'conscience',
          result: validation.values.mutation_mode === 'adaptive' ? 'ARMED' : 'OBSERVE',
          reason: `WebUI committed config.ini (mutation_mode=${validation.values.mutation_mode}, mutation_armed=${validation.values.mutation_armed}).`,
        }),
      ],
    }));
    setInputs((prev) => ({
      ...prev,
      monitorIntervalSec: Number(validation.values.monitor_interval) || 5,
    }));
    return { saved: true };
  };

  const handleChangeDevice = (nextId: DevicePresetId) => {
    const nextProf = HARDWARE_PROFILES[nextId];
    const nextCaps = DEFAULT_CAPABILITIES_FOR_PRESET[nextId];
    setDeviceId(nextId);
    setCapabilities(nextCaps);
    setBanditArms(createInitialBanditArms());
    setExperiment(createInitialExperiment(nextProf.cpuPolicies[0].stockGovernor));
    const initial = buildRuntimeSample(1, 120, inputs, nextProf, nextCaps, [], 0);
    setHistory([initial]);
    setPolicyState(RuntimeState.Normal);
    setPrevPolicyState(RuntimeState.Normal);
    setLogs(formatStartupLogs(nextProf, nextCaps, formatClockTime()));
  };

  const handleApplyScenario = (scenarioId: string) => {
    const found = SCENARIO_PRESETS.find((s) => s.id === scenarioId);
    if (!found) return;
    setActiveScenarioId(scenarioId);
    setInputs((prev) => ({
      ...found.inputs,
      monitorIntervalSec: prev.monitorIntervalSec,
    }));
  };

  const handleResetEngine = () => {
    const freshStore = createInitialVirtualDaemonStore();
    setMirrorStore(freshStore);
    setRawConfigText(freshStore.configRaw);
    setPolicyForm(getDefaultForm());
    setBanditArms(createInitialBanditArms());
    const initial = buildRuntimeSample(1, 120, inputs, profile, capabilities, [], 0);
    setHistory([initial]);
    setPolicyState(RuntimeState.Normal);
    setPrevPolicyState(RuntimeState.Normal);
    setExperiment(createInitialExperiment(profile.cpuPolicies[0].stockGovernor));
    setLogs(formatStartupLogs(profile, capabilities, formatClockTime()));
    setStatusMessage(null);
  };

  const numericSpecs = window.CoreFlowControl?.NUMERIC ?? [];

  return (
    <div id="top" className="min-h-screen flex flex-col bg-slate-50 text-slate-900">
      <TopNav
        activeTab={activeTab}
        onSelectTab={setActiveTab}
        isNativeBridge={isNativeBridge}
        isRunning={isRunning && (mirrorStore.running || isNativeBridge)}
        onToggleRunning={() => setIsRunning((r) => !r)}
        onRefreshNative={() => syncFromNativeDaemon(true)}
      />

      <div className="flex-1 flex flex-col lg:flex-row">
        <ControlSidebar
          isNativeBridge={isNativeBridge}
          probe={probe}
          daemonMode={currentDaemonMode}
          daemonBusy={daemonBusy}
          statusMessage={statusMessage}
          onDaemonAction={handleDaemonAction}
          mirrorSafeMode={mirrorStore.safeMode}
          onToggleMirrorSafeMode={(val) =>
            setMirrorStore((s) => ({ ...s, safeMode: val }))
          }
          deviceId={deviceId}
          onChangeDevice={handleChangeDevice}
          capabilities={capabilities}
          onChangeCapabilities={setCapabilities}
          inputs={inputs}
          onChangeInputs={(next) => {
            setActiveScenarioId(null);
            setInputs(next);
          }}
          activeScenarioId={activeScenarioId}
          onApplyScenario={handleApplyScenario}
          onStepTick={stepEngineTick}
          onResetEngine={handleResetEngine}
        />

        <main key={activeTab} className="cf-view-stage flex-1 min-w-0 p-4 sm:p-6 lg:p-8 max-w-7xl">
          {isNativeBridge && nativeSyncError && <div role="alert" className="mb-4 p-3 bg-red-50 border border-red-200 rounded-xl text-red-800 text-xs break-words">{nativeSyncError} · Displayed status may be stale; use Sync to retry.</div>}
          {isNativeBridge && !nativeParsedStatus?.ok && ['console', 'sysfs', 'baseline'].includes(activeTab) && <div role="status" className="p-5 bg-white border border-slate-200 rounded-xl text-sm">Waiting for live daemon data. Use Sync to retry.</div>}
          {activeTab === 'console' && (!isNativeBridge || nativeParsedStatus?.ok) && (
            <RuntimeConsoleView
              isNativeBridge={isNativeBridge}
              nativeHistory={nativeHistory}
              profile={profile}
              capabilities={capabilities}
              currentSample={currentSample}
              history={history}
              policyState={policyState}
              decision={decision}
              pillarMode={pillarMode}
              hybridBrain={hybridBrain}
              lastNotification={lastNotification}
              effectiveSleepSec={effectiveSleepSec}
              sysfsNodes={sysfsNodes}
              parsedStatus={parsedStatus}
              statusSummary={statusSummary}
              pipelineStages={pipelineStages}
              onOpenSysfs={() => setActiveTab('sysfs')}
              onOpenPolicy={() => setActiveTab('policy')}
            />
          )}

          {activeTab === 'sysfs' && (!isNativeBridge || nativeParsedStatus?.ok) && (
            <SysfsInspectorView nodes={sysfsNodes} parsedStatus={parsedStatus} />
          )}

          {activeTab === 'policy' && configReadError && <div role="alert" className="p-3 mb-3 text-xs bg-red-50 text-red-800">{configReadError}</div>}
          {activeTab === 'policy' && (
            <PolicyConfigView
              form={policyForm}
              savedForm={savedPolicyForm}
              numericSpecs={numericSpecs}
              probe={probe}
              currentDaemonMode={currentDaemonMode}
              onUpdateForm={setPolicyForm}
              onCommitConfig={handleCommitConfig}
              onRestartDaemon={() => handleDaemonAction('restart')}
            />
          )}

          {activeTab === 'baseline' && (!isNativeBridge || nativeParsedStatus?.ok) && (
            <BaselineThermalView
              isNativeBridge={isNativeBridge}
              profile={profile}
              experiment={experiment}
              prediction={prediction}
              hybridBrain={hybridBrain}
              parsedStatus={parsedStatus}
              onSelectGovernor={(gov) =>
                setExperiment((prev) => ({ ...prev, candidateGovernor: gov }))
              }
              onToggleInjectRegression={(val) =>
                setExperiment((prev) => ({ ...prev, injectRegression: val }))
              }
              onStartExperiment={() =>
                setExperiment((prev) => ({
                  ...createInitialExperiment(profile.cpuPolicies[0].stockGovernor),
                  candidateGovernor: prev.candidateGovernor,
                  injectRegression: prev.injectRegression,
                  active: true,
                  action: BaselineAction.CollectingBaseline,
                  summary: 'Capturing pre-mutation baseline (0/4 samples)...',
                }))
              }
              onResetExperiment={() =>
                setExperiment(createInitialExperiment(profile.cpuPolicies[0].stockGovernor))
              }
            />
          )}

          {activeTab === 'logcat' && (
            <LogcatStreamView
              entries={logs}
              traceRows={traceRows}
              nativeLogLines={nativeLogLines}
              isNativeBridge={isNativeBridge}
              history={history}
              nativeHistory={nativeHistory}
              loading={logsLoading}
              readError={logsError}
              refreshedAt={logsRefreshedAt}
              onRefreshLogs={refreshNativeLogs}
              onClearLogs={() => setLogs([])}
            />
          )}
        </main>
      </div>
    </div>
  );
};

export default App;
