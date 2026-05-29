import { create } from 'zustand';
import type { ChatMessage } from '../components/ChatArea';
import type { Contact } from '../components/ContactList';
import type {
  LoginMessage,
  LoginSuccessReturnMessage,
  RegisterUserMessage,
  SendMsgMessage,
  ReceiveMsgMessage,
  SendGroupMsgMessage,
  ReceiveGroupMsgMessage,
  UserJoinGroupMessage,
  UserLeaveGroupMessage,
  UserInfo,
  GroupInfo,
  ProtocolMessage,
  AIRequestMessage,
  AIResponseMessage,
  AIStreamChunkMessage,
  FriendRequestMessage,
  FriendRequestAckMessage,
  FriendAcceptReturnMessage,
  FriendRemoveReturnMessage,
  FriendListReturnMessage,
  FriendOnlineMessage,
  RequestHistoryMessage,
  HistoryResponseMessage,
} from '../../../../protocol/message_types';
import { MsgType, isValidMsgType } from '../../../../protocol/message_types';
import { useConnectionStore } from './connectionStore';

interface CurrentUser {
  id: number;
  nickname: string;
  headId: number;
}

interface AuthState {
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

interface ChatState {
  auth: AuthState;
  setAuthView: (view: 'login' | 'register') => void;
  setAuthLoading: (loading: boolean) => void;
  setAuthError: (error: string | null) => void;

  currentUser: CurrentUser | null;
  setCurrentUser: (user: CurrentUser | null) => void;

  contacts: Contact[];
  setContacts: (contacts: Contact[]) => void;

  messagesByContact: Record<number, ChatMessage[]>;
  activeContactId: number | null;
  setActiveContact: (id: number | null) => void;

  groups: GroupInfo[];
  messagesByGroup: Record<number, ChatMessage[]>;
  activeGroupId: number | null;
  selectGroup: (id: number | null) => void;

  login: (id: number, password: string) => Promise<void>;
  register: (nickname: string, password: string) => Promise<void>;
  sendPrivateMessage: (toId: number, content: string) => Promise<void>;
  sendGroupMessage: (groupId: number, content: string) => Promise<void>;
  searchResults: SearchResult[];
  isSearching: boolean;
  searchQuery: string;
  searchMessages: (query: string, scope?: 'all' | 'direct' | 'group') => Promise<void>;
  requestHistory: (target: { type: 'direct' | 'group'; id: number; limit?: number }) => Promise<void>;
  summaryText: string;
  isSummarizing: boolean;
  summarizeConversation: (target: { type: 'direct' | 'group'; id: number; providerId?: string; model?: string }) => Promise<void>;
  friends: Contact[];
  friendRequests: FriendRequestInfo[];
  sendFriendRequest: (toId: number, msg?: string) => Promise<void>;
  respondToRequest: (fromId: number, accept: boolean) => Promise<void>;
  removeFriend: (friendId: number) => Promise<void>;
  handleIncomingMessage: (msg: ProtocolMessage) => void;
  logout: () => void;
}

function msgId(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

function toContacts(friends: UserInfo[]): Contact[] {
  return friends.map((friend) => ({
    id: friend.id,
    nickname: friend.nickname,
    headId: friend.headId,
    status: 'offline' as const,
    unread: 0,
  }));
}

function toChatMessage(
  raw: Partial<ReceiveMsgMessage | ReceiveGroupMsgMessage> & { fromId?: number; msg?: string; nickname?: string; timestamp?: number; msg_id?: string },
): ChatMessage {
  return {
    id: raw.msg_id || msgId(),
    fromId: raw.fromId ?? 0,
    nickname: raw.nickname ?? 'Unknown',
    content: raw.msg ?? '',
    contentType: raw.content_type ?? 'text',
    timestamp: raw.timestamp ?? Date.now(),
    status: 'sent',
  };
}

function systemMessage(content: string): ChatMessage {
  return {
    id: msgId(),
    fromId: 0,
    nickname: 'System',
    content,
    contentType: 'system',
    timestamp: Date.now(),
    status: 'sent',
  };
}

function markActiveContactSelected(state: ChatState, contactId: number): Partial<ChatState> {
  return {
    activeContactId: contactId,
    activeGroupId: null,
    contacts: state.contacts.map((c) => (c.id === contactId ? { ...c, unread: 0 } : c)),
  };
}

function selectFirstContact(contacts: Contact[]): number | null {
  return contacts[0]?.id ?? null;
}

function updateMessageStatus(messages: ChatMessage[] | undefined, id: string, status: NonNullable<ChatMessage['status']>): ChatMessage[] {
  return (messages || []).map((message) => (message.id === id ? { ...message, status } : message));
}

function sourceNameFor(item: { fromId: number; groupId?: number }, contacts: Contact[], groups: GroupInfo[]): string {
  if (item.groupId) {
    return groups.find((group) => group.groupId === item.groupId)?.name ?? `Group #${item.groupId}`;
  }
  return contacts.find((contact) => contact.id === item.fromId)?.nickname ?? `User #${item.fromId}`;
}

function parseSearchPayload(payload: string, contacts: Contact[], groups: GroupInfo[]): SearchResult[] {
  try {
    const parsed = JSON.parse(payload) as { messages?: Record<string, unknown>[] };
    return (parsed.messages || []).map((item, index) => {
      const fromId = Number(item.fromId ?? 0);
      const toId = item.toId ? Number(item.toId) : undefined;
      const groupId = item.groupId ? Number(item.groupId) : undefined;
      const messageId = String(item.msg_id ?? `${Date.now()}-${index}`);
      return {
        id: `${messageId}-${index}`,
        messageId,
        fromId,
        toId,
        groupId,
        content: String(item.msg ?? ''),
        contentType: (item.content_type as ChatMessage['contentType']) ?? 'text',
        timestamp: Number(item.timestamp ?? Date.now()),
        sourceName: sourceNameFor({ fromId, groupId }, contacts, groups),
      };
    });
  } catch {
    return [];
  }
}

function upsertGroupUser(groups: GroupInfo[], groupId: number, user: UserInfo): GroupInfo[] {
  return groups.map((group) => {
    if (group.groupId !== groupId) return group;
    const users = group.users || [];
    const exists = users.some((item) => item.id === user.id);
    return {
      ...group,
      users: exists ? users.map((item) => (item.id === user.id ? { ...item, ...user } : item)) : [...users, user],
    };
  });
}

function removeGroupUser(groups: GroupInfo[], groupId: number, userId: number): GroupInfo[] {
  return groups.map((group) => {
    if (group.groupId !== groupId) return group;
    return { ...group, users: (group.users || []).filter((item) => item.id !== userId) };
  });
}

export const useChatStore = create<ChatState>()((set, get) => ({
  auth: { view: 'login', loading: false, error: null },
  setAuthView: (view) => set((s) => ({ auth: { ...s.auth, view, error: null } })),
  setAuthLoading: (loading) => set((s) => ({ auth: { ...s.auth, loading } })),
  setAuthError: (error) => set((s) => ({ auth: { ...s.auth, error, loading: false } })),

  currentUser: null,
  setCurrentUser: (user) => set({ currentUser: user }),

  contacts: [],
  setContacts: (contacts) => set({ contacts }),

  messagesByContact: {},
  activeContactId: null,
  setActiveContact: (id) =>
    set((state) => {
      if (id === null) return { activeContactId: null };
      return markActiveContactSelected(state, id);
    }),

  groups: [],
  messagesByGroup: {},
  activeGroupId: null,
  selectGroup: (id) => set({ activeGroupId: id, activeContactId: null }),
  searchResults: [],
  isSearching: false,
  searchQuery: '',
  summaryText: '',
  isSummarizing: false,
  friends: [],
  friendRequests: [],

  login: async (id, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    setAuthError(null);

    const timeoutId = window.setTimeout(() => {
      const state = get();
      if (state.auth.loading) {
        state.setAuthError('Login timed out. Check server connection.');
      }
    }, 12000);

    const loginMsg: LoginMessage = {
      type: MsgType.Login,
      id,
      password,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(loginMsg));
    } catch (e) {
      window.clearTimeout(timeoutId);
      setAuthError(String(e));
    }
  },

  register: async (nickname, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    setAuthError(null);

    const timeoutId = window.setTimeout(() => {
      const state = get();
      if (state.auth.loading) {
        state.setAuthError('Registration timed out. Check server connection.');
      }
    }, 12000);

    const regMsg: RegisterUserMessage = {
      type: MsgType.RegisterUser,
      password,
      nickname,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(regMsg));
    } catch (e) {
      window.clearTimeout(timeoutId);
      setAuthError(String(e));
    }
  },

  sendPrivateMessage: async (toId, content) => {
    const user = get().currentUser;
    if (!user) return;

    const msg: SendMsgMessage = {
      type: MsgType.SendMsg,
      fromId: user.id,
      toId,
      msg: content,
    };

    const localId = msgId();
    const chatMsg: ChatMessage = {
      id: localId,
      fromId: user.id,
      nickname: user.nickname,
      content,
      contentType: 'text',
      timestamp: Date.now(),
      status: 'sending',
    };

    set((s) => ({
      messagesByContact: {
        ...s.messagesByContact,
        [toId]: [...(s.messagesByContact[toId] || []), chatMsg],
      },
      contacts: s.contacts.map((c) => (c.id === toId ? { ...c, lastMessage: content } : c)),
    }));

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
      set((s) => ({
        messagesByContact: {
          ...s.messagesByContact,
          [toId]: updateMessageStatus(s.messagesByContact[toId], localId, 'sent'),
        },
      }));
    } catch (e) {
      set((s) => ({
        messagesByContact: {
          ...s.messagesByContact,
          [toId]: updateMessageStatus(s.messagesByContact[toId], localId, 'failed'),
        },
      }));
      throw e;
    }
  },

  sendGroupMessage: async (groupId, content) => {
    const user = get().currentUser;
    if (!user) return;

    const msg: SendGroupMsgMessage = {
      type: MsgType.SendGroupMsg,
      fromId: user.id,
      groupId,
      msg: content,
    };

    const localId = msgId();
    const chatMsg: ChatMessage = {
      id: localId,
      fromId: user.id,
      nickname: user.nickname,
      content,
      contentType: 'text',
      timestamp: Date.now(),
      status: 'sending',
    };

    set((s) => ({
      messagesByGroup: {
        ...s.messagesByGroup,
        [groupId]: [...(s.messagesByGroup[groupId] || []), chatMsg],
      },
    }));

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
      set((s) => ({
        messagesByGroup: {
          ...s.messagesByGroup,
          [groupId]: updateMessageStatus(s.messagesByGroup[groupId], localId, 'sent'),
        },
      }));
    } catch (e) {
      set((s) => ({
        messagesByGroup: {
          ...s.messagesByGroup,
          [groupId]: updateMessageStatus(s.messagesByGroup[groupId], localId, 'failed'),
        },
      }));
      throw e;
    }
  },

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

  requestHistory: async (target) => {
    const user = get().currentUser;
    if (!user) return;
    const msg: RequestHistoryMessage = {
      type: MsgType.RequestHistory,
      id: user.id,
      limit: target.limit ?? 100,
      ...(target.type === 'group' ? { groupId: target.id } : { toId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
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

  sendFriendRequest: async (toId, msg = '') => {
    const user = get().currentUser;
    if (!user) return;
    const request: FriendRequestMessage = {
      type: MsgType.FriendRequest,
      fromId: user.id,
      toId,
      msg,
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(request));
  },

  respondToRequest: async (fromId, accept) => {
    const user = get().currentUser;
    if (!user) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      type: accept ? MsgType.FriendAccept : MsgType.FriendRemove,
      fromId,
      toId: user.id,
    }));
  },

  removeFriend: async (friendId) => {
    const user = get().currentUser;
    if (!user) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      type: MsgType.FriendRemove,
      fromId: user.id,
      toId: friendId,
    }));
  },

  handleIncomingMessage: (raw) => {
    if (!isValidMsgType(raw.type)) return;

    switch (raw.type) {
      case MsgType.LoginSuccessReturn: {
        const login = raw as LoginSuccessReturnMessage;
        const friends = toContacts(login.friends || []);
        const contacts = toContacts(login.users || login.friends || []);
        const firstContactId = selectFirstContact(contacts);
        set({
          currentUser: {
            id: login.id ?? 0,
            nickname: login.nickname ?? '',
            headId: login.headId ?? 0,
          },
          contacts,
          friends,
          groups: login.groups || [],
          messagesByGroup: {},
          activeContactId: firstContactId,
          activeGroupId: null,
          auth: { view: 'login', loading: false, error: null },
        });
        break;
      }

      case MsgType.LoginFailedReturn: {
        const errorMsg = 'msg' in raw ? raw.msg : 'Login failed';
        set((s) => ({ auth: { ...s.auth, loading: false, error: errorMsg || 'Login failed' } }));
        break;
      }

      case MsgType.RegisterUserReturn: {
        set((s) => ({
          auth: {
            ...s.auth,
            loading: false,
            error: null,
            view: 'login',
          },
        }));
        break;
      }

      case MsgType.ReceiveMsg: {
        const incoming = raw as ReceiveMsgMessage;
        const fromId = incoming.fromId ?? 0;
        const chatMsg = toChatMessage(incoming);
        set((s) => ({
          messagesByContact: {
            ...s.messagesByContact,
            [fromId]: [...(s.messagesByContact[fromId] || []), chatMsg],
          },
          contacts: s.contacts.map((c) =>
            c.id === fromId
              ? {
                  ...c,
                  unread: s.activeContactId === fromId ? c.unread : c.unread + 1,
                  lastMessage: incoming.msg ?? c.lastMessage,
                }
              : c,
          ),
        }));
        break;
      }

      case MsgType.ReceiveGroupMsg: {
        const incoming = raw as ReceiveGroupMsgMessage;
        const groupId = incoming.groupId ?? 0;
        const chatMsg = toChatMessage(incoming);
        set((s) => ({
          messagesByGroup: {
            ...s.messagesByGroup,
            [groupId]: [...(s.messagesByGroup[groupId] || []), chatMsg],
          },
        }));
        break;
      }

      case MsgType.HistoryResponse: {
        const history = raw as HistoryResponseMessage;
        const messages = (history.messages || []).map((item) => toChatMessage(item as Partial<ReceiveMsgMessage | ReceiveGroupMsgMessage>));
        const first = history.messages?.[0] as { fromId?: number; toId?: number; groupId?: number } | undefined;
        if (!first || messages.length === 0) break;
        const mergeById = (existing: ChatMessage[]) => {
          const byId = new Map(existing.map((message) => [message.id, message]));
          for (const message of messages) byId.set(message.id, message);
          return [...byId.values()].sort((a, b) => a.timestamp - b.timestamp);
        };
        if (first.groupId) {
          set((s) => ({
            messagesByGroup: {
              ...s.messagesByGroup,
              [first.groupId as number]: mergeById(s.messagesByGroup[first.groupId as number] || []),
            },
          }));
        } else {
          const currentUserId = get().currentUser?.id ?? 0;
          const peerId = first.fromId === currentUserId ? first.toId : first.fromId;
          if (peerId) {
            set((s) => ({
              messagesByContact: {
                ...s.messagesByContact,
                [peerId]: mergeById(s.messagesByContact[peerId] || []),
              },
            }));
          }
        }
        break;
      }

      case MsgType.UserJoinGroup: {
        const incoming = raw as UserJoinGroupMessage;
        const nickname = incoming.nickname || `User ${incoming.id}`;
        set((s) => ({
          groups: upsertGroupUser(s.groups, incoming.groupId, {
            id: incoming.id,
            nickname,
            headId: incoming.headId,
          }),
          messagesByGroup: {
            ...s.messagesByGroup,
            [incoming.groupId]: [...(s.messagesByGroup[incoming.groupId] || []), systemMessage(`${nickname} joined the group`)],
          },
        }));
        break;
      }

      case MsgType.UserLeaveGroup: {
        const incoming = raw as UserLeaveGroupMessage;
        const nickname = incoming.nickname || `User ${incoming.id}`;
        set((s) => ({
          groups: removeGroupUser(s.groups, incoming.groupId, incoming.id),
          messagesByGroup: {
            ...s.messagesByGroup,
            [incoming.groupId]: [...(s.messagesByGroup[incoming.groupId] || []), systemMessage(`${nickname} left the group`)],
          },
        }));
        break;
      }

      case MsgType.UserOnline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) => (c.id === uid ? { ...c, status: 'online' as const } : c)),
        }));
        break;
      }

      case MsgType.UserOffline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) => (c.id === uid ? { ...c, status: 'offline' as const } : c)),
        }));
        break;
      }

      case MsgType.AIResponse: {
        const response = raw as AIResponseMessage;
        if (response.request_id.startsWith('search-')) {
          set((s) => ({
            isSearching: false,
            searchResults: response.status === 'done' && response.msg ? parseSearchPayload(response.msg, s.contacts, s.groups) : [],
          }));
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

      case MsgType.FriendRequest: {
        const request = raw as FriendRequestMessage;
        const fromId = request.fromId ?? 0;
        const fromUser = get().contacts.find((contact) => contact.id === fromId);
        set((s) => ({
          friendRequests: [
            ...s.friendRequests.filter((request) => request.id !== fromId),
            {
              id: fromId,
              nickname: fromUser?.nickname ?? `User #${fromId}`,
              headId: fromUser?.headId,
              msg: request.msg,
            },
          ],
        }));
        break;
      }

      case MsgType.FriendRequestAck: {
        const ack = raw as FriendRequestAckMessage;
        if (ack.status === 'error' || ack.status === 'failed') {
          set((s) => ({ auth: { ...s.auth, error: ack.msg ?? 'Friend request failed' } }));
        }
        break;
      }

      case MsgType.FriendAcceptReturn: {
        const accepted = raw as FriendAcceptReturnMessage;
        if (accepted.status === 'ok') {
          set((s) => {
            const existing = s.contacts.find((contact) => contact.id === accepted.friendId);
            const contact: Contact = existing ?? {
              id: accepted.friendId,
              nickname: accepted.nickname ?? `User #${accepted.friendId}`,
              headId: accepted.headId,
              status: 'offline',
              unread: 0,
            };
            return {
              contacts: existing ? s.contacts : [...s.contacts, contact],
              friends: [...s.friends.filter((friend) => friend.id !== contact.id), contact],
              friendRequests: s.friendRequests.filter((request) => request.id !== contact.id),
            };
          });
        }
        break;
      }

      case MsgType.FriendRemoveReturn: {
        const removed = raw as FriendRemoveReturnMessage & { friendId?: number };
        if (removed.status === 'ok' && removed.friendId) {
          set((s) => ({
            friends: s.friends.filter((friend) => friend.id !== removed.friendId),
            friendRequests: s.friendRequests.filter((request) => request.id !== removed.friendId),
          }));
        }
        break;
      }

      case MsgType.FriendListReturn: {
        const list = raw as FriendListReturnMessage;
        const friends = toContacts(list.friends || []);
        set((s) => ({
          friends,
          contacts: [
            ...s.contacts.filter((contact) => !friends.some((friend) => friend.id === contact.id)),
            ...friends,
          ],
        }));
        break;
      }

      case MsgType.FriendOnline: {
        const status = raw as FriendOnlineMessage;
        const nextStatus = status.status === 'ok' ? 'online' as const : 'offline' as const;
        set((s) => ({
          contacts: s.contacts.map((c) => (c.id === status.friendId ? { ...c, status: nextStatus } : c)),
          friends: s.friends.map((c) => (c.id === status.friendId ? { ...c, status: nextStatus } : c)),
        }));
        break;
      }

      default:
        break;
    }
  },

  logout: () => {
    set({
      currentUser: null,
      contacts: [],
      friends: [],
      friendRequests: [],
      messagesByContact: {},
      activeContactId: null,
      groups: [],
      messagesByGroup: {},
      activeGroupId: null,
      searchResults: [],
      isSearching: false,
      searchQuery: '',
      summaryText: '',
      isSummarizing: false,
      auth: { view: 'login', loading: false, error: null },
    });
  },
}));
