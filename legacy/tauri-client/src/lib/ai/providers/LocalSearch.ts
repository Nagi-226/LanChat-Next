import { MsgType, type AIRequestMessage } from '../../../../../../protocol/message_types';
import { useConnectionStore } from '../../../stores/connectionStore';
import type { AIService } from '../types';

export class LocalSearchProvider implements AIService {
  readonly id = 'local-search';
  readonly name = 'Local Search';
  readonly capabilities = ['search'] as const;

  async execute(request: AIRequestMessage) {
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      ...request,
      type: MsgType.AIRequest,
      ai_type: 'search',
    }));
  }
}
