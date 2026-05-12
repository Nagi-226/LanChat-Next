import { useState } from 'react';

interface LoginPanelProps {
  onLogin: (id: number, password: string) => void;
  onSwitchToRegister: () => void;
  error?: string;
  loading?: boolean;
}

export function LoginPanel({ onLogin, onSwitchToRegister, error, loading }: LoginPanelProps) {
  const [id, setId] = useState('');
  const [password, setPassword] = useState('');

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const numId = parseInt(id, 10);
    if (Number.isNaN(numId) || !password) return;
    onLogin(numId, password);
  };

  return (
    <div className="flex min-h-0 flex-1 items-center justify-center p-6">
      <form
        onSubmit={handleSubmit}
        className="w-full max-w-sm rounded-2xl bg-light-sidebar p-8 shadow-xl dark:bg-dark-sidebar"
      >
        <h1 className="mb-1 text-center text-2xl font-bold text-light-text dark:text-dark-text">
          LanChat-Next
        </h1>
        <p className="mb-6 text-center text-xs text-light-muted dark:text-dark-muted">
          Sign in to your LAN chat account
        </p>

        {error && (
          <div className="mb-4 rounded-lg bg-red-500/10 px-3 py-2 text-center text-xs text-red-400">
            {error}
          </div>
        )}

        <label className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
          User ID
        </label>
        <input
          type="text"
          value={id}
          onChange={(e) => setId(e.target.value)}
          placeholder="Enter user ID"
          autoFocus
          className="mb-4 w-full rounded-lg border border-light-border bg-white px-3 py-2.5 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
        />

        <label className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
          Password
        </label>
        <input
          type="password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          placeholder="Enter password"
          className="mb-6 w-full rounded-lg border border-light-border bg-white px-3 py-2.5 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
        />

        <button
          type="submit"
          disabled={loading || !id || !password}
          className="w-full rounded-lg bg-dark-highlight px-4 py-2.5 text-sm font-bold text-white transition-colors hover:bg-[#d63850] active:bg-[#c23045] disabled:cursor-not-allowed disabled:opacity-50"
        >
          {loading ? 'Signing in...' : 'Sign in'}
        </button>

        <p className="mt-4 text-center text-xs text-light-muted dark:text-dark-muted">
          No account yet?{' '}
          <button
            type="button"
            onClick={onSwitchToRegister}
            className="font-medium text-dark-highlight hover:underline"
          >
            Create one
          </button>
        </p>
      </form>
    </div>
  );
}

export default LoginPanel;
