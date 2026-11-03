// CoreFlow Modern v2.0 Hardened KernelSU & System Root Shell Bridge

export interface ShellResult {
  errno: number;
  stdout: string;
  stderr: string;
}

declare global {
  interface Window {
    ksu?: {
      exec: (command: string) => Promise<string | ShellResult>;
    };
  }
}

const DEFAULT_EXEC_TIMEOUT_MS = 10000;

export function hasNativeKernelSuBridge(): boolean {
  return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
}

function normalizeShellResult(raw: unknown): ShellResult {
  if (typeof raw === 'string') {
    return { errno: 0, stdout: raw, stderr: '' };
  }
  if (typeof raw === 'object' && raw !== null) {
    const candidate = raw as Partial<ShellResult>;
    const errno = typeof candidate.errno === 'number' && Number.isFinite(candidate.errno)
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

  let timerId: number | undefined;
  try {
    const timeoutPromise = new Promise<ShellResult>((resolve) => {
      timerId = window.setTimeout(() => {
        resolve({
          errno: -1,
          stdout: '',
          stderr: `Native command timed out after ${timeoutMs}ms`,
        });
      }, Math.max(1000, timeoutMs));
    });

    const execPromise = window.ksu.exec(command).then(normalizeShellResult);
    return await Promise.race([execPromise, timeoutPromise]);
  } catch (error) {
    return {
      errno: -1,
      stdout: '',
      stderr: error instanceof Error ? error.message : String(error),
    };
  } finally {
    if (timerId !== undefined) {
      window.clearTimeout(timerId);
    }
  }
}
