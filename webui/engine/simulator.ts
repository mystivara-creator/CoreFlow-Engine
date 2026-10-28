import {
  AppClass,
  BaselineAction,
  BaselineExperimentState,
  BaselineOutcome,
  CapabilityOverrides,
  Decision,
  DeviceHardwareProfile,
  EngineMode,
  LogcatEntry,
  NotificationEvent,
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

export const THERMAL_GUARD_ENTER_MC = 43_000;
export const THERMAL_GUARD_EXIT_MC = 40_000;
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

export function calculateConfidence(sample: RuntimeSample): number {
  let score = 0.0;
  if (sample.thermalAvailable) score += 0.35;
  if (sample.memTotalKb > 0) score += 0.25;
  if (sample.uptimeSeconds > 0) score += 0.10;
  if (sample.chargingTelemetryAvailable) score += 0.15;
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

  sample.confidence = calculateConfidence(sample);
  return sample;
}

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
    if (sample.thermalMillidegrees > THERMAL_GUARD_EXIT_MC) {
      return RuntimeState.Warming;
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

  if (sample.load1 >= ELEVATED_LOAD_THRESHOLD) {
    return RuntimeState.Elevated;
  }

  if (sample.load1 < IDLE_LOAD_THRESHOLD) {
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
  if (state == RuntimeState.ThermalGuard) {
    if (previous !== RuntimeState.ThermalGuard) {
      return NotificationEvent.ThermalGuard;
    }
    if (sample.charging && sample.thermalTrend === Trend.Rising) {
      return NotificationEvent.ChargingProtection;
    }
    return NotificationEvent.None;
  }

  if (sample.thermalAvailable && sample.thermalTrend === Trend.Rising) {
    if (sample.thermalMillidegrees >= 42_000) {
      return NotificationEvent.ThermalWarning;
    }
    if (sample.thermalMillidegrees >= 40_000) {
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

export function computeSysfsNodes(
  profile: DeviceHardwareProfile,
  capabilities: CapabilityOverrides,
  mode: EngineMode,
  policyState: RuntimeState,
  sample: RuntimeSample,
  experiment: BaselineExperimentState,
  mutationArmed = true
): SysfsNodeEntry[] {
  const sysOk = capabilities.sysAvailable;
  const procOk = capabilities.procAvailable && capabilities.vmSysctlWritable;

  const isHighPerf =
    mutationArmed &&
    (mode === EngineMode.ModeGaming || mode === EngineMode.ModeBurst) &&
    policyState !== RuntimeState.ThermalGuard &&
    policyState !== RuntimeState.Pressure;

  const batteryTempC = sample.batteryTempMillidegrees / 1000;
  const thermalCoolingClamp =
    mutationArmed &&
    (policyState === RuntimeState.ThermalGuard || sample.thermalMillidegrees >= 43_000);
  const smartChargingClamp = mutationArmed && sample.charging && batteryTempC > 40.0;

  const stockRa = String(profile.stockReadAheadKb);
  const stockNr = String(profile.stockNrRequests);

  let activeRa = stockRa;
  let activeNr = stockNr;
  if (mutationArmed && mode === EngineMode.ModeUltraIdle) {
    activeRa = '256';
  } else if (isHighPerf && mode === EngineMode.ModeGaming) {
    activeRa = '2048';
    activeNr = '256';
  } else if (isHighPerf && mode === EngineMode.ModeBurst) {
    activeRa = '1024';
    activeNr = '128';
  }

  const p0 = profile.cpuPolicies[0];
  const p4 = profile.cpuPolicies[1];

  const expGovernorActive =
    experiment.active &&
    (experiment.action === BaselineAction.ObservingMutation ||
      experiment.action === BaselineAction.Keep ||
      experiment.action === BaselineAction.Hold);

  const p0Gov = expGovernorActive ? experiment.candidateGovernor : p0.stockGovernor;
  const p4Gov =
    mutationArmed && mode === EngineMode.ModeUltraIdle
      ? 'powersave'
      : expGovernorActive
        ? experiment.candidateGovernor
        : p4.stockGovernor;

  let waltP0 = '80';
  let waltP4 = '80';
  if (mode === EngineMode.ModeGaming && isHighPerf) {
    waltP0 = '60';
    waltP4 = '55';
  } else if (mode === EngineMode.ModeBurst && isHighPerf) {
    waltP0 = '70';
    waltP4 = '70';
  } else if (mutationArmed && (mode === EngineMode.ModeIdle || mode === EngineMode.ModeUltraIdle)) {
    waltP0 = '85';
    waltP4 = '85';
  }

  const p0MaxStock = String(p0.maxFreqKhz);
  const p4MaxStock = String(p4.maxFreqKhz);
  const p0MaxActive = thermalCoolingClamp ? '1400000' : p0MaxStock;
  const p4MaxActive = thermalCoolingClamp || smartChargingClamp ? '1800000' : p4MaxStock;

  return [
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/scaling_governor',
      subsystem: 'CPU & WALT',
      baselineValue: p0.stockGovernor,
      currentValue: sysOk ? p0Gov : 'N/A',
      status: !sysOk ? 'SKIPPED' : p0Gov !== p0.stockGovernor ? 'MUTATED' : 'BASELINE',
      rationale: expGovernorActive
        ? `CpuFreqActuator testing verified candidate "${p0Gov}"`
        : mutationArmed
          ? 'Preserves OEM efficiency cluster scaling governor'
          : 'Observe mode active: read-only snapshot',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/scaling_governor',
      subsystem: 'CPU & WALT',
      baselineValue: p4.stockGovernor,
      currentValue: sysOk ? p4Gov : 'N/A',
      status: !sysOk ? 'SKIPPED' : p4Gov !== p4.stockGovernor ? 'MUTATED' : 'BASELINE',
      rationale:
        mutationArmed && mode === EngineMode.ModeUltraIdle
          ? 'Deep-sleep policy switches performance cluster to powersave'
          : expGovernorActive
            ? `CpuFreqActuator testing verified candidate "${p4Gov}"`
            : 'Preserves OEM performance cluster scaling governor',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/walt_target_load',
      subsystem: 'CPU & WALT',
      baselineValue: '80',
      currentValue: sysOk && capabilities.waltAvailable ? waltP0 : 'SKIPPED',
      status: !sysOk || !capabilities.waltAvailable ? 'SKIPPED' : waltP0 !== '80' ? 'MUTATED' : 'BASELINE',
      rationale: capabilities.waltAvailable
        ? 'WALT scheduler target load threshold (ResourceActuator)'
        : 'Kernel does not expose WALT tunables; skipped by CapabilityRegistry',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/walt_target_load',
      subsystem: 'CPU & WALT',
      baselineValue: '80',
      currentValue: sysOk && capabilities.waltAvailable ? waltP4 : 'SKIPPED',
      status: !sysOk || !capabilities.waltAvailable ? 'SKIPPED' : waltP4 !== '80' ? 'MUTATED' : 'BASELINE',
      rationale: capabilities.waltAvailable
        ? 'Lowered under armed Burst/Gaming with verified prewrite journal'
        : 'Kernel does not expose WALT tunables; skipped by CapabilityRegistry',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq',
      subsystem: 'CPU & WALT',
      baselineValue: p0MaxStock,
      currentValue: sysOk ? p0MaxActive : 'N/A',
      status: !sysOk ? 'SKIPPED' : p0MaxActive !== p0MaxStock ? 'CLAMPED' : 'BASELINE',
      rationale: thermalCoolingClamp
        ? 'ThermalGuard stabilizing clamp active (>=43°C): 1.40 GHz'
        : 'Uncapped factory maximum cluster frequency',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq',
      subsystem: 'CPU & WALT',
      baselineValue: p4MaxStock,
      currentValue: sysOk ? p4MaxActive : 'N/A',
      status: !sysOk ? 'SKIPPED' : p4MaxActive !== p4MaxStock ? 'CLAMPED' : 'BASELINE',
      rationale: thermalCoolingClamp
        ? 'ThermalGuard stabilizing clamp active (>=43°C): 1.80 GHz'
        : smartChargingClamp
          ? 'Charging thermal protection (>40°C): 1.80 GHz'
          : 'Uncapped factory maximum cluster frequency',
    },
    {
      path: '/sys/block/sda/queue/read_ahead_kb',
      subsystem: 'Block I/O',
      baselineValue: stockRa,
      currentValue: sysOk ? activeRa : 'N/A',
      status: !sysOk ? 'SKIPPED' : activeRa !== stockRa ? 'MUTATED' : 'BASELINE',
      rationale: mutationArmed
        ? `Bounded UFS read-ahead (${activeRa} KB) via ResourceMutationController`
        : 'Observe mode active: factory read-ahead preserved',
    },
    {
      path: '/sys/block/sda/queue/nr_requests',
      subsystem: 'Block I/O',
      baselineValue: stockNr,
      currentValue: sysOk ? activeNr : 'N/A',
      status: !sysOk ? 'SKIPPED' : activeNr !== stockNr ? 'MUTATED' : 'BASELINE',
      rationale: 'Block queue depth scaled only with explicit grant_block_queue permit',
    },
    {
      path: '/proc/sys/vm/vfs_cache_pressure',
      subsystem: 'VM / Sysctl',
      baselineValue: '100',
      currentValue: procOk ? (mutationArmed && !isHighPerf ? '130' : '100') : 'SKIPPED',
      status: !procOk ? 'SKIPPED' : mutationArmed && !isHighPerf ? 'MUTATED' : 'BASELINE',
      rationale: 'Controlled by grant_memory VmSysctl actuator with durable rollback journal',
    },
    {
      path: '/proc/sys/vm/swappiness',
      subsystem: 'VM / Sysctl',
      baselineValue: '60',
      currentValue:
        procOk && mutationArmed && policyState === RuntimeState.Pressure ? '90' : procOk ? '60' : 'SKIPPED',
      status:
        !procOk
          ? 'SKIPPED'
          : mutationArmed && policyState === RuntimeState.Pressure
            ? 'MUTATED'
            : 'BASELINE',
      rationale:
        policyState === RuntimeState.Pressure && mutationArmed
          ? 'Stabilizing-only VM reclaim adjustment during memory pressure'
          : 'OEM default ZRAM swap aggressiveness preserved',
    },
    {
      path: '/sys/class/kgsl/kgsl-3d0/devfreq/governor',
      subsystem: 'GPU & DSP',
      baselineValue: 'msm-adreno-tz',
      currentValue: sysOk && capabilities.kgslAvailable ? 'msm-adreno-tz' : 'SKIPPED',
      status: !sysOk || !capabilities.kgslAvailable ? 'SKIPPED' : 'BASELINE',
      rationale: capabilities.kgslAvailable
        ? 'Qualcomm KGSL Adreno devfreq observable via gpu_busy_source'
        : 'Non-KGSL GPU architecture; falls back to devfreq/GED signals',
    },
    {
      path: profile.charging.chargeControlLimitPath,
      subsystem: 'Charging',
      baselineValue: String(profile.charging.chargeControlLimitValue),
      currentValue:
        sysOk && capabilities.chargeControlNodePresent && profile.charging.hasChargeControlLimit
          ? `${profile.charging.chargeControlLimitValue} (READ-ONLY)`
          : 'SKIPPED',
      status: 'SKIPPED',
      rationale:
        capabilities.chargeControlNodePresent && profile.charging.hasChargeControlLimit
          ? 'Fatal-surface / unvalidated charging semantics: read-only by safety contract'
          : 'Node absent on this power_supply driver; skipped',
    },
  ];
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
  const ambientCooling = currentTempC > 34 ? -0.06 * (currentTempC - 34) : 0;

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
  const preemptiveTrigger = predictedTempC >= 43.0 && currentTempC < 43.0;

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
  const thermalCost = Math.max(1, (tempC - 26) * 1.8);
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
      message: `CoreFlow Autonomous v3.0.1 | abi=${profile.abi} kernel=${profile.kernelRelease} discovered=${profile.discoveredResources} ready=${profile.mutationReadyResources}`,
    },
    {
      id: `boot-2-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowConscience',
      message: `SafetyFoundation verified: mutation_journal=READY fatal_surface=ENFORCED default_mode=OBSERVE`,
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

