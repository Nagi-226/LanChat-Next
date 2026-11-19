export type QueuedMessageKind = 'direct' | 'group';

export interface QueuedMessage {
  kind: QueuedMessageKind;
  targetId: number;
  localMessageId: string;
  json: string;
  createdAt: number;
  attempts: number;
}

export const OFFLINE_QUEUE_KEY = 'lanchat-offline-queue';

type QueueStorage = Pick<Storage, 'getItem' | 'setItem' | 'removeItem'>;

export function createQueuedMessage(input: Omit<QueuedMessage, 'createdAt' | 'attempts'>): QueuedMessage {
  return {
    ...input,
    createdAt: Date.now(),
    attempts: 0,
  };
}

function isQueuedMessage(value: unknown): value is QueuedMessage {
  if (!value || typeof value !== 'object') return false;
  const item = value as Partial<QueuedMessage>;
  return (
    (item.kind === 'direct' || item.kind === 'group') &&
    typeof item.targetId === 'number' &&
    typeof item.localMessageId === 'string' &&
    typeof item.json === 'string' &&
    typeof item.createdAt === 'number' &&
    typeof item.attempts === 'number'
  );
}

export function loadOfflineQueue(storage: QueueStorage = window.localStorage): QueuedMessage[] {
  const raw = storage.getItem(OFFLINE_QUEUE_KEY);
  if (!raw) return [];
  try {
    const parsed = JSON.parse(raw) as unknown;
    return Array.isArray(parsed) ? parsed.filter(isQueuedMessage) : [];
  } catch {
    return [];
  }
}

export function saveOfflineQueue(storage: QueueStorage = window.localStorage, queue: QueuedMessage[]): void {
  if (queue.length === 0) {
    storage.removeItem(OFFLINE_QUEUE_KEY);
    return;
  }
  storage.setItem(OFFLINE_QUEUE_KEY, JSON.stringify(queue));
}

export function enqueueOfflineMessage(message: QueuedMessage, storage: QueueStorage = window.localStorage): QueuedMessage[] {
  const queue = [...loadOfflineQueue(storage), message];
  saveOfflineQueue(storage, queue);
  return queue;
}

export function dequeueOfflineMessage(storage: QueueStorage = window.localStorage, localMessageId: string): QueuedMessage[] {
  const queue = loadOfflineQueue(storage).filter((item) => item.localMessageId !== localMessageId);
  saveOfflineQueue(storage, queue);
  return queue;
}
