import test from 'node:test';
import assert from 'node:assert/strict';
import {
  CentralMutationCoordinatorModel,
  classifyResourcePermission,
  classifyWorkload,
  computeRelativeTunableValue,
  computeVulkanTelemetry,
  ControllerPriority,
  DEFAULT_VULKAN_CONFIG,
  diagnoseBottleneck,
  evaluateAdaptiveFeedback,
  mergeConfigText,
  parseConfigIni,
  readSectionNumberKey,
  readSectionStringKey,
  selectOptimizationObjective,
} from '../autonomousEngine.ts';

test('computeVulkanTelemetry never fabricates API version, extension count, or unsupported hardware features', () => {
  const baseHardware = {
    available: true,
    layerInstalled: true,
    apiVersion: '1.3.280',
    deviceExtensions: 142,
    instanceExtensions: 14,
    hwRayTracing: false,
    hwMeshShaders: false,
    hwVariableRateShading: true,
    hwCooperativeMatrix: false,
    hwRtPipeline: false,
    hwRayQuery: false,
    hwAccelerationStructure: false,
    hwShaderFloat64: false,
    hwIndexTypeUint8: false,
    hwLineRasterization: false,
    hwPushDescriptors: false,
    hwGlobalPriorityQuery: false,
    hwShaderFloatControls2: false,
    hwMaintenance5: true,
    hwMaintenance6: false,
    hwMaintenance7: false,
    hwDescriptorBuffer: false,
    hwGraphicsPipelineLibrary: false,
  };

  for (const mode of ['disabled', 'safe_hardware', 'full_capability_override'] as const) {
    const vk = computeVulkanTelemetry({ ...DEFAULT_VULKAN_CONFIG, mode }, baseHardware);
    assert.equal(vk.apiVersion, '1.3.280');
    assert.equal(vk.effectiveApiVersion, '1.3.280');
    assert.equal(vk.deviceExtensions, 142);
    assert.equal(vk.effectiveDeviceExtensions, 142);
    assert.equal(vk.rayTracing, false);
    assert.equal(vk.meshShaders, false);
    assert.equal(vk.variableRateShading, true);
    assert.equal(vk.cooperativeMatrix, false);
    assert.equal(vk.rtPipeline, false);
    assert.equal(vk.rayQuery, false);
    assert.equal(vk.accelerationStructure, false);
    assert.equal(vk.shaderFloat64, false);
    assert.equal(vk.indexTypeUint8, false);
    assert.equal(vk.lineRasterization, false);
    assert.equal(vk.pushDescriptors, false);
    assert.equal(vk.maxPushDescriptors, 0);
    assert.equal(vk.maintenance5, true);
    assert.equal(vk.maintenance6, false);
    assert.equal(vk.maintenance7, false);
    assert.equal(vk.descriptorBuffer, false);
    assert.equal(vk.graphicsPipelineLibrary, false);
    if (mode === 'full_capability_override') {
      assert.equal(vk.mode, 'safe_hardware');
    }
  }
});

test('13.1 Discovery & Resource Permission Classification distinguishes VerifiedConstraints, WritableButUnsafe, UnknownSemantic, ReadOnly, and IncompleteMetadata', () => {
  const sdaSched = classifyResourcePermission({
    path: '/sys/block/sda/queue/scheduler',
    writable: true,
    rawValue: '[none] mq-deadline kyber bfq',
  });
  assert.equal(sdaSched.permissionClass, 'VerifiedConstraints');
  assert.equal(sdaSched.mutatable, true);

  const zramSched = classifyResourcePermission({
    path: '/sys/block/zram0/queue/scheduler',
    writable: true,
    rawValue: '[none]',
  });
  assert.equal(zramSched.permissionClass, 'WritableButUnsafe');
  assert.equal(zramSched.mutatable, false);

  const customBlock = classifyResourcePermission({
    path: '/sys/block/vendor_custom0/queue/scheduler',
    writable: true,
    rawValue: 'mystery_sched',
  });
  assert.equal(customBlock.permissionClass, 'UnknownSemantic');
  assert.equal(customBlock.mutatable, false);

  const mismatchedGov = classifyResourcePermission({
    path: '/sys/devices/system/cpu/cpufreq/policy0/schedutil/up_rate_limit_us',
    writable: true,
    rawValue: '2000',
    activeGovernor: 'performance',
    targetGovernor: 'schedutil',
  });
  assert.equal(mismatchedGov.permissionClass, 'WritableButUnsafe');
  assert.equal(mismatchedGov.mutatable, false);

  const missingCompanion = classifyResourcePermission({
    path: '/sys/block/sda/queue/iosched/read_expire',
    writable: true,
    rawValue: '500',
    companionNodesComplete: false,
  });
  assert.equal(missingCompanion.permissionClass, 'IncompleteMetadata');
  assert.equal(missingCompanion.mutatable, false);
});

test('13.2 Optimization Feedback & Adaptive Step Sizing classifies IMPROVED, REGRESSED, INCONCLUSIVE, NEUTRAL, and OWNERSHIP_LOST', () => {
  // 1. IMPROVED above noise floor -> RETAIN
  const improved = evaluateAdaptiveFeedback({
    intent: 'IoLatencyRelief',
    beforeIoPsi: 8.5,
    afterIoPsiSamples: [2.1, 1.9, 1.8],
    beforeMemPsi: 0.4,
    afterMemPsiSamples: [0.4, 0.4, 0.4],
    beforeCpuPsi: 6.0,
    afterCpuPsiSamples: [5.2, 5.0, 5.1],
    beforeThermal: 46.0,
    afterThermalSamples: [46.2, 46.5, 46.1],
    ownershipIntact: true,
    currentAdaptiveScale: 0.75,
    consecutiveNonImproved: 1,
  });
  assert.equal(improved.verdict, 'IMPROVED');
  assert.equal(improved.action, 'RETAIN');
  assert.equal(improved.cooldownTicks, 0);
  assert.equal(improved.nextAdaptiveScale, 0.9);

  // 2. REGRESSED -> ROLLBACK + adaptiveScale step-down + exponential cooldown
  const regressed = evaluateAdaptiveFeedback({
    intent: 'IoLatencyRelief',
    beforeIoPsi: 6.0,
    afterIoPsiSamples: [10.5, 11.2, 11.0],
    beforeMemPsi: 0.5,
    afterMemPsiSamples: [0.5, 0.5, 0.5],
    beforeCpuPsi: 6.0,
    afterCpuPsiSamples: [7.0, 7.2, 7.1],
    beforeThermal: 46.0,
    afterThermalSamples: [48.5, 49.0, 49.2],
    ownershipIntact: true,
    currentAdaptiveScale: 1.0,
    consecutiveNonImproved: 0,
  });
  assert.equal(regressed.verdict, 'REGRESSED');
  assert.equal(regressed.action, 'ROLLBACK');
  assert.equal(regressed.nextAdaptiveScale, 0.75);
  assert.equal(regressed.cooldownTicks, 8);

  // Verify adaptive step sizing reduces mutation aggressiveness after rollback
  assert.equal(computeRelativeTunableValue(128, 2.0, 64, 2048, 1.0), 256);
  assert.equal(computeRelativeTunableValue(128, 2.0, 64, 2048, 0.5), 192);

  // 3. High sample variance -> INCONCLUSIVE -> ROLLBACK
  const inconclusive = evaluateAdaptiveFeedback({
    intent: 'IoLatencyRelief',
    beforeIoPsi: 4.0,
    afterIoPsiSamples: [0.5, 9.0, 2.4],
    beforeMemPsi: 0.3,
    afterMemPsiSamples: [0.3, 0.3, 0.3],
    beforeCpuPsi: 4.0,
    afterCpuPsiSamples: [4.0, 4.0, 4.0],
    beforeThermal: 44.0,
    afterThermalSamples: [44.0, 44.0, 44.0],
    ownershipIntact: true,
    currentAdaptiveScale: 1.0,
    consecutiveNonImproved: 0,
  });
  assert.equal(inconclusive.verdict, 'INCONCLUSIVE');
  assert.equal(inconclusive.action, 'ROLLBACK');

  // 4. External writer modified node -> OWNERSHIP_LOST
  const lost = evaluateAdaptiveFeedback({
    intent: 'IoLatencyRelief',
    beforeIoPsi: 6.0,
    afterIoPsiSamples: [2.0, 2.0, 2.0],
    beforeMemPsi: 0.3,
    afterMemPsiSamples: [0.3, 0.3, 0.3],
    beforeCpuPsi: 4.0,
    afterCpuPsiSamples: [4.0, 4.0, 4.0],
    beforeThermal: 44.0,
    afterThermalSamples: [44.0, 44.0, 44.0],
    ownershipIntact: false,
    currentAdaptiveScale: 1.0,
    consecutiveNonImproved: 0,
  });
  assert.equal(lost.verdict, 'OWNERSHIP_LOST');
  assert.equal(lost.action, 'ROLLBACK');
});

test('13.4 Thermal Stabilization & Hysteresis Recovery switches objective to ThermalStabilization instead of halting on non-emergency thermal pressure', () => {
  const constrained = {
    cpuPsi: 11.0,
    memoryPsi: 0.5,
    ioPsi: 1.2,
    thermalPressure: 68.0,
    thermalGuardActive: true,
    thermalEmergency: false,
    thermalHeadroom: 0.15,
    thermalTrendCelsius: 0.9,
  };
  const wl = classifyWorkload(constrained);
  const bn = diagnoseBottleneck(constrained, wl);
  const obj = selectOptimizationObjective(constrained, wl, bn);
  assert.equal(wl, 'ThermalConstrained');
  assert.equal(bn, 'ThermalHeadroomDepletion');
  assert.equal(obj, 'ThermalStabilization');

  // Emergency thermal spike triggers EmergencyMitigation
  const emergency = {
    ...constrained,
    thermalPressure: 92.0,
    thermalEmergency: true,
  };
  assert.equal(selectOptimizationObjective(emergency), 'EmergencyMitigation');

  // Hysteresis recovery after thermal stabilization cools down
  const recovery = {
    cpuPsi: 5.0,
    memoryPsi: 0.4,
    ioPsi: 0.5,
    thermalPressure: 50.0,
    thermalGuardActive: false,
    thermalEmergency: false,
    inThermalRecovery: true,
  };
  assert.equal(selectOptimizationObjective(recovery), 'ThermalRecovery');
});

test('13.5 Central Mutation Coordinator enforces single-owner domain locks and priority preemption', () => {
  const coord = new CentralMutationCoordinatorModel();
  const first = coord.acquireLease(
    'CpuPolicy',
    'AutonomousTuner',
    'cpufreq.schedutil_interactive_response',
    ControllerPriority.WorkloadOptimization
  );
  assert.equal(first.granted, true);

  // Equal or lower priority controller is rejected on the same domain
  const conflicting = coord.acquireLease(
    'CpuPolicy',
    'SecondaryController',
    'cpu-boost',
    ControllerPriority.WorkloadOptimization
  );
  assert.equal(conflicting.granted, false);
  assert.equal(coord.conflictRejections, 1);

  // Higher priority ThermalStabilization preempts WorkloadOptimization
  const thermalPreempt = coord.acquireLease(
    'CpuPolicy',
    'ThermalStabilizer',
    'thermal.schedutil_damped_pacing',
    ControllerPriority.ThermalStabilization
  );
  assert.equal(thermalPreempt.granted, true);
  assert.equal(
    coord.isLockedByOther('CpuPolicy', 'AutonomousTuner', ControllerPriority.WorkloadOptimization),
    true
  );
});

test('parseConfigIni is section-aware and rejects misplaced keys while supporting [stability] alias', () => {
  const raw = `
# Global comments
[general]
version = 3.1.0
thermal_trip_celsius = 75.0
top_app_interactive_min = 999

[thermal_guardian]
thermal_trip_celsius = 45.5

[stability]
cpu_pressure_boost_pct = 9.5
`;

  const parsed = parseConfigIni(raw);
  assert.equal(parsed.general?.version, '3.1.0');
  assert.equal(parsed.general?.thermal_trip_celsius, undefined);
  assert.equal(readSectionNumberKey(raw, 'thermal_trip_celsius'), 45.5);
  assert.equal(readSectionNumberKey(raw, 'top_app_interactive_min'), null);
  assert.equal(readSectionNumberKey(raw, 'cpu_pressure_boost_pct'), 9.5);
});

test('mergeConfigText preserves comments, unknown sections, and unknown keys during surgical updates', () => {
  const initial = `# Custom operator header
[general]
version = 3.1.0
operator_custom_key = preserve_this

[uclamp_tuning]
# Keep comment
top_app_interactive_min = 320
top_app_interactive_max = 1024

[vendor_custom_section]
vendor_flag = true
`;

  const merged = mergeConfigText(initial, {
    top_app_interactive_min: 448,
    ui_adaptive_bias: 'responsiveness',
    ui_balanced_uclamp_min: 360,
    vulkan_layer_mode: 'safe_hardware',
  });

  assert.match(merged, /# Custom operator header/);
  assert.match(merged, /operator_custom_key = preserve_this/);
  assert.match(merged, /# Keep comment/);
  assert.match(merged, /\[vendor_custom_section\]/);
  assert.match(merged, /vendor_flag = true/);
  assert.equal(readSectionNumberKey(merged, 'top_app_interactive_min'), 448);
  assert.equal(readSectionStringKey(merged, 'ui_adaptive_bias'), 'responsiveness');
  assert.equal(readSectionNumberKey(merged, 'ui_balanced_uclamp_min'), 360);
  assert.equal(readSectionStringKey(merged, 'vulkan_layer_mode'), 'safe_hardware');
});

