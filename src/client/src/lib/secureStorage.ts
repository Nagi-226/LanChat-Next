import { invoke } from '@tauri-apps/api/core';

type InvokeFn = <T = unknown>(command: string, args?: Record<string, unknown>) => Promise<T>;

export interface SecureStorage {
  setApiKey(providerId: string, value: string): Promise<void>;
  getApiKey(providerId: string): Promise<string>;
  deleteApiKey(providerId: string): Promise<void>;
}

function apiKeyName(providerId: string): string {
  return `ai-key:${providerId}`;
}

export function createSecureStorage(invokeFn: InvokeFn): SecureStorage {
  return {
    async setApiKey(providerId, value) {
      await invokeFn('secure_store_set', { key: apiKeyName(providerId), value });
    },
    async getApiKey(providerId) {
      const value = await invokeFn<string | null>('secure_store_get', { key: apiKeyName(providerId) });
      return value ?? '';
    },
    async deleteApiKey(providerId) {
      await invokeFn('secure_store_delete', { key: apiKeyName(providerId) });
    },
  };
}

const tauriStorage = createSecureStorage(invoke);
const volatileFallback = new Map<string, string>();

export const secureStorage: SecureStorage = {
  async setApiKey(providerId, value) {
    try {
      await tauriStorage.setApiKey(providerId, value);
      volatileFallback.delete(providerId);
    } catch {
      volatileFallback.set(providerId, value);
    }
  },
  async getApiKey(providerId) {
    try {
      return await tauriStorage.getApiKey(providerId);
    } catch {
      return volatileFallback.get(providerId) ?? '';
    }
  },
  async deleteApiKey(providerId) {
    try {
      await tauriStorage.deleteApiKey(providerId);
    } finally {
      volatileFallback.delete(providerId);
    }
  },
};
