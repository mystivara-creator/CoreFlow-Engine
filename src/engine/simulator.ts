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
    ? Math.round((effectiveSocTempC + 1.4) * 1000)
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
    batteryTempMillidegrees: chargingTelemetryAvailable ? Math.round(effectiveBattTempC * 1000) : 0,
    batteryCurrentMicroamps: chargingTelemetryAvailable ? inputs.batteryCurrentMa * 1000 : 0,
    batteryVoltageMicrovolts: chargingTelemetryAvailable ? inputs.batteryVoltageMv * 1000 : 0,
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
  experiment: BaselineExperimentState
): SysfsNodeEntry[] {
  const sysOk = capabilities.sysAvailable;
  const procOk = capabilities.procAvailable && capabilities.vmSysctlWritable;

  const isHighPerf =
    (mode === EngineMode.ModeGaming || mode === EngineMode.ModeBurst) &&
    policyState !== RuntimeState.ThermalGuard &&
    policyState !== RuntimeState.Pressure;

  const batteryTempC = sample.batteryTempMillidegrees / 1000;
  const thermalCoolingClamp =
    policyState === RuntimeState.ThermalGuard || sample.thermalMillidegrees >= 43_000;
  const smartChargingClamp = sample.charging && batteryTempC > 40.0;

  const stockRa = String(profile.stockReadAheadKb);
  const stockNr = String(profile.stockNrRequests);

  let activeRa = stockRa;
  let activeNr = stockNr;
  if (mode === EngineMode.ModeUltraIdle) {
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
    mode === EngineMode.ModeUltraIdle
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
  } else if (mode === EngineMode.ModeIdle || mode === EngineMode.ModeUltraIdle) {
    waltP0 = '85';
    waltP4 = '85';
  }

  const p0MaxStock = String(p0.maxFreqKhz);
  const p4MaxStock = String(p4.maxFreqKhz);
  const p0MaxActive = thermalCoolingClamp ? '1400000' : p0MaxStock;
  const p4MaxActive = thermalCoolingClamp || smartChargingClamp ? '1800000' : p4MaxStock;

  const nodes: SysfsNodeEntry[] = [
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/scaling_governor',
      subsystem: 'CPU & WALT',
      baselineValue: p0.stockGovernor,
      currentValue: sysOk ? p0Gov : 'N/A',
      status: !sysOk ? 'SKIPPED' : p0Gov !== p0.stockGovernor ? 'MUTATED' : 'BASELINE',
      rationale: expGovernorActive
        ? `Baseline Intelligence testing candidate governor "${p0Gov}"`
        : 'Preserves OEM efficiency cluster scaling governor',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/scaling_governor',
      subsystem: 'CPU & WALT',
      baselineValue: p4.stockGovernor,
      currentValue: sysOk ? p4Gov : 'N/A',
      status: !sysOk ? 'SKIPPED' : p4Gov !== p4.stockGovernor ? 'MUTATED' : 'BASELINE',
      rationale:
        mode === EngineMode.ModeUltraIdle
          ? 'Screen-off Ultra Deep Sleep switches performance cluster to powersave'
          : expGovernorActive
            ? `Baseline Intelligence testing candidate governor "${p4Gov}"`
            : 'Preserves OEM performance cluster scaling governor',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/walt_target_load',
      subsystem: 'CPU & WALT',
      baselineValue: '80',
      currentValue: sysOk && capabilities.waltAvailable ? waltP0 : 'SKIPPED',
      status: !sysOk || !capabilities.waltAvailable ? 'SKIPPED' : waltP0 !== '80' ? 'MUTATED' : 'BASELINE',
      rationale: capabilities.waltAvailable
        ? 'WALT scheduler target load threshold for efficiency cores'
        : 'Kernel does not expose WALT tunables; safely skipped',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/walt_target_load',
      subsystem: 'CPU & WALT',
      baselineValue: '80',
      currentValue: sysOk && capabilities.waltAvailable ? waltP4 : 'SKIPPED',
      status: !sysOk || !capabilities.waltAvailable ? 'SKIPPED' : waltP4 !== '80' ? 'MUTATED' : 'BASELINE',
      rationale: capabilities.waltAvailable
        ? 'Lowered to 55/70 during Gaming/Burst for faster frequency ramp'
        : 'Kernel does not expose WALT tunables; safely skipped',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq',
      subsystem: 'CPU & WALT',
      baselineValue: p0MaxStock,
      currentValue: sysOk ? p0MaxActive : 'N/A',
      status: !sysOk ? 'SKIPPED' : p0MaxActive !== p0MaxStock ? 'CLAMPED' : 'BASELINE',
      rationale: thermalCoolingClamp
        ? 'Thermal Guardian active (>=43°C): clamped to 1.40 GHz'
        : 'Uncapped factory maximum cluster frequency',
    },
    {
      path: '/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq',
      subsystem: 'CPU & WALT',
      baselineValue: p4MaxStock,
      currentValue: sysOk ? p4MaxActive : 'N/A',
      status: !sysOk ? 'SKIPPED' : p4MaxActive !== p4MaxStock ? 'CLAMPED' : 'BASELINE',
      rationale: thermalCoolingClamp
        ? 'Thermal Guardian active (>=43°C): clamped to 1.80 GHz'
        : smartChargingClamp
          ? 'Smart Charging Guardian (>40°C while charging): clamped to 1.80 GHz'
          : 'Uncapped factory maximum cluster frequency',
    },
    {
      path: '/sys/block/sda/queue/scheduler',
      subsystem: 'Block I/O',
      baselineValue: 'none',
      currentValue: sysOk ? 'mq-deadline' : 'N/A',
      status: !sysOk ? 'SKIPPED' : 'MUTATED',
      rationale: 'Enforces deterministic multi-queue deadline scheduling on UFS sda',
    },
    {
      path: '/sys/block/sda..sdf/queue/read_ahead_kb',
      subsystem: 'Block I/O',
      baselineValue: stockRa,
      currentValue: sysOk ? activeRa : 'N/A',
      status: !sysOk ? 'SKIPPED' : activeRa !== stockRa ? 'MUTATED' : 'BASELINE',
      rationale: `Adaptive UFS read-ahead (${activeRa} KB for ${mode})`,
    },
    {
      path: '/sys/block/sda..sdf/queue/nr_requests',
      subsystem: 'Block I/O',
      baselineValue: stockNr,
      currentValue: sysOk ? activeNr : 'N/A',
      status: !sysOk ? 'SKIPPED' : activeNr !== stockNr ? 'MUTATED' : 'BASELINE',
      rationale: 'Queue depth scaled only during verified Burst/Gaming demand',
    },
    {
      path: '/sys/block/sda..sdf/queue/rq_affinity',
      subsystem: 'Block I/O',
      baselineValue: '1',
      currentValue: sysOk ? '2' : 'N/A',
      status: !sysOk ? 'SKIPPED' : 'MUTATED',
      rationale: 'Completes I/O on the exact originating CPU core for L1/L2 cache locality',
    },
    {
      path: '/sys/block/sda..sdf/queue/iostats',
      subsystem: 'Block I/O',
      baselineValue: '1',
      currentValue: sysOk ? '0' : 'N/A',
      status: !sysOk ? 'SKIPPED' : 'MUTATED',
      rationale: 'Eliminates per-request block accounting overhead on UFS queues',
    },
    {
      path: '/sys/block/sda..sdf/queue/iosched/read_expire',
      subsystem: 'Block I/O',
      baselineValue: '500',
      currentValue: sysOk ? (isHighPerf ? '250' : '500') : 'N/A',
      status: !sysOk ? 'SKIPPED' : isHighPerf ? 'MUTATED' : 'BASELINE',
      rationale: 'Halves read expiration latency to 250ms under Burst/Gaming',
    },
    {
      path: '/sys/block/sda..sdf/queue/iosched/async_depth',
      subsystem: 'Block I/O',
      baselineValue: '64',
      currentValue: sysOk ? (isHighPerf ? '128' : '64') : 'N/A',
      status: !sysOk ? 'SKIPPED' : isHighPerf ? 'MUTATED' : 'BASELINE',
      rationale: 'Expands asynchronous dispatch depth during asset streaming',
    },
    {
      path: '/proc/sys/vm/vfs_cache_pressure',
      subsystem: 'VM / Sysctl',
      baselineValue: '100',
      currentValue: procOk ? (isHighPerf ? '100' : '150') : 'SKIPPED',
      status: !procOk ? 'SKIPPED' : isHighPerf ? 'BASELINE' : 'MUTATED',
      rationale: isHighPerf
        ? 'Retains dentry/inode cache at 100 to prevent asset reload stutter'
        : 'Reclaims VFS slab cache faster (150) during idle/balanced operation',
    },
    {
      path: '/proc/sys/vm/dirty_background_ratio',
      subsystem: 'VM / Sysctl',
      baselineValue: '10',
      currentValue: procOk ? (isHighPerf ? '20' : '10') : 'SKIPPED',
      status: !procOk ? 'SKIPPED' : isHighPerf ? 'MUTATED' : 'BASELINE',
      rationale: 'Relaxes background writeback threshold under heavy frame rendering',
    },
    {
      path: '/proc/sys/vm/dirty_ratio',
      subsystem: 'VM / Sysctl',
      baselineValue: '20',
      currentValue: procOk ? (isHighPerf ? '30' : '20') : 'SKIPPED',
      status: !procOk ? 'SKIPPED' : isHighPerf ? 'MUTATED' : 'BASELINE',
      rationale: 'Prevents synchronous dirty-page stalls during burst writes',
    },
    {
      path: '/proc/sys/vm/page-cluster',
      subsystem: 'VM / Sysctl',
      baselineValue: '3',
      currentValue: procOk ? '3' : 'SKIPPED',
      status: !procOk ? 'SKIPPED' : 'BASELINE',
      rationale: 'Maintains 8-page swap read-ahead cluster for ZRAM decompression',
    },
    {
      path: '/sys/class/kgsl/kgsl-3d0/devfreq/governor',
      subsystem: 'GPU & DSP',
      baselineValue: 'msm-adreno-tz',
      currentValue: sysOk && capabilities.kgslAvailable ? 'msm-adreno-tz' : 'SKIPPED',
      status: !sysOk || !capabilities.kgslAvailable ? 'SKIPPED' : 'BASELINE',
      rationale: capabilities.kgslAvailable
        ? 'Qualcomm KGSL Adreno TrustZone governor active'
        : 'Non-KGSL GPU architecture; falls back to CPU/load signals cleanly',
    },
    {
      path: 'vendor.dsp.default_qos',
      subsystem: 'GPU & DSP',
      baselineValue: '1',
      currentValue: mode === EngineMode.ModeUltraIdle ? '0' : '1',
      status: mode === EngineMode.ModeUltraIdle ? 'MUTATED' : 'BASELINE',
      rationale:
        mode === EngineMode.ModeUltraIdle
          ? 'Suspends Hexagon DSP/NPU default QoS vote while display is off'
          : 'Keeps DSP QoS vote enabled for active user session',
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
          ? 'Structurally valid [min..max], but semantics_validated=NO; mutation blocked by safety contract'
          : 'Node absent on this power_supply driver; skipped',
    },
  ];

  return nodes;
}

export function computeThermalPrediction(
  sample: RuntimeSample,
  history: RuntimeSample[],
  monitorIntervalSec: number
): ThermalPrediction {
  const prevSample = history.length > 0 ? history[history.length - 1] : sample;
  const currentTempC = sample.thermalMillidegrees / 1000;
  const prevTempC = prevSample.thermalMillidegrees / 1000;
  const thermalDeltaC = Number((currentTempC - prevTempC).toFixed(2));
  const uptimeDeltaS = Math.max(
    1,
    sample.uptimeSeconds - prevSample.uptimeSeconds || monitorIntervalSec
  );

  const battTempC = sample.batteryTempMillidegrees / 1000;
  const battCurrentA = Number((sample.batteryCurrentMicroamps / 1_000_000).toFixed(3));
  const battVoltageV = Number((sample.batteryVoltageMicrovolts / 1_000_000).toFixed(3));

  const tTrendVal = encodeTrendValue(sample.thermalTrend);
  const mTrendVal = encodeTrendValue(sample.memoryTrend);
  const lTrendVal = encodeTrendValue(sample.loadTrend);

  const features: PredictorFeatureRow[] = [
    {
      index: 0,
      name: 'cpu_utilization',
      rawValue: Number((sample.cpuUsagePct / 100).toFixed(3)),
      formattedValue: `${sample.cpuUsagePct.toFixed(1)}%`,
      description: 'Normalized /proc/stat active CPU jiffies ratio',
    },
    {
      index: 1,
      name: 'load1',
      rawValue: sample.load1,
      formattedValue: sample.load1.toFixed(2),
      description: '1-minute runnable queue load average (/proc/loadavg)',
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
      description: 'Primary SoC/CPU/GPU thermal zone temperature',
    },
    {
      index: 4,
      name: 'thermal_delta_c',
      rawValue: thermalDeltaC,
      formattedValue: `${thermalDeltaC >= 0 ? '+' : ''}${thermalDeltaC.toFixed(2)} °C`,
      description: 'Per-tick temperature rate of change',
    },
    {
      index: 5,
      name: 'charging',
      rawValue: sample.charging ? 1.0 : 0.0,
      formattedValue: sample.charging ? '1.0 (YES)' : '0.0 (NO)',
      description: 'Active charging current state from power_supply',
    },
    {
      index: 6,
      name: 'battery_temperature_c',
      rawValue: Number(battTempC.toFixed(2)),
      formattedValue: `${battTempC.toFixed(2)} °C`,
      description: 'Normalized pack sensor temperature',
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
      description: 'Cell terminal voltage',
    },
    {
      index: 9,
      name: 'uptime_delta_s',
      rawValue: uptimeDeltaS,
      formattedValue: `${uptimeDeltaS} s`,
      description: 'Elapsed monotonic seconds between samples',
    },
    {
      index: 10,
      name: 'thermal_trend',
      rawValue: tTrendVal,
      formattedValue: `${tTrendVal.toFixed(1)} (${sample.thermalTrend})`,
      description: '6-sample deadband trend (+/- 0.5°C)',
    },
    {
      index: 11,
      name: 'memory_trend',
      rawValue: mTrendVal,
      formattedValue: `${mTrendVal.toFixed(1)} (${sample.memoryTrend})`,
      description: '6-sample deadband trend (+/- 1.5% RAM)',
    },
    {
      index: 12,
      name: 'load_trend',
      rawValue: lTrendVal,
      formattedValue: `${lTrendVal.toFixed(1)} (${sample.loadTrend})`,
      description: '6-sample deadband trend (+/- 0.20 load1)',
    },
    {
      index: 13,
      name: 'runtime_confidence',
      rawValue: sample.confidence,
      formattedValue: sample.confidence.toFixed(2),
      description: 'Weighted telemetry completeness score [0..1]',
    },
  ];

  // Heuristic 3-tick (15s) thermal trajectory estimator
  const cpuHeatContribution = Math.max(0, (sample.cpuUsagePct - 35) * 0.025);
  const chargeHeatContribution = sample.charging ? 0.45 : -0.10;
  const momentumContribution = thermalDeltaC * 1.6 + (tTrendVal > 0.5 ? 0.35 : 0);
  const ambientCooling = currentTempC > 34 ? -0.06 * (currentTempC - 34) : 0;

  const deltaHorizonC = Number(
    (cpuHeatContribution + chargeHeatContribution + momentumContribution + ambientCooling).toFixed(2)
  );
  const predictedTempC = Number(Math.max(22, currentTempC + deltaHorizonC).toFixed(2));
  const preemptiveTrigger = predictedTempC >= 43.0 && currentTempC < 43.0;

  return {
    features,
    horizonSeconds: monitorIntervalSec * 3,
    horizonTicks: 3,
    predictedTempC,
    deltaHorizonC,
    preemptiveTrigger,
    predictorMode: 'ONNX_SYNC_CONTRACT',
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
      baseScore *= 0.84; // -16% efficiency regression -> triggers automatic RESTORE
    } else if (candidateGovernor === 'walt' || candidateGovernor === 'schedutil') {
      baseScore *= 1.09; // +9% beneficial efficiency improvement -> KEEP
    } else if (candidateGovernor === 'performance') {
      baseScore *= 0.89; // Excess heat/power under performance governor -> REGRESSION
    } else if (candidateGovernor === 'conservative') {
      baseScore *= 1.01; // Within deadband -> NEUTRAL / HOLD
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

    if (baselineSamples.length < 5) {
      return {
        next: {
          ...exp,
          baselineSamples,
          summary: `Capturing factory baseline (${baselineSamples.length}/5 samples)...`,
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
        summary: `Factory baseline locked (${mean.toFixed(2)} pts). Verifying candidate "${exp.candidateGovernor}" (0/5 samples)...`,
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

    if (mutatedSamples.length < 5) {
      return {
        next: {
          ...exp,
          mutatedSamples,
          summary: `Observing verified mutation "${exp.candidateGovernor}" (${mutatedSamples.length}/5 samples)...`,
        },
      };
    }

    const mutMean =
      mutatedSamples.reduce((acc, item) => acc + item.efficiencyScore, 0) /
      mutatedSamples.length;
    const baseMean = exp.baselineMeanScore ?? mutMean;
    const deltaPct = baseMean > 0 ? ((mutMean - baseMean) / baseMean) * 100 : 0;

    if (sample.confidence < 0.5) {
      return {
        next: {
          ...exp,
          active: false,
          mutatedSamples,
          mutatedMeanScore: Number(mutMean.toFixed(2)),
          deltaPct: Number(deltaPct.toFixed(2)),
          outcome: BaselineOutcome.Inconclusive,
          action: BaselineAction.Hold,
          summary: 'Confidence score < 0.50; insufficient telemetry evidence to commit decision.',
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
          summary: `Measured +${deltaPct.toFixed(1)}% efficiency gain over Factory Baseline. Optimization kept in observation hold.`,
        },
        logMessage: `EFFICIENCY_OUTCOME result=BENEFICIAL delta=+${deltaPct.toFixed(2)}% action=KEEP governor=${exp.candidateGovernor}`,
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
          summary: `Measured ${deltaPct.toFixed(1)}% efficiency regression. Rejected mutation and restored OEM Factory Baseline.`,
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
        summary: `Efficiency delta (${deltaPct >= 0 ? '+' : ''}${deltaPct.toFixed(1)}%) within +/-3.0% hysteresis band. Holding current state.`,
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

  const entries: LogcatEntry[] = [
    {
      id: `boot-1-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAutonomous',
      message: `CoreFlow Autonomous 1.4.1 | abi=${profile.abi} kernel=${profile.kernelRelease} cpu_policies=${profile.cpuPolicies.length} thermal_zones=${profile.thermalZonesCount}`,
    },
    {
      id: `boot-2-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAutonomous',
      message: `ChargingCapability battery=${c.batteryAvailable ? 'YES' : 'NO'} status=${c.statusReadable ? 'YES' : 'NO'} telemetry=${c.telemetryReadable ? 'YES' : 'NO'} input_limit=${c.hasInputCurrentLimit ? 'YES' : 'NO'} charge_current=${c.hasChargeCurrentLimit ? 'YES' : 'NO'} control_limit=${hasCtrl ? 'YES' : 'NO'}`,
    },
    {
      id: `boot-3-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAutonomous',
      message: hasCtrl
        ? `ChargingControlValidation path=${c.chargeControlLimitPath} readable=YES writable=YES numeric=YES value=${c.chargeControlLimitValue} min=YES:${c.chargeControlLimitMin} max=YES:${c.chargeControlLimitMax} range_valid=YES semantics_validated=NO mutation_ready=NO`
        : 'ChargingControlValidation available=NO',
    },
    {
      id: `boot-4-${nonce}`,
      sampleIndex: 0,
      timeFormatted,
      level: 'I',
      tag: 'CoreFlowAI',
      message:
        profile.board === 'parrot'
          ? `Verified target board=${profile.board} (${profile.name}). Activated Kunzite Privilege Profile & snapshotted stock baseline (read_ahead_kb=${profile.stockReadAheadKb}, nr_requests=${profile.stockNrRequests}).`
          : `Initialized Adaptive Universal Profile on board=${profile.board} (${profile.soc}). Snapshotted stock baseline (read_ahead_kb=${profile.stockReadAheadKb}, nr_requests=${profile.stockNrRequests}).`,
    },
  ];

  return entries;
}
