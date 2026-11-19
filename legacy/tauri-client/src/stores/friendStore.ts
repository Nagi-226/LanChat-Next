import { create } from 'zustand';
import { t } from '../lib/i18n';
import { useConnectionStore } from './connectionStore';
import { useAuthStore } from './authStore';
import { useMessageStore } from './messageStore';
import type { Contact } from '../components/ContactList';
import type {
  ProtocolMessage,
  FriendRequestMessage,
  FriendRequestAckMessage,
  FriendAcceptReturnMessage,
  FriendRemoveReturnMessage,
  FriendListReturnMessage,
  FriendOnlineMessage,
  UserInfo,
} from '../../../../protocol/message_types';
import { MsgType } from '../../../../protocol/message_types';
import type { FriendRequestInfo } from './types';

function toContacts(friends: UserInfo[]): Contact[] {
  return friends.map((friend) => ({
    id: friend.id,
    nickname: friend.nickname,
    headId: friend.headId,
    status: 'offline' as const,
    unread: 0,
  }));
}

interface FriendStore {
  friends: Contact[];
  friendRequests: FriendRequestInfo[];
  sendFriendRequest: (toId: number, msg?: string) => Promise<void>;
  respondToRequest: (fromId: number, accept: boolean) => Promise<void>;
  removeFriend: (friendId: number) => Promise<void>;
  handleIncomingFriendEvent: (raw: ProtocolMessage) => void;
  resetOnLogout: () => void;
}

export const useFriendStore = create<FriendStore>()((set, get) => ({
  friends: [],
  friendRequests: [],

  sendFriendRequest: async (toId, msg = '') => {
    const user = useAuthStore.getState().currentUser;
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
    const user = useAuthStore.getState().currentUser;
    if (!user) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      type: accept ? MsgType.FriendAccept : MsgType.FriendRemove,
      fromId,
      toId: user.id,
    }));
  },

  removeFriend: async (friendId) => {
    const user = useAuthStore.getState().currentUser;
    if (!user) return;
    await useConnectionStore.getState().sendRawJson(JSON.stringify({
      type: MsgType.FriendRemove,
      fromId: user.id,
      toId: friendId,
    }));
  },

  handleIncomingFriendEvent: (raw: ProtocolMessage) => {
    switch (raw.type) {
      case MsgType.FriendRequest: {
        const request = raw as FriendRequestMessage;
        const fromId = request.fromId ?? 0;
        const fromUser = useMessageStore.getState().contacts.find((contact) => contact.id === fromId);
        set((s) => ({
          friendRequests: [
            ...s.friendRequests.filter((request) => request.id !== fromId),
            {
              id: fromId,
              nickname: fromUser?.nickname ?? t('common.unknownUser', { id: fromId }),
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
          useAuthStore.getState().setAuthError(ack.msg ?? t('store.friendRequestFailed'));
        }
        break;
      }

      case MsgType.FriendAcceptReturn: {
        const accepted = raw as FriendAcceptReturnMessage;
        if (accepted.status === 'ok') {
          const msgContacts = useMessageStore.getState().contacts;
          const existing = msgContacts.find((contact) => contact.id === accepted.friendId);
          const contact: Contact = existing ?? {
            id: accepted.friendId,
            nickname: accepted.nickname ?? t('common.unknownUser', { id: accepted.friendId }),
            headId: accepted.headId,
            status: 'offline',
            unread: 0,
          };
          if (!existing) {
            useMessageStore.getState().addContact(contact);
          }
          set((s) => ({
            friends: [...s.friends.filter((friend) => friend.id !== contact.id), contact],
            friendRequests: s.friendRequests.filter((request) => request.id !== contact.id),
          }));
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
        useMessageStore.getState().mergeFriendsIntoContacts(friends);
        set({ friends });
        break;
      }

      case MsgType.FriendOnline: {
        const fo = raw as FriendOnlineMessage;
        const nextStatus = fo.status === 'ok' ? 'online' as const : 'offline' as const;
        useMessageStore.getState().updateContactStatus(fo.friendId, nextStatus);
        set((s) => ({
          friends: s.friends.map((c) => (c.id === fo.friendId ? { ...c, status: nextStatus } : c)),
        }));
        break;
      }

      default:
        break;
    }
  },

  resetOnLogout: () => {
    set({ friends: [], friendRequests: [] });
  },
}));
