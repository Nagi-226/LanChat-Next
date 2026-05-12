import { useState } from 'react';

interface RegisterPanelProps {
  onRegister: (nickname: string, password: string) => void;
  onSwitchToLogin: () => void;
  error?: string;
  loading?: boolean;
}

export function RegisterPanel({ onRegister, onSwitchToLogin, error, loading }: RegisterPanelProps) {
  const [nickname, setNickname] = useState('');
  const [password, setPassword] = useState('');
  const [confirm, setConfirm] = useState('');
  const [localError, setLocalError] = useState('');

  const displayError = localError || error;

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    setLocalError('');

    if (!nickname.trim()) {
      setLocalError('Nickname is required');
      return;
    }
    if (nickname.trim().length > 16) {
      setLocalError('Nickname must be 16 characters or fewer');
      return;
    }
    if (password.length < 6) {
      setLocalError('Password must be at least 6 characters');
      return;
    }
    if (password !== confirm) {
      setLocalError('Passwords do not match');
      return;
    }

    onRegister(nickname.trim(), password);
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
          Create a new account
        </p>

        {displayError && (
          <div className="mb-4 rounded-lg bg-red-500/10 px-3 py-2 text-center text-xs text-red-400">
            {displayError}
          </div>
        )}

        <label className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
          Nickname
        </label>
        <input
          type="text"
          value={nickname}
          onChange={(e) => setNickname(e.target.value)}
          placeholder="1-16 characters"
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
          placeholder="At least 6 characters"
          className="mb-4 w-full rounded-lg border border-light-border bg-white px-3 py-2.5 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
        />

        <label className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
          Confirm password
        </label>
        <input
          type="password"
          value={confirm}
          onChange={(e) => setConfirm(e.target.value)}
          placeholder="Repeat password"
          className="mb-6 w-full rounded-lg border border-light-border bg-white px-3 py-2.5 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
        />

        <button
          type="submit"
          disabled={loading || !nickname || !password || !confirm}
          className="w-full rounded-lg bg-dark-highlight px-4 py-2.5 text-sm font-bold text-white transition-colors hover:bg-[#d63850] active:bg-[#c23045] disabled:cursor-not-allowed disabled:opacity-50"
        >
          {loading ? 'Creating...' : 'Create account'}
        </button>

        <p className="mt-4 text-center text-xs text-light-muted dark:text-dark-muted">
          Already have an account?{' '}
          <button
            type="button"
            onClick={onSwitchToLogin}
            className="font-medium text-dark-highlight hover:underline"
          >
            Back to sign in
          </button>
        </p>
      </form>
    </div>
  );
}

export default RegisterPanel;
