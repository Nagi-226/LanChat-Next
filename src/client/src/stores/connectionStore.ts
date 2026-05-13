import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import { invoke } from '@tauri-apps/api/core';

type ConnectionStatus = 'disconnected' | 'connecting' | 'connected';

interface ConnectionState {
  status: ConnectionStatus;
  host: string;
  port: number;
  error: string | null;
  lastHeartbeat: number | null;
  retryCount: number;
  retryTimer: number | null;
  setHost: (host: string) => void;
  setPort: (port: number) => void;
  setDisconnected: () => void;
  connect: (host: string, port: number) => Promise<void>;
  disconnect: () => Promise<void>;
  sendRawJson: (json: string) => Promise<void>;
  updateHeartbeat: () => void;
  scheduleReconnect: () => void;
  clearReconnect: () => void;
}

let reconnectTimer: ReturnType<typeof setTimeout> | null = null;

export const useConnectionStore = create<ConnectionState>()(
  persist(
    (set, get) => ({
      status: 'disconnected',
      host: '127.0.0.1',
      port: 12346,
      error: null,
      lastHeartbeat: null,
      retryCount: 0,
      retryTimer: null,

      setHost: (host) => set({ host, error: null }),
      setPort: (port) => set({ port, error: null }),
      setDisconnected: () => set({ status: 'disconnected', lastHeartbeat: null, retryCount: 0 }),

      connect: async (host, port) => {
        get().clearReconnect();
        set({ status: 'connecting', error: null, host, port });
        try {
          await invoke('connect', { host, port });
          set({ status: 'connected', error: null, lastHeartbeat: Date.now(), retryCount: 0 });
        } catch (e) {
          set({ status: 'disconnected', error: String(e), lastHeartbeat: null });
          get().scheduleReconnect();
        }
      },

      disconnect: async () => {
        get().clearReconnect();
        try {
          await invoke('disconnect');
        } finally {
          set({ status: 'disconnected', error: null, lastHeartbeat: null, retryCount: 0 });
        }
      },

      sendRawJson: async (json) => {
        const { status } = get();
        if (status !== 'connected') {
          throw new Error('not connected');
        }
        await invoke('send_raw_json', { json });
      },

      updateHeartbeat: () => set({ lastHeartbeat: Date.now(), retryCount: 0 }),

      scheduleReconnect: () => {
        const { status, host, port, retryCount } = get();
        if (status === 'connected' || reconnectTimer) return;

        const delay = Math.min(1000 * 2 ** retryCount, 30000);
        set({ retryTimer: delay, retryCount: retryCount + 1, status: 'connecting' });

        reconnectTimer = setTimeout(() => {
          reconnectTimer = null;
          void get().connect(host, port);
        }, delay);
      },

      clearReconnect: () => {
        if (reconnectTimer) {
          clearTimeout(reconnectTimer);
          reconnectTimer = null;
        }
        set({ retryTimer: null });
      },
    }),
    {
      name: 'lanchat-connection',
      partialize: (state) => ({ host: state.host, port: state.port }),
    },
  ),
);
