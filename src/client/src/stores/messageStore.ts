import { create } from 'zustand';
import { t } from '../lib/i18n';
import type { ChatMessage } from '../components/ChatArea';
import type { Contact } from '../components/ContactList';
import type {
  SendMsgMessage,
  ReceiveMsgMessage,
  SendGroupMsgMessage,
  ReceiveGroupMsgMessage,
  UserJoinGroupMessage,
  UserLeaveGroupMessage,
  GroupInfo,
  ProtocolMessage,
  RequestHistoryMessage,
  HistoryResponseMessage,
  SystemBroadcastMessage,
  OfflineMessagesMessage,
  MessageEditMessage,
  MessageEditReturnMessage,
  MessageDeleteMessage,
  MessageDeleteReturnMessage,
  MessageReactionMessage,
  MessageReactionReturnMessage,
  ReadReceiptMessage,
  SendFileMessage,
  ReceiveFileMessage,
  FileTransferDoneMessage,
  ProtocolHelloMessage,
  CreateGroupMessage,
  CreateGroupReturnMessage,
  JoinGroupMessage,
  JoinGroupReturnMessage,
  LeaveGroupMessage,
} from '../../../../protocol/message_types';
import { MsgType, isValidMsgType } from '../../../../protocol/message_types';
import { useConnectionStore } from './connectionStore';
import { useAuthStore } from './authStore';
import { useFriendStore } from './friendStore';
import { useSearchStore } from './searchStore';
import {
  createQueuedMessage,
  dequeueOfflineMessage,
  enqueueOfflineMessage,
  loadOfflineQueue,
  saveOfflineQueue,
  type QueuedMessage,
} from '../lib/offlineQueue';

// ── helpers ──────────────────────────────────────────────────────────

function msgId(): string {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

const typingTimers = new Map<number, number>();

function toChatMessage(
  raw: Partial<ReceiveMsgMessage | ReceiveGroupMsgMessage> & {
    fromId?: number;
    msg?: string;
    nickname?: string;
    timestamp?: number;
    msg_id?: string | number;
    edited?: boolean;
    deleted?: boolean;
    reactions?: string;
    read?: boolean;
  },
): ChatMessage {
  return {
    id: raw.msg_id !== undefined ? String(raw.msg_id) : msgId(),
    fromId: raw.fromId ?? 0,
    nickname: raw.nickname ?? t('common.unknown'),
    content: raw.msg ?? '',
    contentType: raw.content_type ?? 'text',
    timestamp: raw.timestamp ?? Date.now(),
    status: 'sent',
    edited: Boolean(raw.edited),
    deleted: Boolean(raw.deleted),
    reactions: raw.reactions ? raw.reactions.split(',').filter(Boolean) : undefined,
    read: Boolean(raw.read),
  };
}

function systemMessage(content: string): ChatMessage {
  return {
    id: msgId(),
    fromId: 0,
    nickname: t('common.system'),
    content,
    contentType: 'system',
    timestamp: Date.now(),
    status: 'sent',
  };
}

function selectFirstContact(contacts: Contact[]): number | null {
  return contacts[0]?.id ?? null;
}

function updateMessageStatus(messages: ChatMessage[] | undefined, id: string, status: NonNullable<ChatMessage['status']>): ChatMessage[] {
  return (messages || []).map((message) => (message.id === id ? { ...message, status } : message));
}

function updateMessageById(messages: ChatMessage[] | undefined, id: string, updater: (message: ChatMessage) => ChatMessage): ChatMessage[] {
  return (messages || []).map((message) => (message.id === id ? updater(message) : message));
}

function numericMessageId(messageId: string): number {
  const parsed = Number(messageId);
  return Number.isFinite(parsed) ? parsed : 0;
}

function directPeerId(payload: { fromId?: number; toId?: number }, currentUserId: number): number {
  return payload.fromId === currentUserId ? (payload.toId ?? 0) : (payload.fromId ?? 0);
}

function queuedStatus(): NonNullable<ChatMessage['status']> {
  return useConnectionStore.getState().status === 'connected' ? 'sending' : 'queued';
}

function upsertGroupUser(groups: GroupInfo[], groupId: number, user: { id: number; nickname: string; headId?: number }): GroupInfo[] {
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

// ── store ─────────────────────────────────────────────────────────────

interface MessageStore {
  contacts: Contact[];
  messagesByContact: Record<number, ChatMessage[]>;
  activeContactId: number | null;

  groups: GroupInfo[];
  messagesByGroup: Record<number, ChatMessage[]>;
  activeGroupId: number | null;

  offlineQueue: QueuedMessage[];

  setContacts: (contacts: Contact[]) => void;
  setActiveContact: (id: number | null) => void;
  selectGroup: (id: number | null) => void;

  // methods called by other stores
  addContact: (contact: Contact) => void;
  mergeFriendsIntoContacts: (friends: Contact[]) => void;
  updateContactStatus: (userId: number, status: Contact['status']) => void;

  sendPrivateMessage: (toId: number, content: string) => Promise<void>;
  sendGroupMessage: (groupId: number, content: string) => Promise<void>;
  editMessage: (target: { type: 'direct' | 'group'; id: number }, messageId: string, content: string) => Promise<void>;
  deleteMessage: (target: { type: 'direct' | 'group'; id: number }, messageId: string) => Promise<void>;
  reactToMessage: (target: { type: 'direct' | 'group'; id: number }, messageId: string, reaction: string) => Promise<void>;
  markMessageRead: (target: { type: 'direct' | 'group'; id: number }, messageId: string) => Promise<void>;
  sendFileMetadata: (target: { type: 'direct' | 'group'; id: number }, file: { name: string; size: number; hash: string; note?: string }) => Promise<void>;
  negotiateProtocol: () => Promise<void>;
  createGroup: (name: string) => Promise<void>;
  joinGroup: (groupId: number) => Promise<void>;
  leaveGroup: (groupId: number) => Promise<void>;
  flushOfflineQueue: () => Promise<void>;
  sendTypingIndicator: (target: { type: 'direct' | 'group'; id: number }) => Promise<void>;
  requestHistory: (target: { type: 'direct' | 'group'; id: number; limit?: number }) => Promise<void>;
  handleIncomingMessage: (msg: ProtocolMessage) => void;
  resetOnLogout: () => void;
}

export const useMessageStore = create<MessageStore>()((set, get) => ({
  contacts: [],
  messagesByContact: {},
  activeContactId: null,

  groups: [],
  messagesByGroup: {},
  activeGroupId: null,

  offlineQueue: loadOfflineQueue(),

  setContacts: (contacts) => set({ contacts }),

  setActiveContact: (id) =>
    set((state) => {
      if (id === null) return { activeContactId: null };
      return {
        activeContactId: id,
        activeGroupId: null,
        contacts: state.contacts.map((c) => (c.id === id ? { ...c, unread: 0 } : c)),
      };
    }),

  selectGroup: (id) => set({ activeGroupId: id, activeContactId: null }),

  addContact: (contact) =>
    set((s) => ({ contacts: [...s.contacts.filter((c) => c.id !== contact.id), contact] })),

  mergeFriendsIntoContacts: (friends) =>
    set((s) => ({
      contacts: [
        ...s.contacts.filter((c) => !friends.some((f) => f.id === c.id)),
        ...friends,
      ],
    })),

  updateContactStatus: (userId, status) =>
    set((s) => ({
      contacts: s.contacts.map((c) => (c.id === userId ? { ...c, status } : c)),
    })),

  sendPrivateMessage: async (toId, content) => {
    const user = useAuthStore.getState().currentUser;
    if (!user) return;

    const msg: SendMsgMessage = { type: MsgType.SendMsg, fromId: user.id, toId, msg: content };
    const localId = msgId();
    const chatMsg: ChatMessage = {
      id: localId, fromId: user.id, nickname: user.nickname,
      content, contentType: 'text', timestamp: Date.now(), status: queuedStatus(),
    };

    set((s) => ({
      messagesByContact: { ...s.messagesByContact, [toId]: [...(s.messagesByContact[toId] || []), chatMsg] },
      contacts: s.contacts.map((c) => (c.id === toId ? { ...c, lastMessage: content } : c)),
    }));

    const json = JSON.stringify(msg);
    if (useConnectionStore.getState().status !== 'connected') {
      const queue = enqueueOfflineMessage(createQueuedMessage({ kind: 'direct', targetId: toId, localMessageId: localId, json }));
      set({ offlineQueue: queue });
      return;
    }

    try {
      await useConnectionStore.getState().sendRawJson(json);
      set((s) => ({
        messagesByContact: { ...s.messagesByContact, [toId]: updateMessageStatus(s.messagesByContact[toId], localId, 'sent') },
      }));
    } catch {
      set((s) => ({
        messagesByContact: { ...s.messagesByContact, [toId]: updateMessageStatus(s.messagesByContact[toId], localId, 'failed') },
      }));
    }
  },

  sendGroupMessage: async (groupId, content) => {
    const user = useAuthStore.getState().currentUser;
    if (!user) return;

    const msg: SendGroupMsgMessage = { type: MsgType.SendGroupMsg, fromId: user.id, groupId, msg: content };
    const localId = msgId();
    const chatMsg: ChatMessage = {
      id: localId, fromId: user.id, nickname: user.nickname,
      content, contentType: 'text', timestamp: Date.now(), status: queuedStatus(),
    };

    set((s) => ({
      messagesByGroup: { ...s.messagesByGroup, [groupId]: [...(s.messagesByGroup[groupId] || []), chatMsg] },
    }));

    const json = JSON.stringify(msg);
    if (useConnectionStore.getState().status !== 'connected') {
      const queue = enqueueOfflineMessage(createQueuedMessage({ kind: 'group', targetId: groupId, localMessageId: localId, json }));
      set({ offlineQueue: queue });
      return;
    }

    try {
      await useConnectionStore.getState().sendRawJson(json);
      set((s) => ({
        messagesByGroup: { ...s.messagesByGroup, [groupId]: updateMessageStatus(s.messagesByGroup[groupId], localId, 'sent') },
      }));
    } catch {
      set((s) => ({
        messagesByGroup: { ...s.messagesByGroup, [groupId]: updateMessageStatus(s.messagesByGroup[groupId], localId, 'failed') },
      }));
    }
  },

  editMessage: async (target, messageId, content) => {
    const user = useAuthStore.getState().currentUser;
    const msgIdNum = numericMessageId(messageId);
    if (!user || msgIdNum <= 0) return;
    const msg: MessageEditMessage = {
      type: MsgType.MessageEdit, fromId: user.id, msg_id: String(msgIdNum), msg: content,
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ ...msg, msg_id: msgIdNum }));
  },

  deleteMessage: async (target, messageId) => {
    const user = useAuthStore.getState().currentUser;
    const msgIdNum = numericMessageId(messageId);
    if (!user || msgIdNum <= 0) return;
    const msg: MessageDeleteMessage = {
      type: MsgType.MessageDelete, fromId: user.id, msg_id: String(msgIdNum),
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ ...msg, msg_id: msgIdNum }));
  },

  reactToMessage: async (target, messageId, reaction) => {
    const user = useAuthStore.getState().currentUser;
    const msgIdNum = numericMessageId(messageId);
    if (!user || msgIdNum <= 0) return;
    const msg: MessageReactionMessage = {
      type: MsgType.MessageReaction, fromId: user.id, msg_id: String(msgIdNum), reaction,
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ ...msg, msg_id: msgIdNum }));
  },

  markMessageRead: async (target, messageId) => {
    const user = useAuthStore.getState().currentUser;
    const msgIdNum = numericMessageId(messageId);
    if (!user || msgIdNum <= 0 || useConnectionStore.getState().status !== 'connected') return;
    const msg: ReadReceiptMessage = {
      type: MsgType.ReadReceipt, fromId: user.id, msg_id: String(msgIdNum),
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ ...msg, msg_id: msgIdNum })).catch(() => undefined);
  },

  sendFileMetadata: async (target, file) => {
    const user = useAuthStore.getState().currentUser;
    if (!user) return;
    const msg: SendFileMessage = {
      type: MsgType.SendFile, fromId: user.id, file_name: file.name, file_size: file.size, file_hash: file.hash, msg: file.note,
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
  },

  negotiateProtocol: async () => {
    if (useConnectionStore.getState().status !== 'connected') return;
    const hello: ProtocolHelloMessage = {
      type: MsgType.ProtocolHello, version: 2, min_version: 1,
      features: ['message_edit', 'message_delete', 'message_reaction', 'read_receipt', 'file_metadata'],
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(hello)).catch(() => undefined);
  },

  createGroup: async (name) => {
    const user = useAuthStore.getState().currentUser;
    const trimmed = name.trim();
    if (!user || !trimmed) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      type: MsgType.CreateGroup, hostId: user.id, name: trimmed, hostNickname: user.nickname, hostHeadId: user.headId,
    } as CreateGroupMessage));
  },

  joinGroup: async (groupId) => {
    const user = useAuthStore.getState().currentUser;
    if (!user || groupId <= 0) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ type: MsgType.JoinGroup, id: user.id, groupId } as JoinGroupMessage));
  },

  leaveGroup: async (groupId) => {
    const user = useAuthStore.getState().currentUser;
    if (!user || groupId <= 0) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({ type: MsgType.LeaveGroup, id: user.id, groupId } as LeaveGroupMessage));
    set((s) => ({
      groups: s.groups.filter((group) => group.groupId !== groupId),
      messagesByGroup: Object.fromEntries(Object.entries(s.messagesByGroup).filter(([id]) => Number(id) !== groupId)),
      activeGroupId: s.activeGroupId === groupId ? null : s.activeGroupId,
    }));
  },

  flushOfflineQueue: async () => {
    if (useConnectionStore.getState().status !== 'connected') return;
    const queue = loadOfflineQueue();
    set({ offlineQueue: queue });
    for (const item of queue) {
      try {
        set((s) => item.kind === 'direct'
          ? { messagesByContact: { ...s.messagesByContact, [item.targetId]: updateMessageStatus(s.messagesByContact[item.targetId], item.localMessageId, 'sending') } }
          : { messagesByGroup: { ...s.messagesByGroup, [item.targetId]: updateMessageStatus(s.messagesByGroup[item.targetId], item.localMessageId, 'sending') } });
        await useConnectionStore.getState().sendRawJson(item.json);
        const nextQueue = dequeueOfflineMessage(window.localStorage, item.localMessageId);
        set((s) => item.kind === 'direct'
          ? { offlineQueue: nextQueue, messagesByContact: { ...s.messagesByContact, [item.targetId]: updateMessageStatus(s.messagesByContact[item.targetId], item.localMessageId, 'sent') } }
          : { offlineQueue: nextQueue, messagesByGroup: { ...s.messagesByGroup, [item.targetId]: updateMessageStatus(s.messagesByGroup[item.targetId], item.localMessageId, 'sent') } });
      } catch {
        const nextQueue = loadOfflineQueue().map((queued) =>
          queued.localMessageId === item.localMessageId ? { ...queued, attempts: queued.attempts + 1 } : queued);
        saveOfflineQueue(window.localStorage, nextQueue);
        set({ offlineQueue: nextQueue });
        break;
      }
    }
  },

  sendTypingIndicator: async (target) => {
    const user = useAuthStore.getState().currentUser;
    if (!user || useConnectionStore.getState().status !== 'connected') return;
    const payload = {
      type: MsgType.SystemBroadcast, msg: 'typing', event: 'typing',
      fromId: user.id, nickname: user.nickname, isTyping: true,
      ...(target.type === 'direct' ? { toId: target.id } : { groupId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(payload)).catch(() => undefined);
  },

  requestHistory: async (target) => {
    const user = useAuthStore.getState().currentUser;
    if (!user) return;
    const msg: RequestHistoryMessage = {
      type: MsgType.RequestHistory, id: user.id, limit: target.limit ?? 100,
      ...(target.type === 'group' ? { groupId: target.id } : { toId: target.id }),
    };
    await useConnectionStore.getState().sendRawJson(JSON.stringify(msg));
  },

  // ── handleIncomingMessage: the central message dispatcher ──────────

  handleIncomingMessage: (raw) => {
    if (!isValidMsgType(raw.type)) return;

    switch (raw.type) {
      // ── Auth (dispatched to authStore) ──
      case MsgType.LoginSuccessReturn: {
        const login = raw as import('../../../../protocol/message_types').LoginSuccessReturnMessage;
        const friends = (login.friends || []).map((f) => ({ id: f.id, nickname: f.nickname, headId: f.headId, status: 'offline' as const, unread: 0 }));
        const contacts: Contact[] = (login.users || login.friends || []).map((f) => ({ id: f.id, nickname: f.nickname, headId: f.headId, status: 'offline' as const, unread: 0 }));
        const firstContactId = selectFirstContact(contacts);
        useAuthStore.setState({
          currentUser: { id: login.id ?? 0, nickname: login.nickname ?? '', headId: login.headId ?? 0 },
        });
        useAuthStore.setState((s) => ({ auth: { ...s.auth, view: 'login', loading: false, error: null } }));
        useFriendStore.setState({ friends });
        set({ contacts, messagesByGroup: {}, activeContactId: firstContactId, activeGroupId: null, groups: login.groups || [] });
        break;
      }

      case MsgType.LoginFailedReturn: {
        const errorMsg = 'msg' in raw ? (raw as unknown as Record<string, unknown>).msg as string : t('store.loginFailed');
        useAuthStore.getState().setAuthError(errorMsg || t('store.loginFailed'));
        break;
      }

      case MsgType.RegisterUserReturn: {
        useAuthStore.setState((s) => ({ auth: { ...s.auth, loading: false, error: null, view: 'login' } }));
        break;
      }

      // ── Private messages ──
      case MsgType.ReceiveMsg: {
        const incoming = raw as ReceiveMsgMessage;
        const fromId = incoming.fromId ?? 0;
        const chatMsg = toChatMessage(incoming);
        set((s) => ({
          messagesByContact: { ...s.messagesByContact, [fromId]: [...(s.messagesByContact[fromId] || []), chatMsg] },
          contacts: s.contacts.map((c) =>
            c.id === fromId ? { ...c, unread: s.activeContactId === fromId ? c.unread : c.unread + 1, lastMessage: incoming.msg ?? c.lastMessage } : c,
          ),
        }));
        break;
      }

      case MsgType.ReceiveGroupMsg: {
        const incoming = raw as ReceiveGroupMsgMessage;
        const groupId = incoming.groupId ?? 0;
        set((s) => ({
          messagesByGroup: { ...s.messagesByGroup, [groupId]: [...(s.messagesByGroup[groupId] || []), toChatMessage(incoming)] },
        }));
        break;
      }

      // ── History ──
      case MsgType.HistoryResponse: {
        const history = raw as HistoryResponseMessage;
        const messages = (history.messages || []).map((item) => toChatMessage(item as Partial<ReceiveMsgMessage | ReceiveGroupMsgMessage>));
        const first = history.messages?.[0] as { fromId?: number; toId?: number; groupId?: number } | undefined;
        if (!first || messages.length === 0) break;
        const mergeById = (existing: ChatMessage[]) => {
          const byId = new Map(existing.map((m) => [m.id, m]));
          for (const m of messages) byId.set(m.id, m);
          return [...byId.values()].sort((a, b) => a.timestamp - b.timestamp);
        };
        if (first.groupId) {
          set((s) => ({
            messagesByGroup: { ...s.messagesByGroup, [first.groupId as number]: mergeById(s.messagesByGroup[first.groupId as number] || []) },
          }));
        } else {
          const currentUserId = useAuthStore.getState().currentUser?.id ?? 0;
          const peerId = first.fromId === currentUserId ? first.toId : first.fromId;
          if (peerId) {
            set((s) => ({
              messagesByContact: { ...s.messagesByContact, [peerId]: mergeById(s.messagesByContact[peerId] || []) },
            }));
          }
        }
        break;
      }

      case MsgType.OfflineMessages: {
        const offline = raw as OfflineMessagesMessage;
        for (const item of offline.messages || []) {
          get().handleIncomingMessage(item as unknown as ProtocolMessage);
        }
        break;
      }

      // ── Message mutations ──
      case MsgType.MessageEditReturn: {
        const edited = raw as MessageEditReturnMessage;
        if (edited.status !== 'ok') break;
        const id = String(edited.msg_id);
        const currentUserId = useAuthStore.getState().currentUser?.id ?? 0;
        const applyEdit = (m: ChatMessage) => ({ ...m, content: edited.msg ?? m.content, edited: true });
        if (edited.groupId) {
          set((s) => ({ messagesByGroup: { ...s.messagesByGroup, [edited.groupId as number]: updateMessageById(s.messagesByGroup[edited.groupId as number], id, applyEdit) } }));
        } else {
          const peerId = directPeerId(edited, currentUserId);
          if (peerId) set((s) => ({ messagesByContact: { ...s.messagesByContact, [peerId]: updateMessageById(s.messagesByContact[peerId], id, applyEdit) } }));
        }
        break;
      }

      case MsgType.MessageDeleteReturn: {
        const deleted = raw as MessageDeleteReturnMessage;
        if (deleted.status !== 'ok') break;
        const id = String(deleted.msg_id);
        const currentUserId = useAuthStore.getState().currentUser?.id ?? 0;
        const mark = (m: ChatMessage) => ({ ...m, content: '', deleted: true });
        if (deleted.groupId) {
          set((s) => ({ messagesByGroup: { ...s.messagesByGroup, [deleted.groupId as number]: updateMessageById(s.messagesByGroup[deleted.groupId as number], id, mark) } }));
        } else {
          const peerId = directPeerId(deleted, currentUserId);
          if (peerId) set((s) => ({ messagesByContact: { ...s.messagesByContact, [peerId]: updateMessageById(s.messagesByContact[peerId], id, mark) } }));
        }
        break;
      }

      case MsgType.MessageReactionReturn: {
        const rxn = raw as MessageReactionReturnMessage;
        if (rxn.status !== 'ok') break;
        const id = String(rxn.msg_id);
        const entry = `${rxn.fromId ?? 0}:${rxn.reaction}`;
        const add = (m: ChatMessage) => ({ ...m, reactions: m.reactions?.includes(entry) ? m.reactions : [...(m.reactions || []), entry] });
        const currentUserId = useAuthStore.getState().currentUser?.id ?? 0;
        if (rxn.groupId) {
          set((s) => ({ messagesByGroup: { ...s.messagesByGroup, [rxn.groupId as number]: updateMessageById(s.messagesByGroup[rxn.groupId as number], id, add) } }));
        } else {
          const peerId = directPeerId(rxn, currentUserId);
          if (peerId) set((s) => ({ messagesByContact: { ...s.messagesByContact, [peerId]: updateMessageById(s.messagesByContact[peerId], id, add) } }));
        }
        break;
      }

      case MsgType.ReadReceipt: {
        const receipt = raw as ReadReceiptMessage;
        const id = String(receipt.msg_id);
        const currentUserId = useAuthStore.getState().currentUser?.id ?? 0;
        const mark = (m: ChatMessage) => ({ ...m, read: true });
        if (receipt.groupId) {
          set((s) => ({ messagesByGroup: { ...s.messagesByGroup, [receipt.groupId as number]: updateMessageById(s.messagesByGroup[receipt.groupId as number], id, mark) } }));
        } else {
          const peerId = directPeerId(receipt, currentUserId);
          if (peerId) set((s) => ({ messagesByContact: { ...s.messagesByContact, [peerId]: updateMessageById(s.messagesByContact[peerId], id, mark) } }));
        }
        break;
      }

      // ── File ──
      case MsgType.ReceiveFile: {
        const file = raw as ReceiveFileMessage;
        const fromId = file.fromId ?? 0;
        const label = `${file.file_name} (${Math.round((file.file_size || 0) / 1024)} KB)`;
        const chatMsg: ChatMessage = {
          id: file.msg_id ? String(file.msg_id) : msgId(), fromId, nickname: t('common.unknownUser', { id: fromId }),
          content: file.msg ? `${file.msg}: ${label}` : label,
          contentType: file.file_name.match(/\.(png|jpg|jpeg|gif|webp)$/i) ? 'image' : 'file',
          timestamp: file.timestamp ?? Date.now(), status: 'sent',
        };
        if (file.groupId) {
          set((s) => ({ messagesByGroup: { ...s.messagesByGroup, [file.groupId as number]: [...(s.messagesByGroup[file.groupId as number] || []), chatMsg] } }));
        } else {
          set((s) => ({ messagesByContact: { ...s.messagesByContact, [fromId]: [...(s.messagesByContact[fromId] || []), chatMsg] } }));
        }
        break;
      }

      case MsgType.FileTransferDone: {
        const done = raw as FileTransferDoneMessage;
        if (done.status !== 'ok') useAuthStore.getState().setAuthError(done.msg ?? 'file transfer failed');
        break;
      }

      // ── Groups ──
      case MsgType.CreateGroupReturn: {
        const created = raw as CreateGroupReturnMessage;
        if (created.status === 'ok' && created.groupId) {
          const user = useAuthStore.getState().currentUser;
          const group: GroupInfo = {
            groupId: created.groupId, name: created.name ?? t('contactList.groupFallback', { id: created.groupId }),
            hostId: user?.id, users: user ? [{ id: user.id, nickname: user.nickname, headId: user.headId }] : [],
          };
          set((s) => ({ groups: [...s.groups.filter((g) => g.groupId !== group.groupId), group], activeGroupId: group.groupId, activeContactId: null }));
        } else {
          useAuthStore.getState().setAuthError(created.msg ?? 'group create failed');
        }
        break;
      }

      case MsgType.JoinGroupReturn: {
        const joined = raw as JoinGroupReturnMessage;
        if (joined.status === 'ok') {
          set((s) => {
            const existing = s.groups.find((g) => g.groupId === joined.groupId);
            const group: GroupInfo = existing ?? { groupId: joined.groupId, name: t('contactList.groupFallback', { id: joined.groupId }) };
            return {
              groups: [...s.groups.filter((g) => g.groupId !== joined.groupId), { ...group, users: joined.users ?? group.users }],
              activeGroupId: joined.groupId, activeContactId: null,
            };
          });
        } else {
          useAuthStore.getState().setAuthError(joined.msg ?? 'join group failed');
        }
        break;
      }

      case MsgType.UserJoinGroup: {
        const incoming = raw as UserJoinGroupMessage;
        const nickname = incoming.nickname || t('common.unknownUser', { id: incoming.id });
        set((s) => ({
          groups: upsertGroupUser(s.groups, incoming.groupId, { id: incoming.id, nickname, headId: incoming.headId }),
          messagesByGroup: { ...s.messagesByGroup, [incoming.groupId]: [...(s.messagesByGroup[incoming.groupId] || []), systemMessage(t('common.userJoinedGroup', { nickname }))] },
        }));
        break;
      }

      case MsgType.UserLeaveGroup: {
        const incoming = raw as UserLeaveGroupMessage;
        const nickname = incoming.nickname || t('common.unknownUser', { id: incoming.id });
        set((s) => ({
          groups: removeGroupUser(s.groups, incoming.groupId, incoming.id),
          messagesByGroup: { ...s.messagesByGroup, [incoming.groupId]: [...(s.messagesByGroup[incoming.groupId] || []), systemMessage(t('common.userLeftGroup', { nickname }))] },
        }));
        break;
      }

      // ── Presence ──
      case MsgType.UserOnline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({ contacts: s.contacts.map((c) => (c.id === uid ? { ...c, status: 'online' as const } : c)) }));
        break;
      }

      case MsgType.UserOffline: {
        const uid = (raw as { id?: number }).id ?? 0;
        set((s) => ({ contacts: s.contacts.map((c) => (c.id === uid ? { ...c, status: 'offline' as const } : c)) }));
        break;
      }

      // ── System / Typing ──
      case MsgType.SystemBroadcast: {
        const event = raw as SystemBroadcastMessage & { event?: string; fromId?: number; nickname?: string; isTyping?: boolean };
        if (event.event === 'typing' && event.fromId && event.isTyping) {
          const fromId = event.fromId;
          set((s) => ({ contacts: s.contacts.map((c) => (c.id === fromId ? { ...c, typing: true } : c)) }));
          const existingTimer = typingTimers.get(fromId);
          if (existingTimer) window.clearTimeout(existingTimer);
          typingTimers.set(fromId, window.setTimeout(() => {
            typingTimers.delete(fromId);
            set((s) => ({ contacts: s.contacts.map((c) => (c.id === fromId ? { ...c, typing: false } : c)) }));
          }, 3500));
        }
        break;
      }

      // ── Protocol ──
      case MsgType.ProtocolHello:
        break;

      // ── Dispatch to friendStore ──
      case MsgType.FriendRequest:
      case MsgType.FriendRequestAck:
      case MsgType.FriendAcceptReturn:
      case MsgType.FriendRemoveReturn:
      case MsgType.FriendListReturn:
      case MsgType.FriendOnline:
        useFriendStore.getState().handleIncomingFriendEvent(raw);
        break;

      // ── Dispatch to searchStore ──
      case MsgType.AIResponse:
      case MsgType.AIStreamChunk:
        useSearchStore.getState().handleIncomingAIEvent(raw);
        break;

      default:
        break;
    }
  },

  resetOnLogout: () => {
    set({
      contacts: [], messagesByContact: {}, activeContactId: null,
      groups: [], messagesByGroup: {}, activeGroupId: null,
      offlineQueue: loadOfflineQueue(),
    });
  },
}));
