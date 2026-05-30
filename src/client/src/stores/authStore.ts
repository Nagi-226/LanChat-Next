import { create } from 'zustand';
import { t } from '../lib/i18n';
import { useConnectionStore } from './connectionStore';
import { useMessageStore } from './messageStore';
import { useFriendStore } from './friendStore';
import { useSearchStore } from './searchStore';
import type { LoginMessage, RegisterUserMessage } from '../../../../protocol/message_types';
import { MsgType } from '../../../../protocol/message_types';
import type { AuthState, CurrentUser } from './types';

const LAST_PASSWORD_KEY = 'lanchat-last-password';
let authRequestSeq = 0;
let authTimeoutId: number | null = null;

function beginAuthRequest(setAuthError: (error: string | null) => void, timeoutMessage: string): number {
  if (authTimeoutId !== null) {
    window.clearTimeout(authTimeoutId);
  }

  const requestSeq = ++authRequestSeq;
  authTimeoutId = window.setTimeout(() => {
    const state = useAuthStore.getState();
    if (requestSeq === authRequestSeq && state.auth.loading) {
      state.setAuthError(timeoutMessage);
    }
  }, 12000);

  setAuthError(null);
  return requestSeq;
}

async function ensureConnected(): Promise<void> {
  const connection = useConnectionStore.getState();
  if (connection.status === 'connected') return;
  if (connection.status === 'connecting') {
    throw new Error(t('connectionBar.connecting'));
  }

  await connection.connect(connection.host, connection.port);

  if (useConnectionStore.getState().status !== 'connected') {
    throw new Error(useConnectionStore.getState().error || t('store.notConnected'));
  }
}

interface AuthStore {
  auth: AuthState;
  setAuthView: (view: 'login' | 'register') => void;
  setAuthLoading: (loading: boolean) => void;
  setAuthError: (error: string | null) => void;

  currentUser: CurrentUser | null;
  setCurrentUser: (user: CurrentUser | null) => void;

  login: (id: number, password: string) => Promise<void>;
  register: (nickname: string, password: string) => Promise<void>;
  logout: () => void;
}

export const useAuthStore = create<AuthStore>()((set, get) => ({
  auth: { view: 'login', loading: false, error: null },
  setAuthView: (view) => set((s) => ({ auth: { ...s.auth, view, error: null } })),
  setAuthLoading: (loading) => set((s) => ({ auth: { ...s.auth, loading } })),
  setAuthError: (error) => set((s) => ({ auth: { ...s.auth, error, loading: false } })),

  currentUser: null,
  setCurrentUser: (user) => set({ currentUser: user }),

  login: async (id, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    const requestSeq = beginAuthRequest(setAuthError, t('store.loginTimeout'));

    const loginMsg: LoginMessage = {
      type: MsgType.Login,
      id,
      password,
    };

    try {
      await ensureConnected();
      await useConnectionStore.getState().sendRawJson(JSON.stringify(loginMsg));
    } catch (e) {
      if (requestSeq === authRequestSeq) {
        setAuthError(String(e));
      }
    }
  },

  register: async (nickname, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    const requestSeq = beginAuthRequest(setAuthError, t('store.registerTimeout'));

    const regMsg: RegisterUserMessage = {
      type: MsgType.RegisterUser,
      password,
      nickname,
    };

    try {
      await ensureConnected();
      await useConnectionStore.getState().sendRawJson(JSON.stringify(regMsg));
      window.localStorage.setItem(LAST_PASSWORD_KEY, password);
    } catch (e) {
      if (requestSeq === authRequestSeq) {
        setAuthError(String(e));
      }
    }
  },

  logout: () => {
    set({
      currentUser: null,
      auth: { view: 'login', loading: false, error: null },
    });
    useMessageStore.getState().resetOnLogout();
    useFriendStore.getState().resetOnLogout();
    useSearchStore.getState().resetOnLogout();
  },
}));
