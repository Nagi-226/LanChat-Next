import { create } from 'zustand';
import { persist } from 'zustand/middleware';

type Theme = 'dark' | 'light';

interface UIState {
  theme: Theme;
  sidebarCollapsed: boolean;
  aiPanelOpen: boolean;
  toggleTheme: () => void;
  toggleSidebar: () => void;
  toggleAIPanel: () => void;
}

export const useUIStore = create<UIState>()(
  persist(
    (set) => ({
      theme: 'dark',
      sidebarCollapsed: false,
      aiPanelOpen: false,
      toggleTheme: () => set((state) => ({ theme: state.theme === 'dark' ? 'light' : 'dark' })),
      toggleSidebar: () => set((state) => ({ sidebarCollapsed: !state.sidebarCollapsed })),
      toggleAIPanel: () => set((state) => ({ aiPanelOpen: !state.aiPanelOpen })),
    }),
    { name: 'lanchat-ui' },
  ),
);
