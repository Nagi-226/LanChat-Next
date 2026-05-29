import { MsgType, type AIRequestMessage } from '../../../../../../protocol/message_types';
import { useConnectionStore } from '../../../stores/connectionStore';
import type { AIService } from '../types';

export class ClaudeAPIProvider implements AIService {
  readonly id = 'claude-api';
  readonly name = 'Claude API via Server';
  readonly capabilities = ['summarize', 'chat', 'translate'] as const;

  async execute(request: AIRequestMessage) {
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      ...request,
      type: MsgType.AIRequest,
    }));
  }
}
