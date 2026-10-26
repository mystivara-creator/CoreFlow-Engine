import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { BaselineThermalView } from './components/BaselineThermalView';
import { ControlSidebar } from './components/ControlSidebar';
import { LogcatStreamView } from './components/LogcatStreamView';
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
  NOTIFICATION_COOLDOWN_SAMPLES,
  selectNotificationEvent,
} from './engine/simulator';
import {
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  CapabilityOverrides,
  DevicePresetId,
  EngineMode,
  LogcatEntry,
  NotificationEvent,
  RuntimeSample,
  RuntimeState,
  WorkloadInputs,
} from './engine/types';

const DEFAULT_INTERVAL =
  Number(import.meta.env.VITE_DEFAULT_MONITOR_INTERVAL) > 0
    ? Number(import.meta.env.VITE_DEFAULT_MONITOR_INTERVAL)
    : 5;

function nowTimeFormatted(): string {
  const d = new Date();
  const hh = String(d.getHours()).padStart(2, '0');
  const mm = String(d.getMinutes()).padStart(2, '0');
  const ss = String(d.getSeconds()).padStart(2, '0');
  return `${hh}:${mm}:${ss}`;
}

function createInitialExperiment(gov: string): BaselineExperimentState {
  return {
    active: false,
    candidateGovernor: gov,
    injectRegression: false,
    baselineSamples: [],
    mutatedSamples: [],
    baselineMeanScore: null,
    mutatedMeanScore: null,
    deltaPct: null,
    outcome: BaselineOutcome.Pending,
    action: BaselineAction.CollectingBaseline,
    summary: 'Idle. Ready to benchmark candidate governor against OEM Factory Baseline.',
  };
}

export function App() {
  const [activeTab, setActiveTab] = useState<ActiveTab>('console');
  const [isRunning, setIsRunning] = useState<boolean>(true);

  const [deviceId, setDeviceId] = useState<DevicePresetId>('kunzite_parrot');
  const [capabilities, setCapabilities] = useState<CapabilityOverrides>(
    DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot
  );
  const [activeScenarioId, setActiveScenarioId] = useState<string | null>('daily_balanced');

  const [inputs, setInputs] = useState<WorkloadInputs>({
    ...SCENARIO_PRESETS[0].inputs,
    monitorIntervalSec: DEFAULT_INTERVAL,
  });

  const profile = HARDWARE_PROFILES[deviceId];

  const [sampleIndex, setSampleIndex] = useState<number>(1);
  const [uptimeSeconds, setUptimeSeconds] = useState<number>(1240);
  const [policyState, setPolicyState] = useState<RuntimeState>(RuntimeState.Normal);
  const [pillarMode, setPillarMode] = useState<EngineMode>(EngineMode.ModeBalanced);
  const [lastNotification, setLastNotification] = useState<NotificationEvent>(
    NotificationEvent.None
  );
  const [lastNotificationSample, setLastNotificationSample] = useState<number>(0);

  const [selectedCandidate, setSelectedCandidate] = useState<string>('schedutil');
  const [injectRegression, setInjectRegression] = useState<boolean>(false);
  const [experiment, setExperiment] = useState<BaselineExperimentState>(() =>
    createInitialExperiment('schedutil')
  );

  const initialSample = useMemo(
    () =>
      buildRuntimeSample(
        1,
        1240,
        { ...SCENARIO_PRESETS[0].inputs, monitorIntervalSec: DEFAULT_INTERVAL },
        HARDWARE_PROFILES.kunzite_parrot,
        DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot,
        [],
        0
      ),
    []
  );

  const [history, setHistory] = useState<RuntimeSample[]>([initialSample]);
  const [currentSample, setCurrentSample] = useState<RuntimeSample>(initialSample);
  const [logs, setLogs] = useState<LogcatEntry[]>(() =>
    formatStartupLogs(
      HARDWARE_PROFILES.kunzite_parrot,
      DEFAULT_CAPABILITIES_FOR_PRESET.kunzite_parrot,
      nowTimeFormatted()
    )
  );

  const stateRef = useRef({
    sampleIndex,
    uptimeSeconds,
    inputs,
    profile,
    capabilities,
    history,
    policyState,
    pillarMode,
    lastNotification,
    lastNotificationSample,
    experiment,
  });

  stateRef.current = {
    sampleIndex,
    uptimeSeconds,
    inputs,
    profile,
    capabilities,
    history,
    policyState,
    pillarMode,
    lastNotification,
    lastNotificationSample,
    experiment,
  };

  const executeTick = useCallback(() => {
    const s = stateRef.current;
    const nextIdx = s.sampleIndex + 1;
    const effectiveSleep =
      s.policyState === RuntimeState.Idle
        ? s.inputs.monitorIntervalSec * 4
        : s.inputs.monitorIntervalSec;
    const nextUptime = s.uptimeSeconds + effectiveSleep;

    const wave = Math.sin(nextIdx * 0.9);
    const sample = buildRuntimeSample(
      nextIdx,
      nextUptime,
      s.inputs,
      s.profile,
      s.capabilities,
      s.history.slice(-HISTORY_WINDOW_SIZE),
      wave
    );

    const nextPolicy = evaluateAdaptivePolicy(sample, s.policyState);
    const decision = decidePolicyAction(nextPolicy);
    const nextPillar = evaluateCoreFlowPillar(s.inputs, sample, s.capabilities);

    const notifEvent = selectNotificationEvent(sample, nextPolicy, s.policyState);
    const cooldownExpired =
      nextIdx >= s.lastNotificationSample + NOTIFICATION_COOLDOWN_SAMPLES;

    const newLogs: LogcatEntry[] = [];
    const ts = nowTimeFormatted();

    if (nextPolicy !== s.policyState) {
      newLogs.push({
        id: `st-${nextIdx}-${Math.random()}`,
        sampleIndex: nextIdx,
        timeFormatted: ts,
        level: nextPolicy === RuntimeState.ThermalGuard ? 'W' : 'I',
        tag: 'CoreFlowAutonomous',
        message: `state: ${s.policyState} -> ${nextPolicy}`,
      });
    }

    if (nextPillar !== s.pillarMode) {
      newLogs.push({
        id: `pl-${nextIdx}-${Math.random()}`,
        sampleIndex: nextIdx,
        timeFormatted: ts,
        level: 'I',
        tag: 'CoreFlowAI',
        message:
          nextPillar === EngineMode.ModeUltraIdle
            ? 'Display blanked. Activated Ultra Deep Sleep (policy4=powersave, read_ahead_kb=256, vendor.dsp.default_qos=0).'
            : `Pillar transition: ${s.pillarMode} -> ${nextPillar} (fg=${s.inputs.foregroundPackage}, cpu=${sample.cpuUsagePct.toFixed(1)}%, gpu=${sample.gpuBusyPct}%)`,
      });
    }

    if (
      notifEvent !== NotificationEvent.None &&
      (notifEvent !== s.lastNotification || cooldownExpired)
    ) {
      newLogs.push({
        id: `nt-${nextIdx}-${Math.random()}`,
        sampleIndex: nextIdx,
        timeFormatted: ts,
        level: 'W',
        tag: 'ThermalGuardian',
        message: `NOTIFICATION_EVENT=${notifEvent} thermal=${(
          sample.thermalMillidegrees / 1000
        ).toFixed(2)}C trend=${sample.thermalTrend} charging=${
          sample.charging ? 'YES' : 'NO'
        }`,
      });
      setLastNotification(notifEvent);
      setLastNotificationSample(nextIdx);
    }

    if (decision !== 'NO_ACTION') {
      newLogs.push({
        id: `dc-${nextIdx}-${Math.random()}`,
        sampleIndex: nextIdx,
        timeFormatted: ts,
        level: 'I',
        tag: 'CoreFlowAutonomous',
        message: `sample=${nextIdx} state=${nextPolicy} decision=${decision} load=${sample.load1.toFixed(
          2
        )} load_trend=${sample.loadTrend} mem_avail=${(
          sample.memAvailableRatio * 100
        ).toFixed(1)}% thermal=${(sample.thermalMillidegrees / 1000).toFixed(
          2
        )}C thermal_trend=${sample.thermalTrend} charging=${
          sample.charging ? 'YES' : 'NO'
        } confidence=${sample.confidence.toFixed(2)}`,
      });
    }

    const expStep = advanceBaselineExperiment(s.experiment, sample);
    if (expStep.logMessage) {
      newLogs.push({
        id: `bi-${nextIdx}-${Math.random()}`,
        sampleIndex: nextIdx,
        timeFormatted: ts,
        level: expStep.logMessage.includes('REGRESSION') ? 'W' : 'I',
        tag: 'CoreFlowAutonomous',
        message: expStep.logMessage,
      });
    }

    setSampleIndex(nextIdx);
    setUptimeSeconds(nextUptime);
    setCurrentSample(sample);
    setHistory((prev) => [...prev.slice(-17), sample]);
    setPolicyState(nextPolicy);
    setPillarMode(nextPillar);
    setExperiment(expStep.next);

    if (newLogs.length > 0) {
      setLogs((prev) => [...newLogs.reverse(), ...prev].slice(0, 150));
    }
  }, []);

  useEffect(() => {
    if (!isRunning) return;
    const timer = window.setInterval(() => {
      executeTick();
    }, 2200);
    return () => window.clearInterval(timer);
  }, [isRunning, executeTick]);

  // Re-evaluate immediate preview sample when sliders or capabilities move
  const liveSample = useMemo(
    () =>
      buildRuntimeSample(
        sampleIndex,
        uptimeSeconds,
        inputs,
        profile,
        capabilities,
        history.slice(-HISTORY_WINDOW_SIZE),
        0
      ),
    [sampleIndex, uptimeSeconds, inputs, profile, capabilities, history]
  );

  const livePolicyState = useMemo(
    () => evaluateAdaptivePolicy(liveSample, policyState),
    [liveSample, policyState]
  );

  const liveDecision = useMemo(
    () => decidePolicyAction(livePolicyState),
    [livePolicyState]
  );

  const livePillarMode = useMemo(
    () => evaluateCoreFlowPillar(inputs, liveSample, capabilities),
    [inputs, liveSample, capabilities]
  );

  const sysfsNodes = useMemo(
    () =>
      computeSysfsNodes(
        profile,
        capabilities,
        livePillarMode,
        livePolicyState,
        liveSample,
        experiment
      ),
    [profile, capabilities, livePillarMode, livePolicyState, liveSample, experiment]
  );

  const prediction = useMemo(
    () =>
      computeThermalPrediction(
        liveSample,
        history.slice(-HISTORY_WINDOW_SIZE),
        inputs.monitorIntervalSec
      ),
    [liveSample, history, inputs.monitorIntervalSec]
  );

  const effectiveSleepSec =
    livePolicyState === RuntimeState.Idle
      ? inputs.monitorIntervalSec * 4
      : inputs.monitorIntervalSec;

  const handleChangeDevice = (nextId: DevicePresetId) => {
    const nextProf = HARDWARE_PROFILES[nextId];
    const nextCaps = DEFAULT_CAPABILITIES_FOR_PRESET[nextId];
    setDeviceId(nextId);
    setCapabilities(nextCaps);
    const defaultCandidate = nextProf.cpuPolicies[1]?.availableGovernors[1] ?? 'schedutil';
    setSelectedCandidate(defaultCandidate);
    setExperiment(createInitialExperiment(defaultCandidate));

    const bootLogs = formatStartupLogs(nextProf, nextCaps, nowTimeFormatted());
    setLogs((prev) => [...bootLogs.reverse(), ...prev].slice(0, 150));
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

  const handleManualInputs = (next: WorkloadInputs) => {
    setActiveScenarioId(null);
    setInputs(next);
  };

  const handleResetEngine = () => {
    const freshCaps = DEFAULT_CAPABILITIES_FOR_PRESET[deviceId];
    setCapabilities(freshCaps);
    setActiveScenarioId('daily_balanced');
    const freshInputs: WorkloadInputs = {
      ...SCENARIO_PRESETS[0].inputs,
      monitorIntervalSec: DEFAULT_INTERVAL,
    };
    setInputs(freshInputs);
    setSampleIndex(1);
    setUptimeSeconds(1240);
    setPolicyState(RuntimeState.Normal);
    setPillarMode(EngineMode.ModeBalanced);
    setLastNotification(NotificationEvent.None);
    setLastNotificationSample(0);
    setExperiment(createInitialExperiment(selectedCandidate));

    const s0 = buildRuntimeSample(1, 1240, freshInputs, profile, freshCaps, [], 0);
    setHistory([s0]);
    setCurrentSample(s0);
    setLogs(formatStartupLogs(profile, freshCaps, nowTimeFormatted()));
  };

  const handleStartExperiment = () => {
    setExperiment({
      active: true,
      candidateGovernor: selectedCandidate,
      injectRegression,
      baselineSamples: [],
      mutatedSamples: [],
      baselineMeanScore: null,
      mutatedMeanScore: null,
      deltaPct: null,
      outcome: BaselineOutcome.Pending,
      action: BaselineAction.CollectingBaseline,
      summary: 'Capturing factory baseline (0/5 samples)...',
    });
  };

  return (
    <div id="top" className="min-h-screen flex flex-col bg-slate-50 text-slate-900">
      <TopNav
        activeTab={activeTab}
        onSelectTab={setActiveTab}
        isRunning={isRunning}
        onToggleRunning={() => setIsRunning((r) => !r)}
      />

      <div className="flex-1 flex flex-col lg:flex-row">
        <ControlSidebar
          deviceId={deviceId}
          onChangeDevice={handleChangeDevice}
          capabilities={capabilities}
          onChangeCapabilities={setCapabilities}
          inputs={inputs}
          onChangeInputs={handleManualInputs}
          activeScenarioId={activeScenarioId}
          onApplyScenario={handleApplyScenario}
          onStepTick={executeTick}
          onResetEngine={handleResetEngine}
        />

        <main className="flex-1 min-w-0 p-6 lg:p-8 max-w-6xl">
          {activeTab === 'console' && (
            <RuntimeConsoleView
              profile={profile}
              capabilities={capabilities}
              currentSample={liveSample}
              history={history}
              policyState={livePolicyState}
              decision={liveDecision}
              pillarMode={livePillarMode}
              lastNotification={lastNotification}
              effectiveSleepSec={effectiveSleepSec}
              sysfsNodes={sysfsNodes}
              onOpenSysfs={() => setActiveTab('sysfs')}
            />
          )}

          {activeTab === 'sysfs' && <SysfsInspectorView nodes={sysfsNodes} />}

          {activeTab === 'baseline' && (
            <BaselineThermalView
              profile={profile}
              experiment={experiment}
              selectedCandidate={selectedCandidate}
              onSelectCandidate={setSelectedCandidate}
              injectRegression={injectRegression}
              onToggleInjectRegression={setInjectRegression}
              onStartExperiment={handleStartExperiment}
              onResetExperiment={() =>
                setExperiment(createInitialExperiment(selectedCandidate))
              }
              prediction={prediction}
            />
          )}

          {activeTab === 'logcat' && (
            <LogcatStreamView
              logs={logs}
              onClearLogs={() => setLogs([])}
              onStepTick={executeTick}
            />
          )}
        </main>
      </div>
    </div>
  );
}

export default App;
