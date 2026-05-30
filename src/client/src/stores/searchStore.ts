import { create } from 'zustand';
import { t } from '../lib/i18n';
import { useConnectionStore } from './connectionStore';
import { useMessageStore } from './messageStore';
import type {
  AIRequestMessage,
  AIResponseMessage,
  AIStreamChunkMessage,
  ProtocolMessage,
  GroupInfo,
} from '../../../../protocol/message_types';
import { MsgType } from '../../../../protocol/message_types';
import type { SearchResult } from './types';
import type { ChatMessage } from '../components/ChatArea';

function getSourceName(item: { fromId: number; groupId?: number }): string {
  const state = useMessageStore.getState();
  if (item.groupId) {
    return state.groups.find((g: GroupInfo) => g.groupId === item.groupId)?.name
      ?? t('contactList.groupFallback', { id: item.groupId });
  }
  return state.contacts.find((c) => c.id === item.fromId)?.nickname
    ?? t('common.unknownUser', { id: item.fromId });
}

function parseSearchPayload(payload: string): SearchResult[] {
  try {
    const parsed = JSON.parse(payload) as { messages?: Record<string, unknown>[] };
    return (parsed.messages || []).map((item, index) => {
      const fromId = Number(item.fromId ?? 0);
      const groupId = item.groupId ? Number(item.groupId) : undefined;
      const messageId = String(item.msg_id ?? `${Date.now()}-${index}`);
      return {
        id: `${messageId}-${index}`,
        messageId,
        fromId,
        toId: item.toId ? Number(item.toId) : undefined,
        groupId,
        content: String(item.msg ?? ''),
        contentType: (item.content_type as ChatMessage['contentType']) ?? 'text',
        timestamp: Number(item.timestamp ?? Date.now()),
        sourceName: getSourceName({ fromId, groupId }),
      };
    });
  } catch {
    return [];
  }
}

interface SearchStore {
  searchResults: SearchResult[];
  isSearching: boolean;
  searchQuery: string;
  searchMessages: (query: string, scope?: 'all' | 'direct' | 'group') => Promise<void>;

  summaryText: string;
  isSummarizing: boolean;
  summarizeConversation: (target: { type: 'direct' | 'group'; id: number; providerId?: string; model?: string }) => Promise<void>;

  handleIncomingAIEvent: (raw: ProtocolMessage) => void;
  resetOnLogout: () => void;
}

export const useSearchStore = create<SearchStore>()((set, get) => ({
  searchResults: [],
  isSearching: false,
  searchQuery: '',
  summaryText: '',
  isSummarizing: false,

  searchMessages: async (query, scope = 'all') => {
    const trimmed = query.trim();
    set({ searchQuery: trimmed, isSearching: Boolean(trimmed), searchResults: trimmed ? get().searchResults : [] });
    if (!trimmed) return;

    const msg: AIRequestMessage & { scope?: string; limit?: number } = {
      type: MsgType.AIRequest,
      request_id: `search-${Date.now()}`,
      ai_type: 'search',
      msg: trimmed,
      scope,
      limit: 50,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
    } catch (e) {
      set({ isSearching: false });
      throw e;
    }
  },

  summarizeConversation: async (target) => {
    set({ isSummarizing: true, summaryText: '' });
    const msg: AIRequestMessage & { provider?: string; model?: string } = {
      type: MsgType.AIRequest,
      request_id: `summarize-${Date.now()}`,
      ai_type: 'summarize',
      msg: `${target.type}:${target.id}`,
      provider: target.providerId,
      model: target.model,
    };
    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
    } catch (e) {
      set({ isSummarizing: false, summaryText: String(e) });
      throw e;
    }
  },

  handleIncomingAIEvent: (raw: ProtocolMessage) => {
    switch (raw.type) {
      case MsgType.AIResponse: {
        const response = raw as AIResponseMessage;
        if (response.request_id.startsWith('search-')) {
          set({
            isSearching: false,
            searchResults: response.status === 'done' && response.msg ? parseSearchPayload(response.msg) : [],
          });
        } else if (response.request_id.startsWith('summarize-') && response.status === 'start') {
          set({ isSummarizing: true, summaryText: '' });
        } else if (response.request_id.startsWith('summarize-')) {
          set((s) => ({ isSummarizing: false, summaryText: response.msg ?? s.summaryText }));
        }
        break;
      }

      case MsgType.AIStreamChunk: {
        const chunk = raw as AIStreamChunkMessage;
        if (chunk.request_id.startsWith('summarize-')) {
          set((s) => ({
            summaryText: `${s.summaryText}${chunk.msg}`,
            isSummarizing: !chunk.is_final,
          }));
        }
        break;
      }

      default:
        break;
    }
  },

  resetOnLogout: () => set({
    searchResults: [], isSearching: false, searchQuery: '',
    summaryText: '', isSummarizing: false,
  }),
}));
