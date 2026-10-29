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
  advanceBaselineExperiment,
  buildRuntimeSample,
  computeSysfsNodes,
  computeThermalPrediction,
  decidePolicyAction,
  evaluateAdaptivePolicy,
  evaluateCoreFlowPillar,
  formatStartupLogs,
  HISTORY_WINDOW_SIZE,
  selectNotificationEvent,
} from './engine/simulator';
import {
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
  ThermalSource,
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

function mapTrendFromEngine(raw: string): Trend {
  const u = (raw || '').toUpperCase();
  if (u.includes('RISING')) return Trend.Rising;
  if (u.includes('FALLING')) return Trend.Falling;
  if (u.includes('STABLE')) return Trend.Stable;
  return Trend.Unknown;
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

  // Native KernelSU live probe & status states (populated only when isNativeBridge is true)
  const [nativeProbe, setNativeProbe] = useState<DaemonProbe | null>(null);
  const [nativeParsedStatus, setNativeParsedStatus] = useState<ParsedEngineStatus | null>(null);
  const [nativeTraceRows, setNativeTraceRows] = useState<ConscienceTraceRow[]>([]);
  const [nativeLogLines, setNativeLogLines] = useState<string[]>([]);
  const [daemonBusy, setDaemonBusy] = useState<boolean>(false);
  const [statusMessage, setStatusMessage] = useState<string | null>(null);

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

  const mirrorSysfsNodes: SysfsNodeEntry[] = useMemo(
    () =>
      computeSysfsNodes(
        profile,
        capabilities,
        pillarMode,
        policyState,
        currentSample,
        experiment,
        mutationArmed
      ),
    [profile, capabilities, pillarMode, policyState, currentSample, experiment, mutationArmed]
  );

  const sysfsNodes: SysfsNodeEntry[] = useMemo(() => {
    if (isNativeBridge && nativeParsedStatus?.data) {
      const d = nativeParsedStatus.data;
      const liveRows: SysfsNodeEntry[] = [];
      for (const c of d.hands.cpuApplied) {
        liveRows.push({
          path: c.key,
          subsystem: 'CPU & WALT',
          baselineValue: 'stock',
          currentValue: c.value,
          status: 'MUTATED',
          rationale: `Verified CPUFreq write by coreflowd (${d.hands.lastCpu || 'VERIFIED'})`,
        });
      }
      for (const r of d.hands.resourceApplied) {
        const sub = r.key.includes('/vm/')
          ? 'VM / Sysctl'
          : r.key.includes('/queue/')
            ? 'Block I/O'
            : r.key.includes('kgsl') || r.key.includes('devfreq')
              ? 'GPU & DSP'
              : 'CPU & WALT';
        liveRows.push({
          path: r.key,
          subsystem: sub,
          baselineValue: 'captured_baseline',
          currentValue: r.value,
          status: d.hands.stabilizingEpoch ? 'CLAMPED' : 'MUTATED',
          rationale: d.hands.stabilizingEpoch
            ? 'Stabilizing epoch clamp enforced by PolicyEngine'
            : `Verified resource tunable (${d.hands.lastResource || 'VERIFIED'})`,
        });
      }
      if (liveRows.length > 0) {
        return liveRows;
      }
    }
    return mirrorSysfsNodes;
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

  const prediction = useMemo(
    () => computeThermalPrediction(currentSample, history, inputs.monitorIntervalSec),
    [currentSample, history, inputs.monitorIntervalSec]
  );

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

  // Native KernelSU polling & sync
  const syncFromNativeDaemon = useCallback(
    async (includeConfigAndLogs = false) => {
      const C = window.CoreFlowControl;
      const S = window.CoreFlowStatus;
      if (!isNativeBridge || !C || !S) return;

      const snapRes = await execNativeKsuShell(C.snapshotCmd());
      if (snapRes.errno === 0 && snapRes.stdout) {
        const parts = C.splitSnapshot(snapRes.stdout);
        const nextProbe = C.parseProbe(parts.probe);
        const nextParsed = S.parse(parts.status);
        setNativeProbe(nextProbe);
        setNativeParsedStatus(nextParsed);

        if (nextParsed.ok && nextParsed.data) {
          const d = nextParsed.data;
          const mappedState = mapStateFromEngine(d.brain.state);
          setPrevPolicyState(mapStateFromEngine(d.brain.previous));
          setPolicyState(mappedState);

          setHistory((prev) => {
            const last = prev[prev.length - 1];
            const memPct = d.eyes.mem !== null ? d.eyes.mem * 100 : 50;
            const totalKb = 8_388_608;
            const nextSample: RuntimeSample = {
              sampleIndex: d.sample || last.sampleIndex + 1,
              uptimeSeconds: last.uptimeSeconds + (d.interval || 5),
              cpuUsagePct: d.eyes.cpu !== null ? Number((d.eyes.cpu * 100).toFixed(1)) : 0,
              gpuBusyPct: 0,
              load1: d.eyes.load ?? 0,
              memTotalKb: totalKb,
              memAvailableKb: Math.round((totalKb * memPct) / 100),
              memAvailableRatio: d.eyes.mem ?? 0.5,
              thermalAvailable: d.eyes.thermal !== null,
              thermalMillidegrees:
                d.eyes.thermal !== null ? Math.round(d.eyes.thermal * 1000) : 0,
              hottestThermalMillidegrees:
                d.eyes.hottest !== null
                  ? Math.round(d.eyes.hottest * 1000)
                  : d.eyes.thermal !== null
                    ? Math.round(d.eyes.thermal * 1000)
                    : 0,
              thermalSource: d.eyes.thermal !== null ? ThermalSource.Primary : ThermalSource.Unknown,
              charging: d.eyes.charging,
              chargingTelemetryAvailable: d.eyes.battery !== null,
              batteryPercent: d.eyes.battery ?? 0,
              batteryTempMillidegrees:
                d.eyes.thermal !== null ? Math.round((d.eyes.thermal - 2.0) * 1000) : 32_000,
              batteryCurrentMicroamps: d.eyes.charging ? 2_000_000 : -400_000,
              batteryVoltageMicrovolts: 3_950_000,
              ioReadKbs: d.eyes.ioRead ?? 0,
              ioWriteKbs: d.eyes.ioWrite ?? 0,
              thermalTrend: mapTrendFromEngine(d.eyes.thermalTrend),
              memoryTrend: mapTrendFromEngine(d.eyes.memTrend),
              loadTrend: mapTrendFromEngine(d.eyes.loadTrend),
              confidence: d.eyes.confidence ?? 0,
            };
            if (last.sampleIndex === nextSample.sampleIndex && prev.length > 1) {
              return [...prev.slice(0, -1), nextSample];
            }
            return [...prev.slice(-(HISTORY_WINDOW_SIZE - 1)), nextSample];
          });
        }
      }

      if (includeConfigAndLogs) {
        const [cfgRes, traceRes, logRes] = await Promise.all([
          execNativeKsuShell(C.readConfigCmd()),
          execNativeKsuShell(C.readTraceCmd()),
          execNativeKsuShell(C.readLogCmd()),
        ]);
        if (cfgRes.errno === 0) {
          setRawConfigText(cfgRes.stdout);
          const parsedCfg = C.parseIni(cfgRes.stdout);
          setPolicyForm(C.formFromConfig({ ...C.DEFAULTS, ...parsedCfg }));
        }
        if (traceRes.errno === 0) {
          setNativeTraceRows(C.parseTrace(traceRes.stdout, 120));
        }
        if (logRes.errno === 0) {
          setNativeLogLines(
            logRes.stdout
              .split(/\r?\n/)
              .map((s) => s.trim())
              .filter(Boolean)
          );
        }
      }
    },
    [isNativeBridge]
  );

  useEffect(() => {
    if (!isNativeBridge) return;
    syncFromNativeDaemon(true);
    const id = window.setInterval(() => {
      syncFromNativeDaemon(false);
    }, 3000);
    return () => window.clearInterval(id);
  }, [isNativeBridge, syncFromNativeDaemon]);

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
                target: '/sys/devices/system/cpu/cpufreq/policy0/scaling_governor',
                requested: prevExp.candidateGovernor,
                baseline_value: profile.cpuPolicies[0].stockGovernor,
              }),
            ],
          }));
        }
        return next;
      });

      return [...prevHistory.slice(-(HISTORY_WINDOW_SIZE - 1)), nextSample];
    });
  }, [isNativeBridge, effectiveSleepSec, inputs, profile, capabilities]);

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
      await syncFromNativeDaemon(true);
      return { saved: true };
    }

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

        <main className="flex-1 min-w-0 p-6 lg:p-8 max-w-7xl">
          {activeTab === 'console' && (
            <RuntimeConsoleView
              profile={profile}
              capabilities={capabilities}
              currentSample={currentSample}
              history={history}
              policyState={policyState}
              decision={decision}
              pillarMode={pillarMode}
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

          {activeTab === 'sysfs' && (
            <SysfsInspectorView nodes={sysfsNodes} parsedStatus={parsedStatus} />
          )}

          {activeTab === 'policy' && (
            <PolicyConfigView
              form={policyForm}
              numericSpecs={numericSpecs}
              probe={probe}
              currentDaemonMode={currentDaemonMode}
              onUpdateForm={setPolicyForm}
              onCommitConfig={handleCommitConfig}
              onRestartDaemon={() => handleDaemonAction('restart')}
            />
          )}

          {activeTab === 'baseline' && (
            <BaselineThermalView
              profile={profile}
              experiment={experiment}
              prediction={prediction}
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
              onRefreshLogs={() => syncFromNativeDaemon(true)}
              onClearLogs={() => setLogs([])}
            />
          )}
        </main>
      </div>
    </div>
  );
};

export default App;
