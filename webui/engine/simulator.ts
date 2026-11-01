import {
  AppClass,
  BanditArmId,
  BanditArmState,
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  CapabilityOverrides,
  Decision,
  DeviceHardwareProfile,
  EngineMode,
  HybridBrainEvaluation,
  LogcatEntry,
  NotificationEvent,
  ParetoObjectiveWeights,
  PredictorFeatureRow,
  RuntimeSample,
  RuntimeState,
  SysfsNodeEntry,
  ThermalPrediction,
  ThermalSource,
  Trend,
  WorkloadInputs,
} from './types';

export const HISTORY_WINDOW_SIZE = 6;
export const NOTIFICATION_COOLDOWN_SAMPLES = 12;

// Synchronized with include/coreflow/policy.hpp & src/balance/policy.cpp:
// SoC sensors normally operate in 42–52 °C under sustained mobile load;
// ThermalGuard enters at 55.0 °C and releases at <= 52.0 °C (Warm band >= 48.0 °C).
export const THERMAL_GUARD_ENTER_MC = 55_000;
export const THERMAL_GUARD_EXIT_MC = 52_000;
export const THERMAL_WARM_MC = 48_000;
export const MEMORY_PRESSURE_ENTER = 0.10;
export const MEMORY_PRESSURE_EXIT = 0.15;
export const ELEVATED_LOAD_THRESHOLD = 1.50;
export const IDLE_LOAD_THRESHOLD = 0.20;

export function calculateTrend(oldest: number, newest: number, deadband: number): Trend {
  const delta = newest - oldest;
  if (delta > deadband) return Trend.Rising;
  if (delta < -deadband) return Trend.Falling;
  return Trend.Stable;
}

export function encodeTrendValue(trend: Trend): number {
  switch (trend) {
    case Trend.Falling:
      return -1.0;
    case Trend.Stable:
      return 0.5;
    case Trend.Rising:
      return 1.0;
    case Trend.Unknown:
    default:
      return 0.0;
  }
}

// Synchronized with src/autonomous/autonomous.cpp::calculateConfidence:
// thermal(0.30) + mem(0.20) + cpu(0.15) + charging(0.15) + uptime(0.05) + thermalTrend(0.10) + memTrend(0.05) = 1.00
export function calculateConfidence(sample: RuntimeSample, cpuAvailable = true): number {
  let score = 0.0;
  if (sample.thermalAvailable) score += 0.30;
  if (sample.memTotalKb > 0) score += 0.20;
  if (cpuAvailable && Number.isFinite(sample.cpuUsagePct)) score += 0.15;
  if (sample.chargingTelemetryAvailable) score += 0.15;
  if (sample.uptimeSeconds > 0) score += 0.05;
  if (sample.thermalTrend !== Trend.Unknown) score += 0.10;
  if (sample.memoryTrend !== Trend.Unknown) score += 0.05;
  return Math.min(1.0, Number(score.toFixed(2)));
}

export function buildRuntimeSample(
  sampleIndex: number,
  uptimeSeconds: number,
  inputs: WorkloadInputs,
  profile: DeviceHardwareProfile,
  capabilities: CapabilityOverrides,
  history: RuntimeSample[],
  jitterSeed = 0
): RuntimeSample {
  const tempJitterC = jitterSeed * 0.15;
  const loadJitter = jitterSeed * 0.04;
  const memJitterPct = jitterSeed * 0.2;

  const effectiveSocTempC = Math.max(20, inputs.socTempC + tempJitterC);
  const effectiveBattTempC = Math.max(20, inputs.batteryTempC + tempJitterC * 0.6);
  const effectiveLoad1 = Math.max(0.01, Number((inputs.load1 + loadJitter).toFixed(2)));
  const effectiveMemPct = Math.min(95, Math.max(2, inputs.memAvailablePct + memJitterPct));

  const memTotalKb = capabilities.procAvailable ? profile.memTotalKb : 0;
  const memAvailableKb = Math.round((memTotalKb * effectiveMemPct) / 100);
  const memAvailableRatio = memTotalKb > 0 ? memAvailableKb / memTotalKb : 0;

  const thermalAvailable = capabilities.sysAvailable && profile.thermalZonesCount > 0;
  const thermalMillidegrees = thermalAvailable ? Math.round(effectiveSocTempC * 1000) : 0;
  const hottestThermalMillidegrees = thermalAvailable
    ? Math.round((effectiveSocTempC + 1.6) * 1000)
    : 0;

  const thermalSource = !thermalAvailable
    ? ThermalSource.Unknown
    : profile.hasPrimaryThermalSensor
      ? ThermalSource.Primary
      : ThermalSource.Fallback;

  const chargingTelemetryAvailable =
    capabilities.sysAvailable && profile.charging.batteryAvailable && profile.charging.telemetryReadable;

  const sample: RuntimeSample = {
    sampleIndex,
    uptimeSeconds: capabilities.procAvailable ? uptimeSeconds : 0,
    cpuUsagePct: Math.min(100, Math.max(0, Number((inputs.cpuUsagePct + jitterSeed * 1.2).toFixed(1)))),
    gpuBusyPct: capabilities.kgslAvailable
      ? Math.min(100, Math.max(0, Math.round(inputs.gpuBusyPct + jitterSeed * 1.5)))
      : 0,
    load1: effectiveLoad1,
    memTotalKb,
    memAvailableKb,
    memAvailableRatio,
    thermalAvailable,
    thermalMillidegrees,
    hottestThermalMillidegrees,
    thermalSource,
    charging: chargingTelemetryAvailable ? inputs.charging : false,
    chargingTelemetryAvailable,
    batteryPercent: inputs.batteryPercent,
    batteryTempMillidegrees: chargingTelemetryAvailable ? Math.round(effectiveBattTempC * 1000) : 0,
    batteryCurrentMicroamps: chargingTelemetryAvailable ? inputs.batteryCurrentMa * 1000 : 0,
    batteryVoltageMicrovolts: chargingTelemetryAvailable ? inputs.batteryVoltageMv * 1000 : 0,
    ioReadKbs: Math.max(0, Math.round(inputs.ioReadKbs + jitterSeed * 12)),
    ioWriteKbs: Math.max(0, Math.round(inputs.ioWriteKbs + jitterSeed * 6)),
    thermalTrend: Trend.Unknown,
    memoryTrend: Trend.Unknown,
    loadTrend: Trend.Unknown,
    confidence: 0,
  };

  if (history.length >= 2) {
    const oldest = history[0];
    if (sample.thermalAvailable && oldest.thermalAvailable) {
      sample.thermalTrend = calculateTrend(
        oldest.thermalMillidegrees,
        sample.thermalMillidegrees,
        500.0
      );
    }
    if (sample.memTotalKb > 0 && oldest.memTotalKb > 0) {
      sample.memoryTrend = calculateTrend(
        oldest.memAvailableRatio,
        sample.memAvailableRatio,
        0.015
      );
    }
    sample.loadTrend = calculateTrend(oldest.load1, sample.load1, 0.20);
  }

  sample.confidence = calculateConfidence(sample, capabilities.procAvailable);
  return sample;
}

// Synchronized with src/balance/policy.cpp::PolicyEngine::evaluateState
export function evaluateAdaptivePolicy(
  sample: RuntimeSample,
  previous: RuntimeState
): RuntimeState {
  if (sample.thermalAvailable) {
    if (
      previous === RuntimeState.ThermalGuard &&
      sample.thermalMillidegrees > THERMAL_GUARD_EXIT_MC
    ) {
      return RuntimeState.ThermalGuard;
    }
    if (sample.thermalMillidegrees >= THERMAL_GUARD_ENTER_MC) {
      return RuntimeState.ThermalGuard;
    }
  }

  if (sample.memTotalKb > 0) {
    if (
      previous === RuntimeState.Pressure &&
      sample.memAvailableRatio < MEMORY_PRESSURE_EXIT
    ) {
      return RuntimeState.Pressure;
    }
    if (sample.memAvailableRatio < MEMORY_PRESSURE_ENTER) {
      return RuntimeState.Pressure;
    }
  }

  if (sample.thermalAvailable) {
    if (
      sample.thermalMillidegrees >= THERMAL_GUARD_EXIT_MC ||
      (sample.thermalMillidegrees >= THERMAL_WARM_MC && sample.thermalTrend === Trend.Rising)
    ) {
      return RuntimeState.Warming;
    }
  }

  if (sample.load1 >= ELEVATED_LOAD_THRESHOLD) {
    return RuntimeState.Elevated;
  }

  if (
    sample.load1 < IDLE_LOAD_THRESHOLD &&
    sample.thermalTrend !== Trend.Rising &&
    (!sample.thermalAvailable || sample.thermalMillidegrees < THERMAL_WARM_MC)
  ) {
    return RuntimeState.Idle;
  }

  return RuntimeState.Normal;
}

export function decidePolicyAction(state: RuntimeState): Decision {
  switch (state) {
    case RuntimeState.ThermalGuard:
    case RuntimeState.Pressure:
      return Decision.ReduceIntervention;
    case RuntimeState.Warming:
    case RuntimeState.Elevated:
      return Decision.Observe;
    case RuntimeState.Idle:
    case RuntimeState.Normal:
    default:
      return Decision.NoAction;
  }
}

export function selectNotificationEvent(
  sample: RuntimeSample,
  state: RuntimeState,
  previous: RuntimeState
): NotificationEvent {
  if (state === RuntimeState.ThermalGuard) {
    if (previous !== RuntimeState.ThermalGuard) {
      return NotificationEvent.ThermalGuard;
    }
    if (sample.charging && sample.thermalTrend === Trend.Rising) {
      return NotificationEvent.ChargingProtection;
    }
    return NotificationEvent.None;
  }

  if (sample.thermalAvailable && sample.thermalTrend === Trend.Rising) {
    if (sample.thermalMillidegrees >= THERMAL_GUARD_EXIT_MC) {
      return NotificationEvent.ThermalWarning;
    }
    if (sample.thermalMillidegrees >= THERMAL_WARM_MC) {
      return NotificationEvent.ThermalWarming;
    }
  }

  return NotificationEvent.None;
}

export function evaluateCoreFlowPillar(
  inputs: WorkloadInputs,
  sample: RuntimeSample,
  capabilities: CapabilityOverrides
): EngineMode {
  if (!inputs.screenOn) {
    return EngineMode.ModeUltraIdle;
  }

  const gpuBusy = capabilities.kgslAvailable ? sample.gpuBusyPct : 0;

  if (
    inputs.foregroundApp === AppClass.Game ||
    gpuBusy > 45 ||
    sample.cpuUsagePct > 75.0
  ) {
    return EngineMode.ModeGaming;
  }

  if (
    inputs.foregroundApp === AppClass.Launcher ||
    (sample.cpuUsagePct < 15.0 && gpuBusy < 8)
  ) {
    return EngineMode.ModeIdle;
  }

  if (sample.cpuUsagePct >= 25.0 && sample.cpuUsagePct <= 75.0) {
    return EngineMode.ModeBurst;
  }

  return EngineMode.ModeBalanced;
}

function clamp01(v: number): number {
  if (!Number.isFinite(v)) return 0;
  return Math.min(1, Math.max(0, v));
}

function lerpInt(a: number, b: number, t: number): number {
  return Math.round(a + (b - a) * clamp01(t));
}

function quantizePowerOfTwo(value: number, minPow2: number, maxPow2: number): number {
  const clamped = Math.min(maxPow2, Math.max(minPow2, value));
  const exp = Math.round(Math.log2(Math.max(1, clamped)));
  return Math.min(maxPow2, Math.max(minPow2, 2 ** exp));
}

export function createInitialBanditArms(): BanditArmState[] {
  return [
    {
      id: 'OEM_STOCK_CONSERVE',
      label: 'OEM Factory Conservation',
      description: 'Maintains stock factory baselines & deep-sleep efficiency when demand is low.',
      pulls: 4,
      meanReward: 0.64,
      variance: 0.012,
      ucbScore: 0.72,
      explorationBonus: 0.08,
    },
    {
      id: 'SILICON_RAW_UNLOCK',
      label: 'Raw Silicon Envelope Unlock',
      description: 'Unlocks OEM-throttled WALT/schedutil rate limits & upper hardware OPPs under clean headroom.',
      pulls: 3,
      meanReward: 0.74,
      variance: 0.018,
      ucbScore: 0.84,
      explorationBonus: 0.10,
    },
    {
      id: 'ENCORE_FRAME_PACING',
      label: 'Encore Dynamic Frame Lock',
      description: 'Coordinates GPU OPP floor, foreground uclamp, A55 background isolation & BBR congestion.',
      pulls: 3,
      meanReward: 0.77,
      variance: 0.016,
      ucbScore: 0.87,
      explorationBonus: 0.10,
    },
    {
      id: 'HYPER_INTERACTIVE_RAMP',
      label: 'Interactive Launch & I/O Ramp',
      description: 'Accelerates foreground ramp & UFS read-ahead proportionally to transient CPU/I/O spikes.',
      pulls: 3,
      meanReward: 0.71,
      variance: 0.015,
      ucbScore: 0.81,
      explorationBonus: 0.10,
    },
    {
      id: 'THERMAL_PARETO_RELIEF',
      label: 'Pareto Thermal & ZRAM Guard',
      description: 'Preemptively trims background cgroups & governor residency before crossing 55.0 °C.',
      pulls: 3,
      meanReward: 0.73,
      variance: 0.011,
      ucbScore: 0.82,
      explorationBonus: 0.09,
    },
  ];
}

export function computeParetoWeights(
  inputs: WorkloadInputs,
  sample: RuntimeSample,
  policyState: RuntimeState,
  prediction: ThermalPrediction
): ParetoObjectiveWeights {
  const tempC = sample.thermalMillidegrees / 1000;
  const guardEnterC = THERMAL_GUARD_ENTER_MC / 1000;
  const warmC = THERMAL_WARM_MC / 1000;

  let throughput = 0.20 + (sample.cpuUsagePct / 100) * 0.28 + Math.min(0.18, sample.ioReadKbs / 10_000);
  let frameLatency =
    0.18 +
    (inputs.foregroundApp === AppClass.Game ? 0.32 : inputs.screenOn ? 0.14 : 0.0) +
    (sample.gpuBusyPct / 100) * 0.26;
  let thermalSafety =
    0.22 +
    clamp01((Math.max(tempC, prediction.predictedTempC) - 36) / (guardEnterC - 36)) * 0.52 +
    (policyState === RuntimeState.ThermalGuard || prediction.preemptiveTrigger ? 0.35 : 0);
  let powerEfficiency =
    0.20 +
    (!inputs.screenOn ? 0.42 : 0) +
    (!sample.charging ? clamp01((100 - sample.batteryPercent) / 100) * 0.22 : 0.04) +
    (tempC < warmC && sample.cpuUsagePct < 20 ? 0.18 : 0);

  if (policyState === RuntimeState.ThermalGuard || tempC >= guardEnterC) {
    throughput *= 0.30;
    frameLatency *= 0.40;
  }

  const total = Math.max(1e-6, throughput + frameLatency + thermalSafety + powerEfficiency);
  return {
    throughput: Number((throughput / total).toFixed(3)),
    frameLatency: Number((frameLatency / total).toFixed(3)),
    thermalSafety: Number((thermalSafety / total).toFixed(3)),
    powerEfficiency: Number((powerEfficiency / total).toFixed(3)),
  };
}

export function evaluateHybridBrain(
  profile: DeviceHardwareProfile,
  capabilities: CapabilityOverrides,
  inputs: WorkloadInputs,
  sample: RuntimeSample,
  policyState: RuntimeState,
  prediction: ThermalPrediction,
  arms: BanditArmState[],
  mutationArmed = true
): HybridBrainEvaluation {
  const paretoWeights = computeParetoWeights(inputs, sample, policyState, prediction);
  const tempC = sample.thermalMillidegrees / 1000;
  const guardEnterC = THERMAL_GUARD_ENTER_MC / 1000;
  const warmC = THERMAL_WARM_MC / 1000;

  const thermalHeadroom = clamp01((guardEnterC - Math.max(tempC, prediction.predictedTempC)) / (guardEnterC - 34));
  const gpuDemand = capabilities.kgslAvailable ? sample.gpuBusyPct / 100 : 0;
  const cpuDemand = sample.cpuUsagePct / 100;
  const ioDemand = clamp01((sample.ioReadKbs + sample.ioWriteKbs) / 2400);

  const isStressed =
    policyState === RuntimeState.ThermalGuard ||
    policyState === RuntimeState.Pressure ||
    tempC >= guardEnterC ||
    prediction.preemptiveTrigger;

  const totalPulls = Math.max(
    1,
    arms.reduce((acc, arm) => acc + arm.pulls, 0)
  );
  const logTotal = Math.log(1 + totalPulls);

  const scoredArms: BanditArmState[] = arms.map((arm) => {
    let contextFit = 0.5;
    switch (arm.id) {
      case 'OEM_STOCK_CONSERVE':
        contextFit =
          paretoWeights.powerEfficiency * 0.95 +
          (!inputs.screenOn || (cpuDemand < 0.24 && gpuDemand < 0.15) ? 0.34 : 0.06);
        break;
      case 'SILICON_RAW_UNLOCK':
        contextFit = isStressed
          ? 0.08
          : (paretoWeights.throughput * 0.60 + paretoWeights.frameLatency * 0.40) *
              (0.55 + 0.45 * thermalHeadroom) +
            (cpuDemand >= 0.45 ? 0.20 : 0);
        break;
      case 'ENCORE_FRAME_PACING':
        contextFit = isStressed
          ? 0.10
          : paretoWeights.frameLatency * 0.72 +
            (inputs.foregroundApp === AppClass.Game || gpuDemand >= 0.42 ? 0.30 : 0.02) *
              (0.50 + 0.50 * thermalHeadroom);
        break;
      case 'HYPER_INTERACTIVE_RAMP':
        contextFit = isStressed
          ? 0.10
          : (paretoWeights.throughput * 0.48 + paretoWeights.frameLatency * 0.42) +
            (inputs.screenOn && (ioDemand >= 0.28 || (cpuDemand >= 0.28 && cpuDemand <= 0.76)) ? 0.24 : 0.04);
        break;
      case 'THERMAL_PARETO_RELIEF':
        contextFit =
          paretoWeights.thermalSafety * 0.85 +
          (isStressed || tempC >= warmC ? 0.38 : 0.04);
        break;
    }

    const blendedMean = clamp01(arm.meanReward * 0.55 + clamp01(contextFit) * 0.45);
    const vBound = Math.min(0.25, arm.variance + Math.sqrt((2 * logTotal) / (arm.pulls + 1)));
    const explorationBonus = isStressed && arm.id !== 'THERMAL_PARETO_RELIEF'
      ? 0
      : Number((0.085 * Math.sqrt((logTotal / (arm.pulls + 1)) * vBound)).toFixed(3));
    const ucbScore = Number(clamp01(blendedMean + explorationBonus).toFixed(3));
    return {
      ...arm,
      ucbScore,
      explorationBonus,
    };
  });

  const sorted = [...scoredArms].sort((a, b) => b.ucbScore - a.ucbScore);
  const bestArm = !mutationArmed
    ? scoredArms.find((a) => a.id === 'OEM_STOCK_CONSERVE') ?? sorted[0]
    : isStressed
      ? scoredArms.find((a) => a.id === 'THERMAL_PARETO_RELIEF') ?? sorted[0]
      : sorted[0];

  const rawWorkloadIntensity = clamp01(Math.max(cpuDemand, gpuDemand * 1.08, ioDemand * 0.75));
  const unlockDrive =
    !mutationArmed || isStressed || !inputs.screenOn
      ? 0
      : bestArm.id === 'OEM_STOCK_CONSERVE'
        ? 0
        : Number(
            clamp01(
              rawWorkloadIntensity *
                (0.45 + 0.55 * thermalHeadroom) *
                (tempC >= warmC ? 0.65 : 1.0)
            ).toFixed(3)
          );

  const latencyDrive =
    !mutationArmed || isStressed || !inputs.screenOn
      ? 0
      : bestArm.id === 'ENCORE_FRAME_PACING' || bestArm.id === 'HYPER_INTERACTIVE_RAMP' || bestArm.id === 'SILICON_RAW_UNLOCK'
        ? Number(
            clamp01(
              (paretoWeights.frameLatency * 1.15 + gpuDemand * 0.45 + ioDemand * 0.25) *
                (0.50 + 0.50 * thermalHeadroom)
            ).toFixed(3)
          )
        : 0;

  const containmentDrive = !mutationArmed
    ? 0
    : Number(
        clamp01(
          isStressed
            ? 0.92
            : !inputs.screenOn
              ? 0.75
              : tempC >= warmC || inputs.foregroundApp === AppClass.Game
                ? 0.58
                : 0
        ).toFixed(3)
      );

  // Measure hardware OEM throttling ratio across clusters & GPU and compute unlocked %
  const perfCluster = profile.cpuPolicies[profile.cpuPolicies.length - 1];
  const loadSpan = Math.max(1, perfCluster.oemTargetLoad - perfCluster.siliconOptimalLoad);
  const rampSpan = Math.max(1, perfCluster.oemUpRateLimitUs - perfCluster.siliconMinUpRateLimitUs);
  const unlockedLoadFrac = (loadSpan * unlockDrive) / loadSpan;
  const unlockedRampFrac = (rampSpan * unlockDrive) / rampSpan;
  const siliconGapPct = Math.round(((unlockedLoadFrac + unlockedRampFrac) * 0.5) * 100);

  const reasoningSummary = !mutationArmed
    ? 'Observe-only gate active: Hybrid Brain computes counterfactual Pareto + UCB scores without writing sysfs.'
    : `${bestArm.label} (UCB=${bestArm.ucbScore.toFixed(2)}) | Pareto[P=${paretoWeights.throughput.toFixed(2)}, L=${paretoWeights.frameLatency.toFixed(2)}, T=${paretoWeights.thermalSafety.toFixed(2)}, E=${paretoWeights.powerEfficiency.toFixed(2)}] | Silicon unlock=${siliconGapPct}%`;

  return {
    selectedArm: bestArm.id,
    selectedArmLabel: bestArm.label,
    paretoWeights,
    arms: scoredArms,
    unlockDrive,
    latencyDrive,
    containmentDrive,
    siliconGapPct,
    reasoningSummary,
  };
}

export function advanceBanditArms(
  arms: BanditArmState[],
  selectedArm: BanditArmId,
  sample: RuntimeSample,
  prediction: ThermalPrediction
): BanditArmState[] {
  const tempC = sample.thermalMillidegrees / 1000;
  const guardC = THERMAL_GUARD_ENTER_MC / 1000;
  const thermalPenalty = clamp01((Math.max(tempC, prediction.predictedTempC) - 44) / (guardC - 40)) * 0.28;
  const throughputGain = clamp01((sample.cpuUsagePct * 0.55 + sample.gpuBusyPct * 0.45) / 100) * 0.24;
  const memHealth = clamp01(sample.memAvailableRatio * 1.4) * 0.14;
  const observedReward = clamp01(0.58 + throughputGain + memHealth - thermalPenalty);

  return arms.map((arm) => {
    if (arm.id !== selectedArm) return arm;
    const nextPulls = arm.pulls + 1;
    const delta = observedReward - arm.meanReward;
    const nextMean = arm.meanReward + delta / nextPulls;
    const nextVar = clamp01(
      ((arm.pulls * arm.variance) + delta * (observedReward - nextMean)) / nextPulls
    );
    return {
      ...arm,
      pulls: nextPulls,
      meanReward: Number(nextMean.toFixed(3)),
      variance: Number(Math.min(0.25, Math.max(0.004, nextVar)).toFixed(4)),
    };
  });
}

export function computeSysfsNodes(
  profile: DeviceHardwareProfile,
  capabilities: CapabilityOverrides,
  mode: EngineMode,
  policyState: RuntimeState,
  sample: RuntimeSample,
  experiment: BaselineExperimentState,
  mutationArmed = true,
  brain?: HybridBrainEvaluation
): SysfsNodeEntry[] {
  const sysOk = capabilities.sysAvailable;
  const procOk = capabilities.procAvailable && capabilities.vmSysctlWritable;

  const isThermalStabilizing =
    mutationArmed &&
    (policyState === RuntimeState.ThermalGuard ||
      sample.thermalMillidegrees >= THERMAL_GUARD_ENTER_MC);

  const unlockDrive =
    brain?.unlockDrive ??
    (!mutationArmed || isThermalStabilizing || policyState === RuntimeState.Pressure
      ? 0
      : mode === EngineMode.ModeGaming
        ? 0.78
        : mode === EngineMode.ModeBurst
          ? 0.48
          : 0);

  const latencyDrive =
    brain?.latencyDrive ??
    (!mutationArmed || isThermalStabilizing
      ? 0
      : mode === EngineMode.ModeGaming
        ? 0.74
        : mode === EngineMode.ModeBurst
          ? 0.52
          : 0);

  const containmentDrive =
    brain?.containmentDrive ??
    (!mutationArmed
      ? 0
      : isThermalStabilizing || policyState === RuntimeState.Pressure
        ? 0.90
        : mode === EngineMode.ModeUltraIdle
          ? 0.72
          : mode === EngineMode.ModeGaming
            ? 0.55
            : 0);

  const expGovernorActive =
    experiment.active &&
    (experiment.action === BaselineAction.ObservingMutation ||
      experiment.action === BaselineAction.Keep ||
      experiment.action === BaselineAction.Hold);

  const nodes: SysfsNodeEntry[] = [];

  // 1. Dynamic CPU Cluster Iteration (zero hardcoded policy0/policy4 paths or target loads)
  for (const cluster of profile.cpuPolicies) {
    const canPowersaveSleep =
      mutationArmed &&
      mode === EngineMode.ModeUltraIdle &&
      cluster.role !== 'efficiency' &&
      cluster.availableGovernors.includes('powersave');

    const activeGov = canPowersaveSleep
      ? 'powersave'
      : expGovernorActive && cluster.availableGovernors.includes(experiment.candidateGovernor)
        ? experiment.candidateGovernor
        : cluster.stockGovernor;

    nodes.push({
      path: `${cluster.policyPath}/scaling_governor`,
      subsystem: 'CPU & WALT',
      baselineValue: cluster.stockGovernor,
      currentValue: sysOk ? activeGov : 'N/A',
      status: !sysOk ? 'SKIPPED' : activeGov !== cluster.stockGovernor ? 'MUTATED' : 'BASELINE',
      domainEnvelope: cluster.availableGovernors.join(' | '),
      rationale: canPowersaveSleep
        ? 'Screen-off conservation selects advertised powersave governor on non-efficiency cluster'
        : expGovernorActive
          ? `CpuFreqActuator verifying candidate "${activeGov}" against 4-sample OEM baseline`
          : mutationArmed
            ? `Preserves kernel-advertised "${cluster.stockGovernor}" governor while fine-tuning sub-attributes`
            : 'Observe mode active: read-only snapshot',
    });

    if (cluster.loadTunableLeaf) {
      const stockLoadStr = String(cluster.oemTargetLoad);
      const targetLoadNum = isThermalStabilizing
        ? lerpInt(cluster.oemTargetLoad, cluster.thermalClampLoad, containmentDrive)
        : unlockDrive > 0.05
          ? lerpInt(cluster.oemTargetLoad, cluster.siliconOptimalLoad, unlockDrive)
          : mode === EngineMode.ModeUltraIdle && mutationArmed
            ? lerpInt(cluster.oemTargetLoad, cluster.thermalClampLoad, 0.5)
            : cluster.oemTargetLoad;
      const activeLoadStr = String(targetLoadNum);
      const waltReady = sysOk && capabilities.waltAvailable;

      nodes.push({
        path: `${cluster.policyPath}/${cluster.loadTunableLeaf}`,
        subsystem: 'CPU & WALT',
        baselineValue: stockLoadStr,
        currentValue: waltReady ? activeLoadStr : 'SKIPPED',
        status: !waltReady
          ? 'SKIPPED'
          : isThermalStabilizing && activeLoadStr !== stockLoadStr
            ? 'CLAMPED'
            : activeLoadStr !== stockLoadStr
              ? 'MUTATED'
              : 'BASELINE',
        domainEnvelope: `[1..100] · Silicon opt ${cluster.siliconOptimalLoad}`,
        rationale: waltReady
          ? isThermalStabilizing
            ? `Stabilizing epoch raises target load (${stockLoadStr} -> ${activeLoadStr}) to shed die heat`
            : activeLoadStr !== stockLoadStr
              ? `Unlocks OEM-throttled OPP residency (${stockLoadStr} -> ${activeLoadStr}) via Pareto drive ${(unlockDrive * 100).toFixed(0)}%`
              : `OEM factory load threshold (${stockLoadStr}) preserved`
          : 'Kernel does not expose WALT target load attribute; skipped cleanly',
      });
    }

    const stockUpRateStr = String(cluster.oemUpRateLimitUs);
    const activeUpRateNum = isThermalStabilizing
      ? Math.round(cluster.oemUpRateLimitUs * (1 + 0.5 * containmentDrive))
      : unlockDrive > 0.05
        ? lerpInt(cluster.oemUpRateLimitUs, cluster.siliconMinUpRateLimitUs, unlockDrive)
        : cluster.oemUpRateLimitUs;
    const activeUpRateStr = String(activeUpRateNum);

    nodes.push({
      path: `${cluster.policyPath}/${cluster.governorSubdir}/up_rate_limit_us`,
      subsystem: 'CPU & WALT',
      baselineValue: stockUpRateStr,
      currentValue: sysOk ? activeUpRateStr : 'SKIPPED',
      status: !sysOk
        ? 'SKIPPED'
        : isThermalStabilizing && activeUpRateStr !== stockUpRateStr
          ? 'CLAMPED'
          : activeUpRateStr !== stockUpRateStr
            ? 'MUTATED'
            : 'BASELINE',
      domainEnvelope: `${cluster.siliconMinUpRateLimitUs}..${cluster.oemUpRateLimitUs * 2} µs`,
      rationale: activeUpRateStr !== stockUpRateStr
        ? isThermalStabilizing
          ? `Slows upward DVFS ramp (${activeUpRateStr} µs) during ThermalGuard hysteresis`
          : `Eliminates OEM ramp lag (${stockUpRateStr} -> ${activeUpRateStr} µs) toward raw silicon spec`
        : `OEM governor upward transition rate (${stockUpRateStr} µs) preserved`,
    });

    const maxStockStr = String(cluster.maxFreqKhz);
    const oemFreqCapped = cluster.maxFreqKhz < cluster.siliconMaxFreqKhz;
    nodes.push({
      path: `${cluster.policyPath}/scaling_max_freq`,
      subsystem: 'CPU & WALT',
      baselineValue: maxStockStr,
      currentValue: sysOk ? maxStockStr : 'N/A',
      status: !sysOk ? 'SKIPPED' : 'BASELINE',
      domainEnvelope: `${(cluster.minFreqKhz / 1e6).toFixed(2)}..${(cluster.siliconMaxFreqKhz / 1e6).toFixed(2)} GHz (${cluster.oppTableKhz.length} OPPs)`,
      rationale: oemFreqCapped
        ? `FrameworkOwned (Power HAL): OEM caps boot max at ${(cluster.maxFreqKhz / 1e6).toFixed(2)} GHz vs ${(cluster.siliconMaxFreqKhz / 1e6).toFixed(2)} GHz silicon max; steered safely via WALT & uclamp`
        : 'FrameworkOwned (Power HAL / thermal-engine): observed read-only to prevent HAL conflict',
    });
  }

  // 2. Cgroup v2 uclamp & Encore-style Background Cpuset Containment
  const cg = profile.cgroups;
  const maxSafeUclampFloor = Math.round(
    (profile.cpuPolicies[0].minFreqKhz / profile.cpuPolicies[profile.cpuPolicies.length - 1].siliconMaxFreqKhz) * 140
  );
  const activeTopUclampNum =
    isThermalStabilizing || latencyDrive <= 0.08
      ? cg.oemTopAppUclampMin
      : lerpInt(cg.oemTopAppUclampMin, Math.min(32, Math.max(12, maxSafeUclampFloor)), latencyDrive);
  const stockTopUclampStr = String(cg.oemTopAppUclampMin);
  const activeTopUclampStr = String(activeTopUclampNum);

  nodes.push({
    path: cg.topAppUclampMinPath,
    subsystem: 'CPU & WALT',
    baselineValue: stockTopUclampStr,
    currentValue: sysOk ? activeTopUclampStr : 'SKIPPED',
    status: !sysOk ? 'SKIPPED' : activeTopUclampStr !== stockTopUclampStr ? 'MUTATED' : 'BASELINE',
    domainEnvelope: '[0..100 %]',
    rationale:
      activeTopUclampStr !== stockTopUclampStr
        ? `Dynamic foreground uclamp floor (${activeTopUclampStr}%) scaled by Pareto frame-latency weight`
        : 'Zero floor preserved when idle or thermal/observe gates hold proactive boost',
  });

  const activeBgUclampStr =
    containmentDrive >= 0.50
      ? String(lerpInt(80, 38, containmentDrive))
      : cg.oemBgUclampMax;

  nodes.push({
    path: cg.bgUclampMaxPath,
    subsystem: 'CPU & WALT',
    baselineValue: cg.oemBgUclampMax,
    currentValue: sysOk ? activeBgUclampStr : 'SKIPPED',
    status: !sysOk
      ? 'SKIPPED'
      : activeBgUclampStr !== cg.oemBgUclampMax
        ? isThermalStabilizing || policyState === RuntimeState.Pressure
          ? 'CLAMPED'
          : 'MUTATED'
        : 'BASELINE',
    domainEnvelope: '[0..100 % | max]',
    rationale:
      activeBgUclampStr !== cg.oemBgUclampMax
        ? `Contains background daemons (${activeBgUclampStr}%) to protect foreground thermal/cache budget`
        : 'Default cgroup background ceiling preserved',
  });

  const effCores = profile.cpuPolicies[0].coresRange;
  const activeBgCpus =
    containmentDrive >= 0.55 && cg.oemBgCpus !== effCores ? effCores : cg.oemBgCpus;
  nodes.push({
    path: cg.bgCpusetPath,
    subsystem: 'CPU & WALT',
    baselineValue: cg.oemBgCpus,
    currentValue: sysOk ? activeBgCpus : 'SKIPPED',
    status: !sysOk
      ? 'SKIPPED'
      : activeBgCpus !== cg.oemBgCpus
        ? isThermalStabilizing
          ? 'CLAMPED'
          : 'MUTATED'
        : 'BASELINE',
    domainEnvelope: `Online ${cg.oemBgCpus} -> Eff ${effCores}`,
    rationale:
      activeBgCpus !== cg.oemBgCpus
        ? `Encore-style L2 cache isolation: pins background tasks to efficiency cores (${effCores})`
        : `OEM background CPU mask (${cg.oemBgCpus}) preserved`,
  });

  // 3. Discovered Storage Queue Topology (zero hardcoded /sys/block/sda or 1024 literals)
  const st = profile.storage;
  const stockRaStr = String(st.stockReadAheadKb);
  const ioThroughputRatio = clamp01((sample.ioReadKbs + sample.ioWriteKbs) / 2200);
  const desiredRa = isThermalStabilizing
    ? quantizePowerOfTwo(st.stockReadAheadKb / 2, st.minReadAheadKb, st.maxReadAheadKb)
    : mutationArmed && mode === EngineMode.ModeUltraIdle
      ? quantizePowerOfTwo(st.minReadAheadKb * 2, st.minReadAheadKb, st.stockReadAheadKb)
      : unlockDrive > 0.12
        ? quantizePowerOfTwo(
            lerpInt(st.stockReadAheadKb, st.maxReadAheadKb, clamp01(unlockDrive * 0.65 + ioThroughputRatio * 0.45)),
            st.minReadAheadKb,
            st.maxReadAheadKb
          )
        : st.stockReadAheadKb;
  const activeRaStr = String(desiredRa);

  const stockNrStr = String(st.stockNrRequests);
  const activeNrStr =
    unlockDrive > 0.25 && !isThermalStabilizing
      ? String(lerpInt(st.stockNrRequests, st.maxQueueDepth, unlockDrive))
      : stockNrStr;

  const stockRqStr = String(st.stockRqAffinity);
  const activeRqStr = isThermalStabilizing
    ? String(Math.max(0, st.stockRqAffinity - 1))
    : stockRqStr;

  nodes.push({
    path: `${st.queuePath}/read_ahead_kb`,
    subsystem: 'Block I/O',
    baselineValue: stockRaStr,
    currentValue: sysOk ? activeRaStr : 'N/A',
    status: !sysOk
      ? 'SKIPPED'
      : isThermalStabilizing && activeRaStr !== stockRaStr
        ? 'CLAMPED'
        : activeRaStr !== stockRaStr
          ? 'MUTATED'
          : 'BASELINE',
    domainEnvelope: `${st.minReadAheadKb}..${st.maxReadAheadKb} KB (${st.interfaceType})`,
    rationale: mutationArmed
      ? `Throughput-quantized read-ahead (${activeRaStr} KB) derived from [${st.minReadAheadKb}..${st.maxReadAheadKb}]`
      : 'Observe mode active: factory read-ahead preserved',
  });

  nodes.push({
    path: `${st.queuePath}/nr_requests`,
    subsystem: 'Block I/O',
    baselineValue: stockNrStr,
    currentValue: sysOk ? activeNrStr : 'N/A',
    status: !sysOk ? 'SKIPPED' : activeNrStr !== stockNrStr ? 'MUTATED' : 'BASELINE',
    domainEnvelope: `4..${st.maxQueueDepth} tags`,
    rationale: `Capability-verified ${st.interfaceType} queue depth scaled by workload intensity`,
  });

  nodes.push({
    path: `${st.queuePath}/rq_affinity`,
    subsystem: 'Block I/O',
    baselineValue: stockRqStr,
    currentValue: sysOk ? activeRqStr : 'N/A',
    status: !sysOk ? 'SKIPPED' : activeRqStr !== stockRqStr ? 'CLAMPED' : 'BASELINE',
    domainEnvelope: '[0..2]',
    rationale:
      activeRqStr !== stockRqStr
        ? 'Relaxes originating-CPU completion affinity during ThermalGuard to spread IRQ load'
        : 'Kernel domain [0..2] strict originating-CPU completion preserved',
  });

  // 4. Linux 5.8+ ZRAM-Aware VM Sysctls
  const stockSwapStr = String(profile.stockSwappiness);
  const activeSwapStr = !procOk
    ? 'SKIPPED'
    : isThermalStabilizing && profile.stockSwappiness > 100
      ? String(lerpInt(profile.stockSwappiness, 100, containmentDrive * 0.5))
      : stockSwapStr;

  const memDeficit = clamp01((0.22 - sample.memAvailableRatio) / 0.16);
  const stockVfsStr = String(profile.stockVfsCachePressure);
  const activeVfsStr = !procOk
    ? 'SKIPPED'
    : mutationArmed && policyState === RuntimeState.Pressure
      ? String(Math.min(500, Math.round(profile.stockVfsCachePressure * (1 + 0.12 + 0.18 * memDeficit))))
      : unlockDrive > 0.25
        ? String(Math.max(50, Math.round(profile.stockVfsCachePressure * (1 - 0.14 * unlockDrive))))
        : stockVfsStr;

  const stockPageClusterStr = String(profile.stockPageCluster);

  nodes.push({
    path: '/proc/sys/vm/swappiness',
    subsystem: 'VM / Sysctl',
    baselineValue: stockSwapStr,
    currentValue: activeSwapStr,
    status: !procOk ? 'SKIPPED' : activeSwapStr !== stockSwapStr ? 'CLAMPED' : 'BASELINE',
    domainEnvelope: `[0..200] · ${profile.zramSizeMb} MB ZRAM`,
    rationale:
      policyState === RuntimeState.Pressure
        ? `Linux 5.8+ ZRAM domain [0..200]: OEM baseline ${stockSwapStr} preserved (never assumes desktop 60)`
        : activeSwapStr !== stockSwapStr
          ? `Stabilizing thermal relief trims ZRAM compression CPU (${stockSwapStr} -> ${activeSwapStr})`
          : `OEM ZRAM factory posture (${stockSwapStr}/200) respected`,
  });

  nodes.push({
    path: '/proc/sys/vm/vfs_cache_pressure',
    subsystem: 'VM / Sysctl',
    baselineValue: stockVfsStr,
    currentValue: activeVfsStr,
    status: !procOk
      ? 'SKIPPED'
      : activeVfsStr !== stockVfsStr
        ? policyState === RuntimeState.Pressure
          ? 'CLAMPED'
          : 'MUTATED'
        : 'BASELINE',
    domainEnvelope: '[10..500]',
    rationale:
      policyState === RuntimeState.Pressure && mutationArmed
        ? `Demand-scaled dentry/inode reclaim (${activeVfsStr}) proportional to RAM deficit`
        : 'Kernel domain [10..500] VFS slab cache balance with durable rollback journal',
  });

  nodes.push({
    path: '/proc/sys/vm/page-cluster',
    subsystem: 'VM / Sysctl',
    baselineValue: stockPageClusterStr,
    currentValue: procOk ? stockPageClusterStr : 'SKIPPED',
    status: !procOk ? 'SKIPPED' : 'BASELINE',
    domainEnvelope: '[0..8] (2^N pages)',
    rationale: 'Single-page random ZRAM read (0 = 1 page) preserved to prevent swap latency spikes',
  });

  // 5. GPU & KGSL / Devfreq OPP Topology
  const gt = profile.gpuTopology;
  const gpuReady = sysOk && (gt.governorPath.includes('kgsl') ? capabilities.kgslAvailable : true);
  nodes.push({
    path: gt.governorPath,
    subsystem: 'GPU & DSP',
    baselineValue: gt.stockGovernor,
    currentValue: gpuReady ? gt.stockGovernor : 'SKIPPED',
    status: !gpuReady ? 'SKIPPED' : 'BASELINE',
    domainEnvelope: gt.availableGovernors.join(' | '),
    rationale: gpuReady
      ? `Hardware GPU devfreq governor (${gt.oppTableMhz[0]}..${gt.siliconPeakClockMhz} MHz)`
      : 'GPU sysfs tree absent or disabled in capability matrix; skipped',
  });

  if (gt.minPwrlevelPath != null && gt.oemDefaultPwrlevel != null && gt.numPwrlevels != null) {
    const stockPwrStr = String(gt.oemDefaultPwrlevel);
    const maxSafeFloorStep = Math.max(1, Math.floor((gt.numPwrlevels - 1) * 0.55));
    const gpuFloorDrive =
      !gpuReady || isThermalStabilizing || mode !== EngineMode.ModeGaming
        ? 0
        : clamp01(latencyDrive * 0.65 + (sample.gpuBusyPct / 100) * 0.35);
    const activePwrNum = Math.max(
      gt.oemDefaultPwrlevel - maxSafeFloorStep,
      Math.round(gt.oemDefaultPwrlevel - maxSafeFloorStep * gpuFloorDrive)
    );
    const activePwrStr = String(activePwrNum);
    const floorClockMhz =
      gt.oppTableMhz[Math.min(gt.oppTableMhz.length - 1, gt.oppTableMhz.length - 1 - activePwrNum)] ??
      gt.oppTableMhz[0];

    nodes.push({
      path: gt.minPwrlevelPath,
      subsystem: 'GPU & DSP',
      baselineValue: stockPwrStr,
      currentValue: gpuReady ? activePwrStr : 'SKIPPED',
      status: !gpuReady ? 'SKIPPED' : activePwrStr !== stockPwrStr ? 'MUTATED' : 'BASELINE',
      domainEnvelope: `KGSL [0=${gt.siliconPeakClockMhz}MHz .. ${gt.oemDefaultPwrlevel}=${gt.oppTableMhz[0]}MHz]`,
      rationale: !gpuReady
        ? 'KGSL power level interface unavailable; skipped'
        : activePwrStr !== stockPwrStr
          ? `Event-driven gaming OPP floor (${floorClockMhz} MHz, pwrlevel ${activePwrStr}); reverts to ${stockPwrStr} on exit`
          : `OEM idle GPU floor (pwrlevel ${stockPwrStr} / ${gt.oppTableMhz[0]} MHz) preserved for zero idle drain`,
    });
  }

  // 6. Encore-Inspired Low-Latency TCP Congestion Control
  const net = profile.network;
  const preferredLowLatCongestion = net.availableCongestion.find(
    (alg) => alg === 'bbr' || alg === 'bbr2' || alg === 'westwood'
  );
  const activeCongestion =
    !procOk
      ? 'SKIPPED'
      : latencyDrive >= 0.35 && preferredLowLatCongestion
        ? preferredLowLatCongestion
        : net.stockCongestion;

  nodes.push({
    path: net.congestionPath,
    subsystem: 'Network',
    baselineValue: net.stockCongestion,
    currentValue: activeCongestion,
    status: !procOk
      ? 'SKIPPED'
      : activeCongestion !== net.stockCongestion
        ? 'MUTATED'
        : 'BASELINE',
    domainEnvelope: net.availableCongestion.join(' | '),
    rationale: !procOk
      ? 'Network sysctl read-only; skipped'
      : activeCongestion !== net.stockCongestion
        ? `Selects kernel-advertised "${activeCongestion}" pacing for low-jitter interactive sessions`
        : `OEM default TCP congestion ("${net.stockCongestion}") preserved`,
  });

  // 7. Fatal-Surface / Unvalidated Charging Guard
  nodes.push({
    path: profile.charging.chargeControlLimitPath,
    subsystem: 'Charging',
    baselineValue: String(profile.charging.chargeControlLimitValue),
    currentValue:
      sysOk && capabilities.chargeControlNodePresent && profile.charging.hasChargeControlLimit
        ? `${profile.charging.chargeControlLimitValue} (READ-ONLY)`
        : 'SKIPPED',
    status: 'SKIPPED',
    domainEnvelope: profile.charging.hasChargeControlLimit
      ? `[${profile.charging.chargeControlLimitMin}..${profile.charging.chargeControlLimitMax}]`
      : 'absent',
    rationale:
      capabilities.chargeControlNodePresent && profile.charging.hasChargeControlLimit
        ? 'Fatal-surface / unvalidated charging semantics: read-only by safety contract'
        : 'Node absent on this power_supply driver; skipped',
  });

  return nodes;
}

// Frozen 18-feature contract matching include/coreflow/thermal_features.hpp
// and tools/train_coreflow_thermal_predictor_v1_2_18f.py
export function computeThermalPrediction(
  sample: RuntimeSample,
  history: RuntimeSample[],
  monitorIntervalSec: number
): ThermalPrediction {
  const prevSample = history.length > 0 ? history[history.length - 1] : sample;
  const currentTempC = sample.thermalMillidegrees / 1000;
  const hottestTempC = sample.hottestThermalMillidegrees / 1000;
  const spreadTempC = Math.max(0, Number((hottestTempC - currentTempC).toFixed(2)));
  const prevTempC = prevSample.thermalMillidegrees / 1000;
  const thermalDeltaC = Number((currentTempC - prevTempC).toFixed(2));
  const uptimeDeltaS = Math.max(
    1,
    sample.uptimeSeconds - prevSample.uptimeSeconds || monitorIntervalSec
  );

  const battTempC = sample.batteryTempMillidegrees / 1000;
  const battCurrentA = Number((sample.batteryCurrentMicroamps / 1_000_000).toFixed(3));
  const battVoltageV = Number((sample.batteryVoltageMicrovolts / 1_000_000).toFixed(3));
  const ioTotalKbs = Number((sample.ioReadKbs + sample.ioWriteKbs).toFixed(1));

  const tTrendVal = encodeTrendValue(sample.thermalTrend);
  const mTrendVal = encodeTrendValue(sample.memoryTrend);
  const lTrendVal = encodeTrendValue(sample.loadTrend);

  const vectorComplete =
    sample.thermalAvailable &&
    sample.chargingTelemetryAvailable &&
    history.length >= 3;

  const features: PredictorFeatureRow[] = [
    {
      index: 0,
      name: 'cpu_utilization',
      rawValue: Number((sample.cpuUsagePct / 100).toFixed(3)),
      formattedValue: `${sample.cpuUsagePct.toFixed(1)}%`,
      description: 'Active CPU utilization ratio (/proc/stat)',
    },
    {
      index: 1,
      name: 'load1',
      rawValue: sample.load1,
      formattedValue: sample.load1.toFixed(2),
      description: '1-minute runnable queue load average',
    },
    {
      index: 2,
      name: 'mem_available_ratio',
      rawValue: Number(sample.memAvailableRatio.toFixed(4)),
      formattedValue: `${(sample.memAvailableRatio * 100).toFixed(1)}%`,
      description: 'MemAvailable / MemTotal from /proc/meminfo',
    },
    {
      index: 3,
      name: 'thermal_current_c',
      rawValue: Number(currentTempC.toFixed(2)),
      formattedValue: `${currentTempC.toFixed(2)} °C`,
      description: 'Primary SoC/CPU thermal zone reading',
    },
    {
      index: 4,
      name: 'thermal_delta_c',
      rawValue: thermalDeltaC,
      formattedValue: `${thermalDeltaC >= 0 ? '+' : ''}${thermalDeltaC.toFixed(2)} °C`,
      description: 'Per-tick temperature delta',
    },
    {
      index: 5,
      name: 'charging',
      rawValue: sample.charging ? 1.0 : 0.0,
      formattedValue: sample.charging ? '1.0 (YES)' : '0.0 (NO)',
      description: 'Active charging current state',
    },
    {
      index: 6,
      name: 'battery_temperature_c',
      rawValue: Number(battTempC.toFixed(2)),
      formattedValue: `${battTempC.toFixed(2)} °C`,
      description: 'Battery pack thermistor temperature',
    },
    {
      index: 7,
      name: 'battery_current_a',
      rawValue: battCurrentA,
      formattedValue: `${battCurrentA >= 0 ? '+' : ''}${battCurrentA.toFixed(2)} A`,
      description: 'Instantaneous charge/discharge current',
    },
    {
      index: 8,
      name: 'battery_voltage_v',
      rawValue: battVoltageV,
      formattedValue: `${battVoltageV.toFixed(3)} V`,
      description: 'Battery terminal voltage',
    },
    {
      index: 9,
      name: 'uptime_delta_s',
      rawValue: uptimeDeltaS,
      formattedValue: `${uptimeDeltaS} s`,
      description: 'Monotonic seconds elapsed since previous sample',
    },
    {
      index: 10,
      name: 'thermal_trend',
      rawValue: tTrendVal,
      formattedValue: `${tTrendVal.toFixed(1)} (${sample.thermalTrend})`,
      description: 'Deadband trend encoding (+/- 0.5°C)',
    },
    {
      index: 11,
      name: 'memory_trend',
      rawValue: mTrendVal,
      formattedValue: `${mTrendVal.toFixed(1)} (${sample.memoryTrend})`,
      description: 'Deadband trend encoding (+/- 1.5% RAM)',
    },
    {
      index: 12,
      name: 'load_trend',
      rawValue: lTrendVal,
      formattedValue: `${lTrendVal.toFixed(1)} (${sample.loadTrend})`,
      description: 'Deadband trend encoding (+/- 0.20 load1)',
    },
    {
      index: 13,
      name: 'runtime_confidence',
      rawValue: sample.confidence,
      formattedValue: sample.confidence.toFixed(2),
      description: 'Weighted telemetry completeness score [0..1]',
    },
    {
      index: 14,
      name: 'hottest_thermal_c',
      rawValue: Number(hottestTempC.toFixed(2)),
      formattedValue: `${hottestTempC.toFixed(2)} °C`,
      description: 'Peak reading across all plausible thermal zones',
    },
    {
      index: 15,
      name: 'thermal_spread_c',
      rawValue: spreadTempC,
      formattedValue: `${spreadTempC.toFixed(2)} °C`,
      description: 'Hotspot gradient (hottest_thermal_c - thermal_current_c)',
    },
    {
      index: 16,
      name: 'io_total_kb_s',
      rawValue: ioTotalKbs,
      formattedValue: `${ioTotalKbs.toFixed(0)} KB/s`,
      description: 'Combined block read + write throughput',
    },
    {
      index: 17,
      name: 'battery_level_percent',
      rawValue: sample.batteryPercent,
      formattedValue: `${sample.batteryPercent}%`,
      description: 'Reported state of charge percentage',
    },
  ];

  const cpuHeatContribution = Math.max(0, (sample.cpuUsagePct - 35) * 0.025);
  const chargeHeatContribution = sample.charging ? 0.45 : -0.10;
  const spreadContribution = spreadTempC * 0.12;
  const momentumContribution = thermalDeltaC * 1.5 + (tTrendVal > 0.5 ? 0.35 : 0);
  const ambientCooling = currentTempC > 34 ? -0.05 * (currentTempC - 34) : 0;

  const deltaHorizonC = Number(
    (
      cpuHeatContribution +
      chargeHeatContribution +
      spreadContribution +
      momentumContribution +
      ambientCooling
    ).toFixed(2)
  );
  const predictedTempC = Number(Math.max(22, currentTempC + deltaHorizonC).toFixed(2));
  const guardEnterC = THERMAL_GUARD_ENTER_MC / 1000;
  const preemptiveTrigger = predictedTempC >= guardEnterC && currentTempC < guardEnterC;

  return {
    features,
    vectorComplete,
    horizonSeconds: monitorIntervalSec * 3,
    horizonTicks: 3,
    predictedTempC,
    deltaHorizonC,
    preemptiveTrigger,
    predictorMode: vectorComplete ? 'ONNX_18F_EVALUATED' : 'HEURISTIC_FALLBACK',
  };
}

export function computeSampleEfficiencyScore(
  sample: RuntimeSample,
  candidateGovernor: string | null,
  injectRegression: boolean
): number {
  const tempC = sample.thermalMillidegrees / 1000;
  const workDelivered = sample.cpuUsagePct * 0.55 + sample.gpuBusyPct * 0.45 + 15;
  const thermalCost = Math.max(1, (tempC - 26) * 1.45);
  const loadOverhead = 1 + sample.load1 * 0.14;

  let baseScore = (workDelivered / (thermalCost * loadOverhead)) * 22;

  if (candidateGovernor) {
    if (injectRegression) {
      baseScore *= 0.84;
    } else if (candidateGovernor === 'walt' || candidateGovernor === 'schedutil') {
      baseScore *= 1.09;
    } else if (candidateGovernor === 'performance') {
      baseScore *= 0.89;
    } else if (candidateGovernor === 'conservative') {
      baseScore *= 1.01;
    } else if (candidateGovernor === 'powersave') {
      baseScore *= sample.cpuUsagePct < 25 ? 1.11 : 0.91;
    }
  }

  return Number(Math.min(99.9, Math.max(10.0, baseScore)).toFixed(2));
}

export function advanceBaselineExperiment(
  exp: BaselineExperimentState,
  sample: RuntimeSample
): { next: BaselineExperimentState; logMessage?: string } {
  if (!exp.active) return { next: exp };

  if (exp.action === BaselineAction.CollectingBaseline) {
    const score = computeSampleEfficiencyScore(sample, null, false);
    const baselineSamples = [
      ...exp.baselineSamples,
      {
        tick: sample.sampleIndex,
        efficiencyScore: score,
        cpuUsagePct: sample.cpuUsagePct,
        tempC: Number((sample.thermalMillidegrees / 1000).toFixed(2)),
        load1: sample.load1,
      },
    ];

    if (baselineSamples.length < 4) {
      return {
        next: {
          ...exp,
          baselineSamples,
          summary: `Capturing pre-mutation baseline (${baselineSamples.length}/4 samples)...`,
        },
      };
    }

    const mean =
      baselineSamples.reduce((acc, item) => acc + item.efficiencyScore, 0) /
      baselineSamples.length;

    return {
      next: {
        ...exp,
        baselineSamples,
        baselineMeanScore: Number(mean.toFixed(2)),
        action: BaselineAction.ObservingMutation,
        summary: `Factory baseline locked (${mean.toFixed(2)} pts). Verifying "${exp.candidateGovernor}" (0/4 samples)...`,
      },
      logMessage: `EFFICIENCY_OBSERVATION_BEGIN baseline_score=${mean.toFixed(2)} candidate_governor=${exp.candidateGovernor}`,
    };
  }

  if (exp.action === BaselineAction.ObservingMutation) {
    const score = computeSampleEfficiencyScore(
      sample,
      exp.candidateGovernor,
      exp.injectRegression
    );
    const mutatedSamples = [
      ...exp.mutatedSamples,
      {
        tick: sample.sampleIndex,
        efficiencyScore: score,
        cpuUsagePct: sample.cpuUsagePct,
        tempC: Number((sample.thermalMillidegrees / 1000).toFixed(2)),
        load1: sample.load1,
      },
    ];

    if (mutatedSamples.length < 4) {
      return {
        next: {
          ...exp,
          mutatedSamples,
          summary: `Observing verified mutation "${exp.candidateGovernor}" (${mutatedSamples.length}/4 samples)...`,
        },
      };
    }

    const mutMean =
      mutatedSamples.reduce((acc, item) => acc + item.efficiencyScore, 0) /
      mutatedSamples.length;
    const baseMean = exp.baselineMeanScore ?? mutMean;
    const deltaPct = baseMean > 0 ? ((mutMean - baseMean) / baseMean) * 100 : 0;

    if (sample.confidence < 0.6) {
      return {
        next: {
          ...exp,
          active: false,
          mutatedSamples,
          mutatedMeanScore: Number(mutMean.toFixed(2)),
          deltaPct: Number(deltaPct.toFixed(2)),
          outcome: BaselineOutcome.Inconclusive,
          action: BaselineAction.Hold,
          summary: 'Confidence < 0.60; insufficient telemetry evidence to retain mutation.',
        },
        logMessage: `EFFICIENCY_OUTCOME result=INCONCLUSIVE confidence=${sample.confidence.toFixed(2)} action=HOLD`,
      };
    }

    if (deltaPct >= 3.0) {
      return {
        next: {
          ...exp,
          mutatedSamples,
          mutatedMeanScore: Number(mutMean.toFixed(2)),
          deltaPct: Number(deltaPct.toFixed(2)),
          outcome: BaselineOutcome.Beneficial,
          action: BaselineAction.Keep,
          summary: `Measured +${deltaPct.toFixed(1)}% efficiency gain. EFF_BENEFICIAL action=RETAINED.`,
        },
        logMessage: `EFFICIENCY_BENEFICIAL action=RETAINED delta=+${deltaPct.toFixed(2)}% governor=${exp.candidateGovernor}`,
      };
    }

    if (deltaPct <= -3.0) {
      return {
        next: {
          ...exp,
          active: false,
          mutatedSamples,
          mutatedMeanScore: Number(mutMean.toFixed(2)),
          deltaPct: Number(deltaPct.toFixed(2)),
          outcome: BaselineOutcome.Regression,
          action: BaselineAction.Restore,
          summary: `Measured ${deltaPct.toFixed(1)}% regression. Rolled back via MutationJournal to OEM baseline.`,
        },
        logMessage: `EFFICIENCY_REGRESSION delta=${deltaPct.toFixed(2)}% ACTION=ROLLED_BACK restored=FACTORY_BASELINE`,
      };
    }

    return {
      next: {
        ...exp,
        mutatedSamples,
        mutatedMeanScore: Number(mutMean.toFixed(2)),
        deltaPct: Number(deltaPct.toFixed(2)),
        outcome: BaselineOutcome.Neutral,
        action: BaselineAction.Hold,
        summary: `Efficiency delta (${deltaPct >= 0 ? '+' : ''}${deltaPct.toFixed(1)}%) within hysteresis band. Holding state.`,
      },
      logMessage: `EFFICIENCY_OUTCOME result=NEUTRAL delta=${deltaPct.toFixed(2)}% action=HOLD`,
    };
  }

  return { next: exp };
}

export function formatStartupLogs(
  profile: DeviceHardwareProfile,
  capabilities: CapabilityOverrides,
  timeFormatted: string
): LogcatEntry[] {
  const c = profile.charging;
  const hasCtrl = capabilities.chargeControlNodePresent && c.hasChargeControlLimit;
  const nonce = `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;

  return [
    {
      id: `boot-1-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAutonomous',
      message: `CoreFlow Autonomous v3.1.2 | abi=${profile.abi} kernel=${profile.kernelRelease} vm.swappiness=${profile.stockSwappiness} zram=${profile.zramSizeMb}MB discovered=${profile.discoveredResources} ready=${profile.mutationReadyResources}`,
    },
    {
      id: `boot-2-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowConscience',
      message: `SafetyFoundation verified: mutation_journal=READY fatal_surface=ENFORCED automatic_mode=ADAPTIVE guard_enter=55.0C guard_exit=52.0C`,
    },
    {
      id: `boot-3-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAutonomous',
      message: hasCtrl
        ? `ChargingControlValidation path=${c.chargeControlLimitPath} range=[${c.chargeControlLimitMin}..${c.chargeControlLimitMax}] semantics_validated=NO mutation_ready=NO`
        : 'ChargingControlValidation available=NO',
    },
  ];
}

export function exportHistoryToCsv(history: RuntimeSample[]): string {
  const headers = [
    'Sample',
    'Uptime(s)',
    'CPU(%)',
    'GPU_Busy(%)',
    'Load1',
    'Mem_Total(KB)',
    'Mem_Available(KB)',
    'Mem_Available_Ratio',
    'Thermal_Available',
    'Thermal(°C)',
    'Hottest_Thermal(°C)',
    'Thermal_Source',
    'Thermal_Trend',
    'Mem_Trend',
    'Load_Trend',
    'Charging',
    'Battery(%)',
    'Battery_Temp(°C)',
    'Battery_Current(mA)',
    'Battery_Voltage(V)',
    'IO_Read(KB/s)',
    'IO_Write(KB/s)',
    'Confidence',
  ];

  const escapeCsv = (val: unknown): string => {
    if (val === null || val === undefined) return '';
    const str = String(val);
    if (str.includes(',') || str.includes('"') || str.includes('\n') || str.includes('\r')) {
      return `"${str.replace(/"/g, '""')}"`;
    }
    return str;
  };

  const rows = history.map((s) => [
    s.sampleIndex,
    s.uptimeSeconds,
    s.cpuUsagePct.toFixed(1),
    s.gpuBusyPct.toFixed(1),
    s.load1.toFixed(2),
    s.memTotalKb,
    s.memAvailableKb,
    s.memAvailableRatio.toFixed(3),
    s.thermalAvailable ? 'true' : 'false',
    (s.thermalMillidegrees / 1000).toFixed(1),
    (s.hottestThermalMillidegrees / 1000).toFixed(1),
    s.thermalSource,
    s.thermalTrend,
    s.memoryTrend,
    s.loadTrend,
    s.charging ? 'true' : 'false',
    s.batteryPercent,
    (s.batteryTempMillidegrees / 1000).toFixed(1),
    (s.batteryCurrentMicroamps / 1000).toFixed(1),
    (s.batteryVoltageMicrovolts / 1000000).toFixed(3),
    s.ioReadKbs.toFixed(1),
    s.ioWriteKbs.toFixed(1),
    s.confidence.toFixed(2),
  ]);

  return [headers.join(','), ...rows.map((r) => r.map(escapeCsv).join(','))].join('\n');
}
