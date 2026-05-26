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

  handleIncomingMessage: (raw) => {
    if (!isValidMsgType(raw.type)) return;

    switch (raw.type) {
      case MsgType.LoginSuccessReturn: {
        const login = raw as LoginSuccessReturnMessage;
        const contacts = toContacts(login.friends || []);
        const firstContactId = selectFirstContact(contacts);
        set({
          currentUser: {
            id: login.id ?? 0,
            nickname: login.nickname ?? '',
            headId: login.headId ?? 0,
          },
          contacts,
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

      default:
        break;
    }
  },

  logout: () => {
    set({
      currentUser: null,
      contacts: [],
      messagesByContact: {},
      activeContactId: null,
      groups: [],
      messagesByGroup: {},
      activeGroupId: null,
      auth: { view: 'login', loading: false, error: null },
    });
  },
}));
