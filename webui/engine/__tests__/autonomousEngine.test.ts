import test from 'node:test';
import assert from 'node:assert/strict';
import {
  buildSafeSetConfigArg,
  computeHealthScore,
  createPreviewStatus,
  isValidPackageName,
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

test('buildSafeSetConfigArg allowlists only v5 keys and safe single-quoted values', () => {
  assert.equal(buildSafeSetConfigArg('profile', 'smoothness'), "'profile=smoothness'");
  assert.equal(buildSafeSetConfigArg('profile', 'extreme_hack'), null);
  assert.equal(buildSafeSetConfigArg('enable_frame_smoothness', 'true'), "'enable_frame_smoothness=true'");
  assert.equal(buildSafeSetConfigArg('enable_frame_smoothness', '1;id'), null);
  assert.equal(buildSafeSetConfigArg('gaming_apps', ''), "'gaming_apps='");
  assert.equal(
    buildSafeSetConfigArg('gaming_apps', 'com.tencent.ig, com.miHoYo.GenshinImpact'),
    "'gaming_apps=com.tencent.ig,com.miHoYo.GenshinImpact'"
  );
  assert.equal(buildSafeSetConfigArg('gaming_apps', 'com.app$(whoami)'), null);
  assert.equal(buildSafeSetConfigArg('unknown_key', 'true'), null);
});

test('parseV5Status parses live daemon payload and rejects malformed schema', () => {
  const sample = createPreviewStatus();
  const parsed = parseV5Status(sample);
  assert.equal(parsed.version, '5.0.2');
  assert.equal(parsed.profile, 'balanced');
  assert.equal(parsed.hardware.storageType, 'UFS');
  assert.equal(parsed.actuators.perThreadUclampSupported, true);
  assert.throws(() => parseV5Status(null));
  assert.throws(() => parseV5Status({ running: true }));
});

test('computeHealthScore and stepPreviewStatus stay bounded and clean', () => {
  let state = createPreviewStatus();
  for (let i = 0; i < 10; i++) {
    state = stepPreviewStatus(state, i);
    const score = computeHealthScore(state);
    assert.ok(score >= 15 && score <= 100);
  }
  assert.ok(state.history.length <= 60);
  const emergency = { ...state, mode: 'THERMAL_EMERGENCY' as const, thermalGuard: true };
  assert.ok(computeHealthScore(emergency) < computeHealthScore(state));
});
