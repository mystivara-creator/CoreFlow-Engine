import test from 'node:test';
import assert from 'node:assert/strict';
import {
  assignPackageRule,
  buildSafeSetConfigArg,
  computeHealthScore,
  computePillarReadiness,
  createPreviewStatus,
  formatDuration,
  getPackageRuleCategory,
  isValidPackageName,
  parseInstalledPackages,
  parseV5Status,
  sanitizePackageList,
  stepPreviewStatus,
} from '../autonomousEngine.ts';

test('isValidPackageName and sanitizePackageList accept real Android packages and reject injection attempts', () => {
  assert.equal(isValidPackageName('com.miHoYo.GenshinImpact'), true);
  assert.equal(isValidPackageName('org.mozilla.firefox'), true);
  assert.equal(isValidPackageName('notapackage'), false);
  assert.equal(isValidPackageName('com.app;rm -rf /'), false);
  assert.equal(isValidPackageName("com.app' && id"), false);
  assert.equal(isValidPackageName('com.app\n[general]'), false);

  assert.deepEqual(
    sanitizePackageList([
      ' com.miHoYo.GenshinImpact ',
      'COM.MIHOYO.GENSHINIMPACT',
      'bad;inject',
      'com.android.camera',
    ]),
    ['com.miHoYo.GenshinImpact', 'com.android.camera']
  );
});

test('buildSafeSetConfigArg allowlists only v5.1 keys and safe single-quoted values', () => {
  assert.equal(buildSafeSetConfigArg('profile', 'smoothness'), "'profile=smoothness'");
  assert.equal(buildSafeSetConfigArg('profile', 'extreme_hack'), null);
  assert.equal(buildSafeSetConfigArg('enable_frame_smoothness', 'true'), "'enable_frame_smoothness=true'");
  assert.equal(buildSafeSetConfigArg('enable_frame_smoothness', '1;id'), null);
  assert.equal(buildSafeSetConfigArg('gaming_apps', ''), "'gaming_apps='");
  assert.equal(buildSafeSetConfigArg('smoothness_apps', 'com.instagram.android'), "'smoothness_apps=com.instagram.android'");
  assert.equal(buildSafeSetConfigArg('battery_apps', 'com.whatsapp'), "'battery_apps=com.whatsapp'");
  assert.equal(
    buildSafeSetConfigArg('gaming_apps', 'com.tencent.ig, com.miHoYo.GenshinImpact'),
    "'gaming_apps=com.tencent.ig,com.miHoYo.GenshinImpact'"
  );
  assert.equal(buildSafeSetConfigArg('gaming_apps', 'com.app$(whoami)'), null);
  assert.equal(buildSafeSetConfigArg('bypass_charging', 'true'), "'bypass_charging=true'");
  assert.equal(buildSafeSetConfigArg('bypass_on_gaming', 'false'), "'bypass_on_gaming=false'");
  assert.equal(buildSafeSetConfigArg('enable_idle_compaction', 'true'), "'enable_idle_compaction=true'");
  assert.equal(buildSafeSetConfigArg('enable_predictive_preload', 'true'), "'enable_predictive_preload=true'");
  assert.equal(buildSafeSetConfigArg('enable_camera_boost', 'false'), "'enable_camera_boost=false'");
  assert.equal(buildSafeSetConfigArg('charge_limit_pct', '85'), "'charge_limit_pct=85'");
  assert.equal(buildSafeSetConfigArg('charge_limit_pct', '0'), "'charge_limit_pct=0'");
  assert.equal(buildSafeSetConfigArg('charge_limit_pct', '45'), null);
  assert.equal(buildSafeSetConfigArg('charge_limit_pct', '100'), null);
  assert.equal(buildSafeSetConfigArg('unknown_key', 'true'), null);
});

test('parseV5Status parses live daemon payload and rejects malformed schema', () => {
  const sample = createPreviewStatus();
  const parsed = parseV5Status(sample);
  assert.equal(parsed.version, '6.2.0');
  assert.equal(parsed.profile, 'balanced');
  assert.equal(parsed.effectiveProfile, 'balanced');
  assert.equal(parsed.hardware.storageType, 'UFS');
  assert.equal(parsed.actuators.perThreadUclampSupported, true);
  assert.equal(parsed.actuators.semanticKnobsDiscovered, 25);
  assert.equal(parsed.actuators.oemYieldedNodes, 2);
  assert.ok(parsed.actuators.semanticKnobLeaves.length > 0);
  assert.equal(parsed.hyperFluid.framePacingAvailable, true);
  assert.equal(parsed.hyperFluid.appDnaLearned, 12);
  assert.equal(parsed.powerConfig.chargeLimitPct, 85);
  assert.equal(parsed.batteryPower.bypassSupported, true);
  assert.ok(parsed.cpuClusters.length > 0);
  assert.ok(parsed.episodes.length > 0);
  assert.deepEqual(parsed.apps.smoothness, ['com.instagram.android']);
  assert.deepEqual(parsed.apps.battery, ['com.whatsapp']);
  assert.equal(parsed.session.launchBursts, 14);
  assert.equal(parsed.bootSettleActive, false);
  assert.equal(parsed.bootSettleExitReason, 'warm_daemon_restart');
  assert.equal(parsed.bootRampProgress, 1.0);
  const settling = parseV5Status({
    ...sample,
    mode: 'BOOT_SETTLE',
    bootSettleActive: true,
    bootSettleDurationSec: 22.5,
    bootSettleExitReason: 'settling',
    bootRampProgress: 0.0,
  });
  assert.equal(settling.mode, 'BOOT_SETTLE');
  assert.equal(settling.bootSettleActive, true);
  assert.equal(settling.bootSettleDurationSec, 22.5);
  assert.equal(settling.bootRampProgress, 0.0);
  assert.throws(() => parseV5Status(null));
  assert.throws(() => parseV5Status({ running: true }));
});

test('per-app rule assignment is mutually exclusive across all 4 buckets and parses pm output safely', () => {
  let rules = createPreviewStatus().apps;
  assert.equal(getPackageRuleCategory(rules, 'com.miHoYo.GenshinImpact'), 'gaming');
  rules = assignPackageRule(rules, 'com.miHoYo.GenshinImpact', 'smoothness');
  assert.equal(getPackageRuleCategory(rules, 'com.miHoYo.GenshinImpact'), 'smoothness');
  assert.equal(rules.gaming.includes('com.miHoYo.GenshinImpact'), false);
  rules = assignPackageRule(rules, 'com.miHoYo.GenshinImpact', 'default');
  assert.equal(getPackageRuleCategory(rules, 'com.miHoYo.GenshinImpact'), 'default');

  const parsedPm = parseInstalledPackages(
    'package:com.whatsapp\npackage:com.android.chrome\npackage:bad;cmd\npackage:com.whatsapp\n'
  );
  assert.deepEqual(parsedPm, ['com.android.chrome', 'com.whatsapp']);
});

test('computeHealthScore, computePillarReadiness, formatDuration, and stepPreviewStatus stay bounded and clean', () => {
  let state = createPreviewStatus();
  assert.equal(computePillarReadiness(state).readyCount, 4);
  assert.equal(formatDuration(3665), '1h 1m');
  assert.equal(formatDuration(125), '2m 5s');
  for (let i = 0; i < 10; i++) {
    state = stepPreviewStatus(state, i);
    const score = computeHealthScore(state);
    assert.ok(score >= 15 && score <= 100);
  }
  assert.ok(state.history.length <= 60);
  const emergency = { ...state, mode: 'THERMAL_EMERGENCY' as const, thermalGuard: true };
  assert.ok(computeHealthScore(emergency) < computeHealthScore(state));
});
