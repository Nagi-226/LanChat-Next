import { describe, expect, it } from 'vitest';
import { createSecureStorage } from './secureStorage';

describe('secure storage bridge', () => {
  it('stores API keys through Tauri commands instead of localStorage', async () => {
    const calls: Array<{ command: string; args?: unknown }> = [];
    const storage = createSecureStorage(async (command, args) => {
      calls.push({ command, args });
      return null;
    });

    await storage.setApiKey('deepseek-api', 'sk-secret');

    expect(calls).toEqual([
      { command: 'secure_store_set', args: { key: 'ai-key:deepseek-api', value: 'sk-secret' } },
    ]);
  });

  it('reads and clears API keys through the same secure namespace', async () => {
    const calls: Array<{ command: string; args?: unknown }> = [];
    const storage = createSecureStorage(async (command, args) => {
      calls.push({ command, args });
      return command === 'secure_store_get' ? 'sk-secret' : null;
    });

    await expect(storage.getApiKey('deepseek-api')).resolves.toBe('sk-secret');
    await storage.deleteApiKey('deepseek-api');

    expect(calls).toEqual([
      { command: 'secure_store_get', args: { key: 'ai-key:deepseek-api' } },
      { command: 'secure_store_delete', args: { key: 'ai-key:deepseek-api' } },
    ]);
  });
});
