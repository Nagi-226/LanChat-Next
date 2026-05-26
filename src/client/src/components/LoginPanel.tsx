import { useState } from 'react';
import { motion } from 'framer-motion';
import ClickSpark from '../lib/ClickSpark';
import SpotlightCard from '../lib/SpotlightCard';
import TextType from '../lib/TextType';
import { ButtonSpinner } from '../lib/utils';

interface LoginPanelProps {
  onLogin: (id: number, password: string) => void;
  onSwitchToRegister: () => void;
  error?: string;
  loading?: boolean;
}

const LAST_USER_ID_KEY = 'lanchat-last-user-id';
const LAST_PASSWORD_KEY = 'lanchat-last-password';

function readLastUserId(): string {
  if (typeof window === 'undefined') return '';
  return window.localStorage.getItem(LAST_USER_ID_KEY) || '';
}

function readLastPassword(): string {
  if (typeof window === 'undefined') return '';
  return window.localStorage.getItem(LAST_PASSWORD_KEY) || '';
}

export function LoginPanel({ onLogin, onSwitchToRegister, error, loading }: LoginPanelProps) {
  const [id, setId] = useState(readLastUserId);
  const [password, setPassword] = useState(readLastPassword);
  const [showPassword, setShowPassword] = useState(false);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    const numId = parseInt(id, 10);
    if (Number.isNaN(numId) || !password) return;
    window.localStorage.setItem(LAST_USER_ID_KEY, String(numId));
    window.localStorage.setItem(LAST_PASSWORD_KEY, password);
    onLogin(numId, password);
  };

  return (
    <div className="flex min-h-0 flex-1 items-center justify-center p-6">
      <SpotlightCard className="w-full max-w-sm p-8 shadow-xl">
        <form onSubmit={handleSubmit}>
          <h1 className="mb-1 text-center text-2xl font-bold text-light-text dark:text-dark-text">
            <TextType text="LanChat-Next" loop={false} typingSpeed={40} />
          </h1>
          <p className="mb-6 text-center text-xs text-light-muted dark:text-dark-muted">
            Sign in to your LAN chat account
          </p>

          {error && (
            <div className="mb-4 rounded-lg bg-red-500/10 px-3 py-2 text-center text-xs text-red-400">
              {error}
            </div>
          )}

          <label htmlFor="login-user-id" className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
            User ID
          </label>
          <input
            id="login-user-id"
            type="text"
            value={id}
            onChange={(e) => setId(e.target.value)}
            placeholder="Enter user ID"
            autoFocus
            className="mb-4 w-full rounded-lg border border-light-border bg-white px-3 py-2.5 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
          />

          <label htmlFor="login-password" className="mb-1 block text-xs font-medium text-light-muted dark:text-dark-muted">
            Password
          </label>
          <div className="mb-6 flex rounded-lg border border-light-border bg-white focus-within:border-dark-highlight dark:border-dark-accent dark:bg-dark-accent dark:focus-within:border-dark-highlight">
            <input
              id="login-password"
              type={showPassword ? 'text' : 'password'}
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="Enter password"
              className="min-w-0 flex-1 rounded-l-lg bg-transparent px-3 py-2.5 text-sm text-light-text placeholder-light-muted outline-none dark:text-dark-text dark:placeholder-dark-muted"
            />
            <motion.button
              type="button"
              onClick={() => setShowPassword((value) => !value)}
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              className="rounded-r-lg px-3 text-xs font-medium text-light-muted hover:text-dark-highlight dark:text-dark-muted"
              aria-label={showPassword ? 'Hide password' : 'Show password'}
            >
              {showPassword ? 'Hide' : 'Show'}
            </motion.button>
          </div>

          <ClickSpark className="relative block w-full" sparkColor="#e94560" sparkCount={6}>
            <motion.button
              type="submit"
              disabled={loading || !id || !password}
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              animate={{ opacity: loading ? 0.82 : 1 }}
              className="flex w-full items-center justify-center rounded-lg bg-dark-highlight px-4 py-2.5 text-sm font-bold text-white transition-colors hover:bg-[#d63850] active:bg-[#c23045] disabled:cursor-not-allowed disabled:opacity-50"
            >
              {loading && <ButtonSpinner />}
              {loading ? 'Signing in...' : 'Sign in'}
            </motion.button>
          </ClickSpark>

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
      </SpotlightCard>
    </div>
  );
}

export default LoginPanel;
