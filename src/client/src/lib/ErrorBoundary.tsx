import type { ErrorInfo, ReactNode } from 'react';
import { Component } from 'react';

interface ErrorBoundaryProps {
  children: ReactNode;
}

interface ErrorBoundaryState {
  error: Error | null;
}

export class ErrorBoundary extends Component<ErrorBoundaryProps, ErrorBoundaryState> {
  state: ErrorBoundaryState = { error: null };

  static getDerivedStateFromError(error: Error): ErrorBoundaryState {
    return { error };
  }

  componentDidCatch(error: Error, info: ErrorInfo): void {
    console.error('[lanchat] React render failed', error, info.componentStack);
  }

  render() {
    if (!this.state.error) return this.props.children;

    return (
      <main className="flex min-h-screen items-center justify-center bg-light-bg px-6 text-light-text dark:bg-dark-bg dark:text-dark-text">
        <section className="max-w-lg rounded-2xl border border-light-border bg-white p-6 shadow-2xl dark:border-dark-border dark:bg-dark-sidebar">
          <p className="text-xs font-semibold uppercase tracking-[0.24em] text-dark-highlight">LanChat recovery</p>
          <h1 className="mt-3 text-2xl font-black">Something went wrong</h1>
          <p className="mt-2 text-sm text-light-muted dark:text-dark-muted">
            The chat shell caught a render error, so the desktop window stayed alive. Reload to return to the login screen.
          </p>
          <pre className="mt-4 max-h-32 overflow-auto rounded-lg bg-light-sidebar p-3 text-xs dark:bg-dark-bg">
            {this.state.error.message}
          </pre>
          <button
            type="button"
            onClick={() => window.location.reload()}
            className="mt-5 rounded-lg bg-dark-highlight px-4 py-2 text-sm font-semibold text-white hover:bg-[#d63850]"
          >
            Reload app
          </button>
        </section>
      </main>
    );
  }
}
