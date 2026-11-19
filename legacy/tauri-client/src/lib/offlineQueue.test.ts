import { beforeEach, describe, expect, it } from 'vitest';
import {
  createQueuedMessage,
  dequeueOfflineMessage,
  loadOfflineQueue,
  saveOfflineQueue,
} from './offlineQueue';

class MemoryStorage implements Pick<Storage, 'getItem' | 'setItem' | 'removeItem'> {
  private values = new Map<string, string>();

  getItem(key: string): string | null {
    return this.values.get(key) ?? null;
  }

  setItem(key: string, value: string): void {
    this.values.set(key, value);
  }

  removeItem(key: string): void {
    this.values.delete(key);
  }
}

describe('offline message queue', () => {
  let storage: MemoryStorage;

  beforeEach(() => {
    storage = new MemoryStorage();
  });

  it('persists queued direct and group messages in FIFO order', () => {
    const first = createQueuedMessage({ kind: 'direct', targetId: 2, localMessageId: 'm1', json: '{"type":5}' });
    const second = createQueuedMessage({ kind: 'group', targetId: 9, localMessageId: 'm2', json: '{"type":16}' });

    saveOfflineQueue(storage, [first, second]);

    expect(loadOfflineQueue(storage).map((item) => item.localMessageId)).toEqual(['m1', 'm2']);
  });

  it('drops a delivered queued message without disturbing the rest', () => {
    const first = createQueuedMessage({ kind: 'direct', targetId: 2, localMessageId: 'm1', json: '{"type":5}' });
    const second = createQueuedMessage({ kind: 'group', targetId: 9, localMessageId: 'm2', json: '{"type":16}' });
    saveOfflineQueue(storage, [first, second]);

    dequeueOfflineMessage(storage, 'm1');

    expect(loadOfflineQueue(storage).map((item) => item.localMessageId)).toEqual(['m2']);
  });

  it('recovers to an empty queue when persisted data is corrupt', () => {
    storage.setItem('lanchat-offline-queue', '{broken');

    expect(loadOfflineQueue(storage)).toEqual([]);
  });
});
