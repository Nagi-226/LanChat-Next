import type { AIRequestMessage, AIResponseMessage, AIRequestType } from '../../../../../protocol/message_types';

export type AIStreamChunkCallback = (chunk: string, done: boolean) => void;

export interface AIService {
  readonly id: string;
  readonly name: string;
  readonly capabilities: readonly AIRequestType[];
  execute(request: AIRequestMessage): Promise<AIResponseMessage | void>;
  executeStream?(request: AIRequestMessage, onChunk: AIStreamChunkCallback): Promise<void>;
}

export interface AIProviderConfig {
  providerId: string;
  model?: string;
  apiKeyConfigured?: boolean;
}
