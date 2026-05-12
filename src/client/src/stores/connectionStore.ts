import { create } from 'zustand';
import { invoke } from '@tauri-apps/api/core';

type ConnectionStatus = 'disconnected' | 'connecting' | 'connected';

interface ConnectionState {
  status: ConnectionStatus;
  host: string;
  port: number;
  error: string | null;
  lastHeartbeat: number | null;
  setHost: (host: string) => void;
  setPort: (port: number) => void;
  setDisconnected: () => void;
  connect: (host: string, port: number) => Promise<void>;
  disconnect: () => Promise<void>;
  sendRawJson: (json: string) => Promise<void>;
  updateHeartbeat: () => void;
}

export const useConnectionStore = create<ConnectionState>()((set, get) => ({
  status: 'disconnected',
  host: '127.0.0.1',
  port: 12346,
  error: null,
  lastHeartbeat: null,

  setHost: (host) => set({ host, error: null }),
  setPort: (port) => set({ port, error: null }),
  setDisconnected: () => set({ status: 'disconnected', lastHeartbeat: null }),

  connect: async (host, port) => {
    set({ status: 'connecting', error: null, host, port });
    try {
      await invoke('connect', { host, port });
      set({ status: 'connected' });
    } catch (e) {
      set({ status: 'disconnected', error: String(e) });
    }
  },

  disconnect: async () => {
    try {
      await invoke('disconnect');
    } finally {
      set({ status: 'disconnected', error: null });
    }
  },

  sendRawJson: async (json) => {
    const { status } = get();
    if (status !== 'connected') {
      throw new Error('not connected');
    }
    await invoke('send_raw_json', { json });
  },

  updateHeartbeat: () => set({ lastHeartbeat: Date.now() }),
}));
