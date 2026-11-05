// CFAE System Framework v3.1.0 Hardened KernelSU / KernelSU Next & Root Shell Bridge

export interface ShellResult {
  errno: number;
  stdout: string;
  stderr: string;
}

type KsuExecFn = (
  command: string,
  optionsOrCallback?: string,
  callbackFuncName?: string
) => unknown;

declare global {
  interface Window {
    ksu?: {
      exec: KsuExecFn;
    };
    [key: string]: unknown;
  }
}

const DEFAULT_EXEC_TIMEOUT_MS = 10000;
let callbackSeq = 0;

export function hasNativeKernelSuBridge(): boolean {
  return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
}

function normalizeShellResult(raw: unknown): ShellResult {
  if (typeof raw === 'string') {
    return { errno: 0, stdout: raw, stderr: '' };
  }
  if (typeof raw === 'object' && raw !== null) {
    const candidate = raw as Partial<ShellResult>;
    const errno =
      typeof candidate.errno === 'number' && Number.isFinite(candidate.errno)
        ? candidate.errno
        : -1;
    const stdout = typeof candidate.stdout === 'string' ? candidate.stdout : '';
    const stderr = typeof candidate.stderr === 'string' ? candidate.stderr : '';
    return { errno, stdout, stderr };
  }
  return { errno: -1, stdout: '', stderr: 'Malformed native bridge response' };
}

export async function execNativeKsuShell(
  command: string,
  timeoutMs: number = DEFAULT_EXEC_TIMEOUT_MS
): Promise<ShellResult> {
  if (!hasNativeKernelSuBridge() || !window.ksu) {
    return { errno: -1, stdout: '', stderr: 'Native KernelSU bridge unavailable' };
  }
  if (!command || command.includes('\0')) {
    return { errno: -1, stdout: '', stderr: 'Invalid shell command' };
  }

  const ksu = window.ksu;
  const boundedTimeout = Math.max(1000, timeoutMs);

  return new Promise<ShellResult>((resolve) => {
    let settled = false;
    callbackSeq += 1;
    const cbName = `__coreflow_ksu_cb_${Date.now()}_${callbackSeq}`;

    const finish = (result: ShellResult) => {
      if (settled) return;
      settled = true;
      window.clearTimeout(timerId);
      delete window[cbName];
      resolve(result);
    };

    const timerId = window.setTimeout(() => {
      finish({
        errno: -1,
        stdout: '',
        stderr: `Native command timed out after ${boundedTimeout}ms`,
      });
    }, boundedTimeout);

    window[cbName] = (errno: unknown, stdout?: unknown, stderr?: unknown) => {
      if (typeof errno === 'object' && errno !== null) {
        finish(normalizeShellResult(errno));
        return;
      }
      finish({
        errno: typeof errno === 'number' && Number.isFinite(errno) ? errno : 0,
        stdout: typeof stdout === 'string' ? stdout : String(stdout ?? ''),
        stderr: typeof stderr === 'string' ? stderr : String(stderr ?? ''),
      });
    };

    try {
      // Standard KernelSU / KernelSU Next @JavascriptInterface signature:
      // ksu.exec(cmd, optionsJson, callbackFuncName) returns undefined and invokes
      // window[cbName](errno, stdout, stderr) asynchronously from a background thread.
      const directReturn = ksu.exec(command, '{}', cbName);
      if (directReturn !== undefined) {
        Promise.resolve(directReturn)
          .then((resolved) => finish(normalizeShellResult(resolved)))
          .catch((err) =>
            finish({
              errno: -1,
              stdout: '',
              stderr: err instanceof Error ? err.message : String(err),
            })
          );
      }
    } catch {
      // Fallback for legacy 1-argument @JavascriptInterface ksu.exec(cmd) bridges.
      try {
        const legacyReturn = ksu.exec(command);
        Promise.resolve(legacyReturn)
          .then((resolved) => finish(normalizeShellResult(resolved)))
          .catch((err) =>
            finish({
              errno: -1,
              stdout: '',
              stderr: err instanceof Error ? err.message : String(err),
            })
          );
      } catch (fallbackErr) {
        finish({
          errno: -1,
          stdout: '',
          stderr: fallbackErr instanceof Error ? fallbackErr.message : String(fallbackErr),
        });
      }
    }
  });
}
