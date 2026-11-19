import type { AIRequestMessage, AIResponseMessage } from '../../../../../../protocol/message_types';
import { MsgType } from '../../../../../../protocol/message_types';
import type { AIService, AIStreamChunkCallback } from '../types';
import { secureStorage } from '../../secureStorage';

const DEEPSEEK_BASE = 'https://api.deepseek.com';

async function getKey(): Promise<string> {
  return secureStorage.getApiKey('deepseek-api');
}

function getModel(): string {
  return window.localStorage.getItem('lanchat-ai-model') || 'deepseek-chat';
}

function buildSystemPrompt(aiType: string): string {
  switch (aiType) {
    case 'summarize':
      return 'You are a helpful assistant. Summarize the following conversation concisely. Focus on key topics, decisions, and action items. Keep it under 300 words.';
    case 'translate':
      return 'You are a professional translator. Translate the following text accurately while preserving tone and formatting.';
    case 'search':
      return 'You are a helpful assistant. Answer the user\'s question based on the provided context. Be concise and accurate.';
    case 'chat':
    default:
      return 'You are a helpful assistant integrated in a LAN chat application. Keep responses concise and friendly.';
  }
}

export class DeepSeekAPIProvider implements AIService {
  readonly id = 'deepseek-api';
  readonly name = 'DeepSeek API';
  readonly capabilities = ['summarize', 'chat', 'translate', 'search'] as const;

  async execute(request: AIRequestMessage): Promise<AIResponseMessage | void> {
    const key = await getKey();
    if (!key) {
      return {
        type: MsgType.AIResponse,
        request_id: request.request_id,
        status: 'error',
        msg: 'DeepSeek API key not configured. Please set your API key in the AI panel.',
      };
    }

    const model = getModel();
    const aiType = request.ai_type || 'chat';

    const response = await fetch(`${DEEPSEEK_BASE}/chat/completions`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${key}`,
      },
      body: JSON.stringify({
        model,
        messages: [
          { role: 'system', content: buildSystemPrompt(aiType) },
          { role: 'user', content: request.msg },
        ],
        max_tokens: 2048,
        temperature: 0.7,
      }),
    });

    if (!response.ok) {
      const errorText = await response.text().catch(() => 'unknown error');
      return {
        type: MsgType.AIResponse,
        request_id: request.request_id,
        status: 'error',
        msg: `DeepSeek API error (${response.status}): ${errorText}`,
      };
    }

    const json = await response.json() as {
      choices?: Array<{ message?: { content?: string } }>;
    };

    return {
      type: MsgType.AIResponse,
      request_id: request.request_id,
      status: 'ok',
      msg: json.choices?.[0]?.message?.content ?? 'No response',
    };
  }

  async executeStream(
    request: AIRequestMessage,
    onChunk: AIStreamChunkCallback,
  ): Promise<void> {
    const key = await getKey();
    if (!key) {
      onChunk('DeepSeek API key not configured.', true);
      return;
    }

    const model = getModel();
    const aiType = request.ai_type || 'chat';

    const response = await fetch(`${DEEPSEEK_BASE}/chat/completions`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${key}`,
      },
      body: JSON.stringify({
        model,
        messages: [
          { role: 'system', content: buildSystemPrompt(aiType) },
          { role: 'user', content: request.msg },
        ],
        max_tokens: 2048,
        temperature: 0.7,
        stream: true,
      }),
    });

    if (!response.ok) {
      onChunk(`DeepSeek API error (${response.status})`, true);
      return;
    }

    const reader = response.body?.getReader();
    if (!reader) {
      onChunk('No response body', true);
      return;
    }

    const decoder = new TextDecoder();
    let buffer = '';

    try {
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() ?? '';

        for (const line of lines) {
          const trimmed = line.trim();
          if (!trimmed.startsWith('data: ')) continue;
          const data = trimmed.slice(6);
          if (data === '[DONE]') {
            onChunk('', true);
            return;
          }
          try {
            const parsed = JSON.parse(data) as {
              choices?: Array<{ delta?: { content?: string } }>;
            };
            const content = parsed.choices?.[0]?.delta?.content ?? '';
            if (content) onChunk(content, false);
          } catch {
            // skip unparseable chunks
          }
        }
      }
    } finally {
      reader.releaseLock();
    }

    onChunk('', true);
  }
}
