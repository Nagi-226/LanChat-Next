import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import type { Language } from '../lib/i18n';

type Theme = 'dark' | 'light';

interface UIState {
  theme: Theme;
  sidebarCollapsed: boolean;
  aiPanelOpen: boolean;
  language: Language;
  toggleTheme: () => void;
  toggleSidebar: () => void;
  toggleAIPanel: () => void;
  setLanguage: (lang: Language) => void;
}

export const useUIStore = create<UIState>()(
  persist(
    (set) => ({
      theme: 'dark',
      sidebarCollapsed: false,
      aiPanelOpen: false,
      language: 'en',
      toggleTheme: () => set((state) => ({ theme: state.theme === 'dark' ? 'light' : 'dark' })),
      toggleSidebar: () => set((state) => ({ sidebarCollapsed: !state.sidebarCollapsed })),
      toggleAIPanel: () => set((state) => ({ aiPanelOpen: !state.aiPanelOpen })),
      setLanguage: (lang) => set({ language: lang }),
    }),
    { name: 'lanchat-ui' },
  ),
);
