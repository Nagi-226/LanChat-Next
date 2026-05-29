import { create } from 'zustand';
import { persist } from 'zustand/middleware';
import { invoke } from '@tauri-apps/api/core';

type ConnectionStatus = 'disconnected' | 'connecting' | 'connected';
const MAX_RECONNECT_ATTEMPTS = 5;

export interface RecentServer {
  host: string;
  port: number;
}

interface ConnectionState {
  status: ConnectionStatus;
  host: string;
  port: number;
  error: string | null;
  lastHeartbeat: number | null;
  retryCount: number;
  retryTimer: number | null;
  retryDueAt: number | null;
  reconnectTimer: number | null;
  recentServers: RecentServer[];
  setHost: (host: string) => void;
  setPort: (port: number) => void;
  selectServer: (server: RecentServer) => void;
  dismissError: () => void;
  setDisconnected: () => void;
  connect: (host: string, port: number) => Promise<void>;
  disconnect: () => Promise<void>;
  sendRawJson: (json: string) => Promise<void>;
  updateHeartbeat: () => void;
  scheduleReconnect: () => void;
  clearReconnect: () => void;
}

function rememberServer(servers: RecentServer[], server: RecentServer): RecentServer[] {
  const deduped = servers.filter((item) => item.host !== server.host || item.port !== server.port);
  return [server, ...deduped].slice(0, 5);
}

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
      retryDueAt: null,
      reconnectTimer: null,
      recentServers: [],

      setHost: (host) => set({ host, error: null }),
      setPort: (port) => set({ port, error: null }),
      selectServer: (server) => set({ host: server.host, port: server.port, error: null }),
      dismissError: () => set({ error: null }),
      setDisconnected: () => set({ status: 'disconnected', lastHeartbeat: null }),

      connect: async (host, port) => {
        get().clearReconnect();
        set({ status: 'connecting', error: null, host, port });
        try {
          await invoke('connect', { host, port });
          set((s) => ({
            status: 'connected',
            error: null,
            lastHeartbeat: Date.now(),
            retryCount: 0,
            retryTimer: null,
            retryDueAt: null,
            reconnectTimer: null,
            recentServers: rememberServer(s.recentServers, { host, port }),
          }));
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
        if (status === 'connected' || get().reconnectTimer) return;
        if (retryCount >= MAX_RECONNECT_ATTEMPTS) {
          set({
            status: 'disconnected',
            error: `Reconnect stopped after ${MAX_RECONNECT_ATTEMPTS} attempts.`,
            retryTimer: null,
            retryDueAt: null,
            reconnectTimer: null,
          });
          return;
        }

        const delay = Math.min(1000 * 2 ** retryCount, 30000);
        const timer = window.setTimeout(() => {
          set({ reconnectTimer: null, retryTimer: null, retryDueAt: null });
          void get().connect(host, port);
        }, delay);

        set({ retryTimer: delay, retryDueAt: Date.now() + delay, retryCount: retryCount + 1, status: 'connecting', reconnectTimer: timer });
      },

      clearReconnect: () => {
        const { reconnectTimer } = get();
        if (reconnectTimer) {
          window.clearTimeout(reconnectTimer);
        }
        set({ retryTimer: null, retryDueAt: null, reconnectTimer: null });
      },
    }),
    {
      name: 'lanchat-connection',
      partialize: (state) => ({
        host: state.host,
        port: state.port,
        recentServers: state.recentServers,
      }),
    },
  ),
);
