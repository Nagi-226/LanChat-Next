import { create } from 'zustand';
import { t } from '../lib/i18n';
import { useConnectionStore } from './connectionStore';
import { useMessageStore } from './messageStore';
import { useFriendStore } from './friendStore';
import { useSearchStore } from './searchStore';
import type { LoginMessage, RegisterUserMessage } from '../../../../protocol/message_types';
import { MsgType } from '../../../../protocol/message_types';
import type { AuthState, CurrentUser } from './types';

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
    setAuthError(null);

    const timeoutId = window.setTimeout(() => {
      const state = get();
      if (state.auth.loading) {
        state.setAuthError(t('store.loginTimeout'));
      }
    }, 12000);

    const loginMsg: LoginMessage = {
      type: MsgType.Login,
      id,
      password,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(loginMsg));
    } catch (e) {
      window.clearTimeout(timeoutId);
      setAuthError(String(e));
    }
  },

  register: async (nickname, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    setAuthError(null);

    const timeoutId = window.setTimeout(() => {
      const state = get();
      if (state.auth.loading) {
        state.setAuthError(t('store.registerTimeout'));
      }
    }, 12000);

    const regMsg: RegisterUserMessage = {
      type: MsgType.RegisterUser,
      password,
      nickname,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(regMsg));
    } catch (e) {
      window.clearTimeout(timeoutId);
      setAuthError(String(e));
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
