import { Component, ErrorInfo, ReactNode } from 'react';
import { AlertTriangle, RotateCcw, ShieldCheck } from 'lucide-react';

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
  error: Error | null;
}

export class ErrorBoundary extends Component<Props, State> {
  public state: State = {
    hasError: false,
    error: null,
  };

  public static getDerivedStateFromError(error: Error): State {
    return { hasError: true, error };
  }

  public componentDidCatch(error: Error, errorInfo: ErrorInfo) {
    console.error('[CoreFlow M3 WebUI] Uncaught component error:', error, errorInfo);
  }

  private handleReset = () => {
    this.setState({ hasError: false, error: null });
    window.location.reload();
  };

  public render() {
    if (this.state.hasError) {
      return (
        <div className="min-h-screen flex items-center justify-center p-4" style={{ backgroundColor: 'var(--md-sys-color-background)', color: 'var(--md-sys-color-on-surface)' }}>
          <div className="md3-card-elevated max-w-md w-full p-7 text-center space-y-4">
            <div
              className="w-12 h-12 rounded-2xl flex items-center justify-center mx-auto"
              style={{
                backgroundColor: 'var(--md-sys-color-error-container)',
                color: 'var(--md-sys-color-on-error-container)',
              }}
            >
              <AlertTriangle className="w-6 h-6" />
            </div>
            <div className="space-y-1.5">
              <h2 className="text-xl font-bold tracking-tight">
                UI Surface Interrupted
              </h2>
              <p className="text-xs leading-relaxed" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
                The Material 3 console caught a rendering exception. Native <code className="font-mono">coreflowd</code> daemon state and OEM baseline journals remain untouched.
              </p>
            </div>
            {this.state.error && (
              <pre className="md3-card-filled p-3 text-[11px] font-mono text-left overflow-auto max-h-32">
                {this.state.error.message}
              </pre>
            )}
            <div className="pt-1 flex items-center justify-center gap-3">
              <button
                type="button"
                id="btn-error-reload"
                onClick={this.handleReset}
                className="md3-btn-filled"
              >
                <RotateCcw className="w-3.5 h-3.5" />
                <span>Reload Console</span>
              </button>
            </div>
            <div className="pt-2 flex items-center justify-center gap-1.5 text-[11px]" style={{ color: 'var(--md-sys-color-on-surface-variant)' }}>
              <ShieldCheck className="w-3.5 h-3.5" />
              <span>Write-ahead CFJ4 recovery remains independent</span>
            </div>
          </div>
        </div>
      );
    }

    return this.props.children;
  }
}
