import test from 'node:test';
import assert from 'node:assert/strict';
import {
  computeVulkanTelemetry,
  DEFAULT_VULKAN_CONFIG,
  mergeConfigText,
  parseConfigIni,
  readSectionNumberKey,
  readSectionStringKey,
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
    hwPushDescriptors: true,
    hwGlobalPriorityQuery: false,
    hwShaderFloatControls2: false,
    hwMaintenance5: true,
    hwMaintenance6: false,
    hwMaintenance7: false,
    hwDescriptorBuffer: false,
    hwGraphicsPipelineLibrary: false,
    maxPushDescriptors: 32,
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
    assert.equal(vk.pushDescriptors, true);
    assert.equal(vk.maintenance5, true);
    assert.equal(vk.maintenance6, false);
    assert.equal(vk.maintenance7, false);
    assert.equal(vk.descriptorBuffer, false);
    assert.equal(vk.graphicsPipelineLibrary, false);
    assert.equal(vk.maxPushDescriptors, 32);
    if (mode === 'full_capability_override') {
      assert.equal(vk.mode, 'safe_hardware');
    }
  }
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
