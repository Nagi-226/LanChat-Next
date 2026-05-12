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
  // Auth
  auth: AuthState;
  setAuthView: (view: 'login' | 'register') => void;
  setAuthLoading: (loading: boolean) => void;
  setAuthError: (error: string | null) => void;

  // User
  currentUser: CurrentUser | null;
  setCurrentUser: (user: CurrentUser | null) => void;

  // Contacts
  contacts: Contact[];
  setContacts: (contacts: Contact[]) => void;

  // Messages: key = contactId
  messagesByContact: Record<number, ChatMessage[]>;
  activeContactId: number | null;
  setActiveContact: (id: number | null) => void;

  // Groups
  groups: GroupInfo[];

  // Actions
  login: (id: number, password: string) => Promise<void>;
  register: (nickname: string, password: string) => Promise<void>;
  sendPrivateMessage: (toId: number, content: string) => Promise<void>;
  handleIncomingMessage: (msg: ProtocolMessageAsJson) => void;
  logout: () => void;
}

type ProtocolMessageAsJson = {
  type: number;
  fromId?: number;
  toId?: number;
  msg?: string;
  nickname?: string;
  headId?: number;
  id?: number;
  friends?: UserInfo[];
  groups?: GroupInfo[];
  users?: UserInfo[];
  status?: string;
  timestamp?: number;
  msg_id?: string;
  content_type?: string;
};

function msgId(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
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
  setActiveContact: (id) => set({ activeContactId: id }),

  groups: [],

  login: async (id, password) => {
    const { setAuthLoading, setAuthError, setCurrentUser, setContacts } = get();
    setAuthLoading(true);

    const loginMsg: LoginMessage = {
      type: MsgType.Login,
      id,
      password,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(loginMsg));
      // Note: in production, login response comes async via Tauri event.
      // For now the send succeeds; the response is handled by handleIncomingMessage.
      // Placeholder optimistic login for testing:
      setAuthLoading(false);
    } catch (e) {
      setAuthError(String(e));
    }
  },

  register: async (nickname, password) => {
    const { setAuthLoading, setAuthError } = get();
    setAuthLoading(true);

    const regMsg: RegisterUserMessage = {
      type: MsgType.RegisterUser,
      password,
      nickname,
    };

    try {
      await useConnectionStore.getState().sendRawJson(JSON.stringify(regMsg));
      setAuthLoading(false);
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

    // Optimistic local push
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
    }));

    await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
  },

  handleIncomingMessage: (raw) => {
    if (!isValidMsgType(raw.type)) return;

    const user = get().currentUser;

    switch (raw.type) {
      case MsgType.LoginSuccessReturn: {
        const friends: Contact[] = (raw.friends || []).map((f) => ({
          id: f.id,
          nickname: f.nickname,
          headId: f.headId,
          status: 'offline' as const,
          unread: 0,
        }));
        set({
          currentUser: {
            id: raw.id ?? 0,
            nickname: raw.nickname ?? '',
            headId: raw.headId ?? 0,
          },
          contacts: friends,
          groups: raw.groups || [],
          auth: { view: 'login', loading: false, error: null },
        });
        break;
      }

      case MsgType.ReceiveMsg: {
        const fromId = raw.fromId ?? 0;
        const chatMsg: ChatMessage = {
          id: raw.msg_id || msgId(),
          fromId,
          nickname: raw.nickname ?? `User ${fromId}`,
          content: raw.msg ?? '',
          contentType: 'text',
          timestamp: raw.timestamp ?? Date.now(),
        };
        set((s) => ({
          messagesByContact: {
            ...s.messagesByContact,
            [fromId]: [...(s.messagesByContact[fromId] || []), chatMsg],
          },
        }));
        break;
      }

      case MsgType.UserOnline: {
        const uid = raw.id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) =>
            c.id === uid ? { ...c, status: 'online' as const } : c,
          ),
        }));
        break;
      }

      case MsgType.UserOffline: {
        const uid = raw.id ?? 0;
        set((s) => ({
          contacts: s.contacts.map((c) =>
            c.id === uid ? { ...c, status: 'offline' as const } : c,
          ),
        }));
        break;
      }
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
