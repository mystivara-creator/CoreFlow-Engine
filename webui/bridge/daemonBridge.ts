// CoreFlow Modern v2.0 KernelSU & Root Shell Daemon Bridge

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

export function hasNativeKernelSuBridge(): boolean {
  return typeof window !== 'undefined' && typeof window.ksu?.exec === 'function';
}

export async function execNativeKsuShell(command: string): Promise<ShellResult> {
  if (!hasNativeKernelSuBridge() || !window.ksu) {
    return { errno: -1, stdout: '', stderr: 'Native KernelSU bridge unavailable' };
  }

  try {
    const raw = await window.ksu.exec(command);
    if (typeof raw === 'string') {
      return { errno: 0, stdout: raw, stderr: '' };
    }
    return raw;
  } catch (error) {
    return {
      errno: -1,
      stdout: '',
      stderr: error instanceof Error ? error.message : String(error),
    };
  }
}
