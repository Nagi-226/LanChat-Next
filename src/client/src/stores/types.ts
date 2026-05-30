import type { ChatMessage } from '../components/ChatArea';
import type { UserInfo } from '../../../../protocol/message_types';

export interface CurrentUser {
  id: number;
  nickname: string;
  headId: number;
}

export interface AuthState {
  view: 'login' | 'register';
  loading: boolean;
  error: string | null;
}

export interface FriendRequestInfo extends UserInfo {
  msg?: string;
}

export interface SearchResult {
  id: string;
  messageId: string;
  fromId: number;
  toId?: number;
  groupId?: number;
  content: string;
  contentType: ChatMessage['contentType'];
  timestamp: number;
  sourceName: string;
}
