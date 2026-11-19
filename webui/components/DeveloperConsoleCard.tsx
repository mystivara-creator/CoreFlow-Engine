import React, { useEffect, useRef, useState } from 'react';
import {
  Activity,
  Check,
  Code2,
  Copy,
  Play,
  RefreshCw,
  ShieldAlert,
  Terminal,
} from 'lucide-react';
import { execNativeKsuShell, hasNativeKernelSuBridge } from '../bridge/daemonBridge';

interface Props {
  nativeMode: boolean;
  onRefreshTelemetry: () => Promise<void>;
}

export const DeveloperConsoleCard: React.FC<Props> = ({
  nativeMode,
  onRefreshTelemetry,
}) => {
  const [output, setOutput] = useState<string>(
    'Select an inspection action above to query coreflowd runtime JSON, capabilities, recovery journal, or CF-DVS state.'
  );
  const [activeActionTitle, setActiveActionTitle] = useState<string>('Ready');
  const [isRunning, setIsRunning] = useState(false);
  const [copied, setCopied] = useState(false);
  const copyTimerRef = useRef<number | null>(null);

  useEffect(() => {
    return () => {
      if (copyTimerRef.current !== null) window.clearTimeout(copyTimerRef.current);
    };
  }, []);

  const RESOLVE_BIN =
    'BIN="/data/adb/modules_update/cfae_framework/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/cfae_framework/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
    '[ -f "$BIN" ] || BIN="/system/bin/coreflowd"';

  const extractJson = (raw: string): unknown => {
    const trimmed = raw.trim();
    const lines = trimmed
      .split('\n')
      .map((l) => l.trim())
      .filter(Boolean);
    for (let i = lines.length - 1; i >= 0; i--) {
      if (lines[i].startsWith('{') && lines[i].endsWith('}')) {
        try {
          return JSON.parse(lines[i]);
        } catch {
          // continue scanning
        }
      }
    }
    const first = trimmed.indexOf('{');
    const last = trimmed.lastIndexOf('}');
    if (first >= 0 && last > first) {
      return JSON.parse(trimmed.slice(first, last + 1).replace(/\r?\n/g, ''));
    }
    return JSON.parse(lines[lines.length - 1] ?? '{}');
  };

  const runCommand = async (cmd: string, title: string) => {
    setIsRunning(true);
    setActiveActionTitle(title);
    try {
      if (!hasNativeKernelSuBridge()) {
        if (cmd.includes('--status')) {
          setOutput(
            JSON.stringify(
              {
                running: true,
                pid: 24108,
                uclampMin: 320,
                uclampMax: 1024,
                uclampSupported: true,
                platformVendor: 'Universal Android',
                osFamily: 'Android AOSP / GKI',
                executionTier: 'UniversalFallback',
                policyFamily: 'CoreFlowBase',
                cpuPressurePct: 1.2,
                memoryPressurePct: 0.3,
                ioPressurePct: 0.4,
                baselineNodesSnapshotted: 6,
                socTempCelsius: 38.6,
                socCompositeCelsius: 38.6,
                cpuAverageCelsius: 38.2,
                cpuPeakCelsius: 41.5,
                gpuTempCelsius: 36.8,
                batteryTempCelsius: 33.4,
                skinTempCelsius: 35.1,
                recoveryPending: false,
                thermalPolicyAvailable: true,
                thermalSource: 'multi-sensor-composite',
                thermalSensor: 'SoC-Composite (8 CPU cores, 1 GPU domain)',
                thermalSeverity: 0,
                thermalPressure: 44.0,
                aiBackend: 'embedded-rls-mlp',
                aiOnnxActive: false,
                aiPredictedPressure10s: 46.2,
                aiPredictedVelocity: 0.08,
                aiRecommendedScale: 0.94,
                aiConfidence: 0.86,
                tunerStage: 'RETAINED',
                tunerIntent: 'HoldEquilibrium',
                tunerCandidateId: 'io.mq_deadline_low_latency',
                tunerVerdict: 'IMPROVED',
              },
              null,
              2
            )
          );
        } else if (cmd.includes('--capabilities')) {
          setOutput(
            JSON.stringify(
              {
                socModel: 'Preview SoC Profile',
                kernelVersion: 'Linux 6.1-android15-gki',
                onlineCpus: '0-7',
                gpuType: 'Universal Multi-Vendor GPU',
                rustEngine: true,
                powerHalDetected: true,
                aiBackend: 'embedded-rls-mlp',
                onnxRuntimeLoaded: false,
                binderInterception: false,
                npuOffload: false,
                frameTelemetry: false,
              },
              null,
              2
            )
          );
        } else if (cmd.includes('--journal')) {
          setOutput(
            JSON.stringify(
              {
                format: 'CFJ4',
                bootId: 'preview-boot-epoch-v4',
                recoveryPending: false,
                entries: [],
              },
              null,
              2
            )
          );
        } else if (cmd.includes('--flight-recorder')) {
          setOutput(
            JSON.stringify(
              {
                windowSeconds: 60,
                samples: [
                  {
                    ts: Date.now() - 5000,
                    temp: 38.2,
                    pressure: 43.1,
                    cpuPsi: 2.1,
                    memPsi: 0.4,
                    ioPsi: 0.9,
                    scale: 0.94,
                    stage: 'RETAINED',
                    candidate: 'io.mq_deadline_low_latency',
                    verdict: 'IMPROVED',
                  },
                  {
                    ts: Date.now(),
                    temp: 38.6,
                    pressure: 44.0,
                    cpuPsi: 1.2,
                    memPsi: 0.3,
                    ioPsi: 0.4,
                    scale: 0.94,
                    stage: 'RETAINED',
                    candidate: 'io.mq_deadline_low_latency',
                    verdict: 'IMPROVED',
                  },
                ],
              },
              null,
              2
            )
          );
        } else if (cmd.includes('--dvs-status')) {
          setOutput('OK INACTIVE (Preview Simulation)');
        } else {
          setOutput(`[${title}]\nStatus: OK (Preview Sandbox)\nExit code: 0`);
        }
        return;
      }

      const result = await execNativeKsuShell(cmd);
      const failure =
        result.errno !== 0
          ? `[exit ${result.errno}] ${result.stderr.trim() || 'Command failed'}\n`
          : '';
      if (!result.stdout.trim()) {
        setOutput(failure || result.stderr || 'Command executed with no output.');
      } else {
        try {
          const parsed = extractJson(result.stdout);
          if (cmd.includes('--flight-recorder') && typeof parsed === 'object' && parsed !== null && 'flightRecorder' in parsed) {
            setOutput(
              failure +
                JSON.stringify(
                  {
                    flightRecorder: (parsed as Record<string, unknown>).flightRecorder,
                    batteryPower: (parsed as Record<string, unknown>).batteryPower,
                  },
                  null,
                  2
                )
            );
          } else {
            setOutput(failure + JSON.stringify(parsed, null, 2));
          }
        } catch {
          setOutput(failure + result.stdout);
        }
      }
      if (cmd.includes('--status')) {
        await onRefreshTelemetry();
      }
    } catch (err) {
      setOutput(`Error: ${String(err)}`);
    } finally {
      setIsRunning(false);
    }
  };

  const copyToClipboard = async () => {
    try {
      await navigator.clipboard.writeText(output);
    } catch {
      return;
    }
    setCopied(true);
    if (copyTimerRef.current !== null) window.clearTimeout(copyTimerRef.current);
    copyTimerRef.current = window.setTimeout(() => {
      setCopied(false);
      copyTimerRef.current = null;
    }, 2000);
  };

  return (
    <div className="md3-card-elevated p-6 sm:p-8 space-y-5">
      {/* Header */}
      <div
        className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5"
        style={{ borderBottom: '1px solid var(--md-sys-color-outline-variant)' }}
      >
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2">
            <Terminal className="w-4 h-4" style={{ color: 'var(--md-sys-color-primary)' }} />
            <h2 className="text-base sm:text-lg font-bold">
              Runtime RPC &amp; Diagnostic Console
            </h2>
            <span className="md3-badge md3-badge-neutral font-mono">
              coreflow-api.js
            </span>
            <span
              className={
                nativeMode
                  ? 'md3-badge md3-badge-success'
                  : 'md3-badge md3-badge-warning'
              }
            >
              {nativeMode ? 'KernelSU Bridge Active' : 'Preview Simulator'}
            </span>
          </div>
          <p
            className="text-xs sm:text-sm leading-relaxed"
            style={{ color: 'var(--md-sys-color-on-surface-variant)' }}
          >
            Inspect raw JSON telemetry, discovered hardware capabilities, CFJ4 journal records, Flight-Recorder traces, and CF-DVS capture status directly from <code className="font-mono">coreflowd</code>.
          </p>
        </div>

        <button
          type="button"
          id="btn-copy-console-output"
          onClick={() => void copyToClipboard()}
          className="md3-btn-outlined self-start shrink-0"
        >
          {copied ? (
            <Check className="w-3.5 h-3.5" style={{ color: 'var(--md-sys-color-primary)' }} />
          ) : (
            <Copy className="w-3.5 h-3.5" />
          )}
          <span>{copied ? 'Copied' : 'Copy Output'}</span>
        </button>
      </div>

      {/* M3 Tonal Action Chips */}
      <div className="flex flex-wrap gap-2">
        {[
          {
            label: 'Query Status (--status)',
            cmd: `${RESOLVE_BIN}; "$BIN" --status`,
            title: 'Daemon Status',
            Icon: Play,
          },
          {
            label: 'Flight Recorder (--flight-recorder)',
            cmd: `${RESOLVE_BIN}; "$BIN" --flight-recorder`,
            title: 'Flight Recorder Trace',
            Icon: Activity,
          },
          {
            label: 'Capabilities (--capabilities)',
            cmd: `${RESOLVE_BIN}; "$BIN" --capabilities`,
            title: 'Hardware Capabilities',
            Icon: Code2,
          },
          {
            label: 'Inspect Journal (--journal)',
            cmd: `${RESOLVE_BIN}; "$BIN" --journal`,
            title: 'Recovery Journal',
            Icon: RefreshCw,
          },
          {
            label: 'CF-DVS Status (--dvs-status)',
            cmd: `${RESOLVE_BIN}; "$BIN" --dvs-status`,
            title: 'CF-DVS Session Status',
            Icon: Activity,
          },
          {
            label: 'Inspect coreflow.conf',
            cmd: 'cat /data/adb/coreflow/coreflow.conf 2>/dev/null',
            title: 'Configuration File',
            Icon: ShieldAlert,
          },
        ].map(({ label, cmd, title, Icon }) => {
          const isSelected = activeActionTitle === title;
          return (
            <button
              key={title}
              type="button"
              onClick={() => void runCommand(cmd, title)}
              disabled={isRunning}
              className={'md3-chip ' + (isSelected ? 'is-selected' : '')}
            >
              <Icon className="w-3.5 h-3.5" />
              <span>{label}</span>
            </button>
          );
        })}
      </div>

      {/* Terminal Output */}
      <div className="md3-card-filled p-5 font-mono text-xs overflow-x-auto max-h-80">
        <div
          className="flex items-center justify-between pb-2.5 mb-2.5 text-[11px]"
          style={{
            borderBottom: '1px solid var(--md-sys-color-outline-variant)',
            color: 'var(--md-sys-color-on-surface-variant)',
          }}
        >
          <span>Inspector Output · {activeActionTitle}</span>
          <span>{isRunning ? 'Executing query…' : 'Read-only RPC'}</span>
        </div>
        <pre className="whitespace-pre leading-relaxed">{output}</pre>
      </div>
    </div>
  );
};
