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
  const [output, setOutput] = useState<string>('// Select an RPC command below or run custom command...');
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
        // Simulated response for Web UI demo
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
          setOutput(`// ${title}\nOK (Simulated response)\nExit code: 0`);
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
    navigator.clipboard.writeText(output);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="rounded-3xl bg-white dark:bg-[#111827] border border-slate-200 dark:border-slate-800 p-5 sm:p-7 shadow-sm space-y-5">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3">
        <div>
          <div className="flex items-center gap-2 mb-1">
            <Terminal className="w-4 h-4 text-sky-500" />
            <h3 className="text-base sm:text-lg font-bold text-slate-900 dark:text-white">
              Developer & KernelSU API Console
            </h3>
            <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-sky-500/10 text-sky-500 border border-sky-500/20 font-mono">
              coreflow-api.js
            </span>
            <span className="px-2 py-0.5 rounded-full text-[10px] font-semibold bg-slate-100 dark:bg-slate-800 text-slate-600 dark:text-slate-400 border border-slate-300 dark:border-slate-700">
              {nativeMode ? 'KernelSU Active' : 'Sandbox Simulator'}
            </span>
          </div>
          <p className="text-xs text-slate-500 dark:text-slate-400">
            Directly test RPC endpoints and query hardware status via the native KernelSU bridge.
          </p>
        </div>

        <button
          onClick={copyToClipboard}
          className="px-3 py-1.5 rounded-xl border border-slate-200 dark:border-slate-700 text-xs font-medium text-slate-700 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-800 flex items-center gap-1.5 self-start sm:self-auto"
        >
          {copied ? <Check className="w-3.5 h-3.5 text-emerald-500" /> : <Copy className="w-3.5 h-3.5" />}
          {copied ? 'Copied' : 'Copy Output'}
        </button>
      </div>

      {/* Quick Action Buttons */}
      <div className="flex flex-wrap gap-2">
        <button
          onClick={() => runCommand(`${RESOLVE_BIN}; "$BIN" --status`, 'Daemon Status')}
          disabled={isRunning}
          className="px-3.5 py-2 rounded-xl bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 text-xs font-semibold transition-all flex items-center gap-1.5"
        >
          <Play className="w-3 h-3 text-sky-500" />
          Query Status (--status)
        </button>

        <button
          onClick={() => runCommand(`${RESOLVE_BIN}; "$BIN" --capabilities`, 'Hardware Capabilities')}
          disabled={isRunning}
          className="px-3.5 py-2 rounded-xl bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 text-xs font-semibold transition-all flex items-center gap-1.5"
        >
          <Code2 className="w-3 h-3 text-teal-500" />
          Capabilities (--capabilities)
        </button>

        <button
          onClick={() => runCommand('cat /data/adb/coreflow/baseline.journal 2>/dev/null || echo "Journal clean"', 'Recovery Journal')}
          disabled={isRunning}
          className="px-3.5 py-2 rounded-xl bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 text-xs font-semibold transition-all flex items-center gap-1.5"
        >
          <RefreshCw className="w-3 h-3 text-emerald-500" />
          Verify Journal
        </button>

        <button
          onClick={() => runCommand('cat /data/adb/coreflow/coreflow.conf 2>/dev/null', 'Configuration File')}
          disabled={isRunning}
          className="px-3.5 py-2 rounded-xl bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 text-slate-800 dark:text-slate-200 text-xs font-semibold transition-all flex items-center gap-1.5"
        >
          <ShieldAlert className="w-3 h-3 text-amber-500" />
          Inspect coreflow.conf
        </button>
      </div>

      {/* Terminal Output Screen */}
      <div className="relative rounded-2xl bg-slate-950 border border-slate-800 p-4 font-mono text-xs text-sky-400 overflow-x-auto max-h-72">
        <pre className="whitespace-pre">{output}</pre>
      </div>
    </div>
  );
};
