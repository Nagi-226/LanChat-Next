import { create } from 'zustand';
import type { ChatMessage } from '../components/ChatArea';
import type { Contact } from '../components/ContactList';
import type {
  LoginMessage,
  LoginSuccessReturnMessage,
  RegisterUserMessage,
  SendMsgMessage,
  ReceiveMsgMessage,
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

  login: (id: number, password: string) => Promise<void>;
  register: (nickname: string, password: string) => Promise<void>;
  sendPrivateMessage: (toId: number, content: string) => Promise<void>;
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
  raw: Partial<ReceiveMsgMessage> & { fromId?: number; msg?: string; nickname?: string; timestamp?: number; msg_id?: string },
): ChatMessage {
  return {
    id: raw.msg_id || msgId(),
    fromId: raw.fromId ?? 0,
    nickname: raw.nickname ?? 'Unknown',
    content: raw.msg ?? '',
    contentType: 'text',
    timestamp: raw.timestamp ?? Date.now(),
  };
}

function markActiveContactSelected(state: ChatState, contactId: number): ChatState {
  return {
    ...state,
    activeContactId: contactId,
    contacts: state.contacts.map((c) => (c.id === contactId ? { ...c, unread: 0 } : c)),
  };
}

function selectFirstContact(contacts: Contact[]): number | null {
  return contacts[0]?.id ?? null;
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

  login: async (id, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    setAuthError(null);

    const loginMsg: LoginMessage = {
      type: MsgType.Login,
      id,
      password,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(loginMsg));
    } catch (e) {
      setAuthError(String(e));
    }
  },

  register: async (nickname, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);
    setAuthError(null);

    const regMsg: RegisterUserMessage = {
      type: MsgType.RegisterUser,
      password,
      nickname,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(regMsg));
    } catch (e) {
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

    const chatMsg: ChatMessage = {
      id: msgId(),
      fromId: user.id,
      nickname: user.nickname,
      content,
      contentType: 'text',
      timestamp: Date.now(),
    };

    set((s) => ({
      messagesByContact: {
        ...s.messagesByContact,
        [toId]: [...(s.messagesByContact[toId] || []), chatMsg],
      },
      contacts: s.contacts.map((c) =>
        c.id === toId
          ? { ...c, lastMessage: content }
          : c,
      ),
    }));

    await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
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
          activeContactId: firstContactId,
          auth: { view: 'login', loading: false, error: null },
        });
        break;
      }

      case MsgType.LoginFailedReturn: {
        set((s) => ({
          auth: { ...s.auth, loading: false, error: 'Login failed' },
        }));
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

      case MsgType.UserOnline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) =>
            c.id === uid ? { ...c, status: 'online' as const } : c,
          ),
        }));
        break;
      }

      case MsgType.UserOffline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) =>
            c.id === uid ? { ...c, status: 'offline' as const } : c,
          ),
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
      auth: { view: 'login', loading: false, error: null },
    });
  },
}));
