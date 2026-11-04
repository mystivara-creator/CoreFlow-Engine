import React, { useState } from 'react';
import { Terminal, Play, Copy, Check, RefreshCw, Code2, ShieldAlert } from 'lucide-react';
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
    'Select an inspection action above to query coreflowd runtime JSON or recovery journal.'
  );
  const [isRunning, setIsRunning] = useState(false);
  const [copied, setCopied] = useState(false);

  const RESOLVE_BIN =
    'BIN="/data/adb/modules/cfae_framework/system/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/data/adb/modules/coreflow_engine/system/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/system_ext/bin/coreflowd"; ' +
    '[ -x "$BIN" ] || BIN="/system/bin/coreflowd"';

  const runCommand = async (cmd: string, title: string) => {
    setIsRunning(true);
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
                thermalPressure: 65.0,
              },
              null,
              2
            )
          );
        } else if (cmd.includes('--capabilities')) {
          setOutput(
            JSON.stringify(
              {
                socModel: 'Qualcomm Snapdragon (SM7250 / SD765G)',
                kernelVersion: 'Linux 5.4.242-android12-9-g06489379',
                onlineCpus: '0-7 (4+3+1 Tri-Cluster EAS)',
                gpuType: 'Qualcomm Adreno 620',
                rustEngine: true,
                powerHalDetected: true,
                binderInterception: false,
                npuOffload: false,
                frameTelemetry: false,
              },
              null,
              2
            )
          );
        } else {
          setOutput(`[${title}]\nStatus: OK (Preview Sandbox)\nExit code: 0`);
        }
        return;
      }

      const result = await execNativeKsuShell(cmd);
      try {
        const parsed = JSON.parse(result.stdout.trim().split('\n').pop() ?? '{}');
        setOutput(JSON.stringify(parsed, null, 2));
      } catch {
        setOutput(result.stdout || result.stderr || 'Command executed with no output.');
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

  const copyToClipboard = () => {
    void navigator.clipboard.writeText(output);
    setCopied(true);
    window.setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="m3-card p-6 sm:p-8 space-y-5">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-start justify-between gap-4 pb-5 border-b border-slate-900/[0.06] dark:border-white/[0.06]">
        <div className="space-y-1.5 max-w-2xl">
          <div className="flex flex-wrap items-center gap-2 text-xs text-slate-500 dark:text-slate-400">
            <Terminal className="w-4 h-4 text-[#0b57d0] dark:text-[#a8c7fa]" />
            <span className="text-base sm:text-lg font-semibold text-slate-900 dark:text-slate-100">
              Runtime RPC & Diagnostic Console
            </span>
            <span aria-hidden="true">·</span>
            <span className="font-mono">coreflow-api.js</span>
            <span aria-hidden="true">·</span>
            <span>{nativeMode ? 'KernelSU Bridge Active' : 'Preview Simulator'}</span>
          </div>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400 leading-relaxed">
            Inspect raw JSON telemetry, discovered hardware capabilities, and durable journal state directly from `coreflowd`.
          </p>
        </div>

        <button
          onClick={copyToClipboard}
          className="px-4 py-2 rounded-full bg-slate-900/[0.05] hover:bg-slate-900/[0.09] dark:bg-white/[0.06] dark:hover:bg-white/[0.11] text-xs font-medium text-slate-700 dark:text-slate-200 inline-flex items-center gap-2 self-start"
        >
          {copied ? (
            <Check className="w-3.5 h-3.5 text-[#0b57d0] dark:text-[#a8c7fa]" />
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
            label: 'Capabilities (--capabilities)',
            cmd: `${RESOLVE_BIN}; "$BIN" --capabilities`,
            title: 'Hardware Capabilities',
            Icon: Code2,
          },
          {
            label: 'Inspect Recovery Journal',
            cmd: 'cat /data/adb/coreflow/baseline.journal 2>/dev/null || echo "Journal clean"',
            title: 'Recovery Journal',
            Icon: RefreshCw,
          },
          {
            label: 'Inspect coreflow.conf',
            cmd: 'cat /data/adb/coreflow/coreflow.conf 2>/dev/null',
            title: 'Configuration File',
            Icon: ShieldAlert,
          },
        ].map(({ label, cmd, title, Icon }) => (
          <button
            key={title}
            onClick={() => void runCommand(cmd, title)}
            disabled={isRunning}
            className="px-4 py-2 rounded-full bg-[#d3e3fd]/70 hover:bg-[#d3e3fd] text-[#041e49] dark:bg-[#0842a0]/60 dark:hover:bg-[#0842a0] dark:text-[#d3e3fd] text-xs font-medium transition-all inline-flex items-center gap-2 disabled:opacity-50"
          >
            <Icon className="w-3.5 h-3.5" />
            <span>{label}</span>
          </button>
        ))}
      </div>

      {/* Terminal Output */}
      <div className="m3-tonal-block p-5 font-mono text-xs text-slate-800 dark:text-slate-200 overflow-x-auto max-h-80">
        <pre className="whitespace-pre leading-relaxed">{output}</pre>
      </div>
    </div>
  );
};
