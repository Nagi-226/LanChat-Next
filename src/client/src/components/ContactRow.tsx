import { memo } from 'react';
import { motion } from 'framer-motion';
import Counter from '../lib/Counter';
import { getInitials } from '../lib/utils';
import { useTranslation } from '../lib/i18n';
import { useFriendStore } from '../stores/chatStore';
import type { Contact } from './ContactList';

const statusColor: Record<Contact['status'], string> = {
  online: 'bg-green-500',
  offline: 'bg-gray-400',
  busy: 'bg-amber-500',
};

const statusRank: Record<Contact['status'], number> = { online: 0, busy: 1, offline: 2 };

interface ContactRowProps {
  contact: Contact;
  active: boolean;
  onSelect: (id: number) => void;
}

export const ContactRow = memo(function ContactRow({ contact, active, onSelect }: ContactRowProps) {
  const { t } = useTranslation();
  const friends = useFriendStore((s) => s.friends);
  const sendFriendRequest = useFriendStore((s) => s.sendFriendRequest);
  const removeFriend = useFriendStore((s) => s.removeFriend);
  const isFriend = friends.some((friend) => friend.id === contact.id);

  const statusLabelMap: Record<Contact['status'], string> = {
    online: t('contactList.online'),
    offline: t('contactList.offline'),
    busy: t('contactList.busy'),
  };

  return (
    <motion.div
      role="button"
      tabIndex={0}
      layout
      onClick={() => onSelect(contact.id)}
      onKeyDown={(event) => {
        if (event.key === 'Enter' || event.key === ' ') onSelect(contact.id);
      }}
      whileHover={{ x: 2 }}
      whileTap={{ scale: 0.99 }}
      className={`relative flex w-full items-center gap-3 px-4 py-2.5 text-left transition-colors ${
        active ? 'bg-dark-highlight/10 dark:bg-dark-highlight/15' : 'hover:bg-dark-hover'
      }`}
      aria-label={t('contactList.openChat', { name: contact.nickname })}
    >
      {active && <motion.span layoutId="active-conversation" className="absolute inset-y-1 left-0 w-1 rounded-r-full bg-dark-highlight" />}
      <div className="relative flex-shrink-0">
        <div className="flex h-10 w-10 items-center justify-center rounded-full bg-dark-accent text-sm text-dark-text">
          {getInitials(contact.nickname)}
        </div>
        <span
          className={`absolute bottom-0 right-0 h-3 w-3 rounded-full border-2 border-light-sidebar dark:border-dark-sidebar ${statusColor[contact.status]} ${contact.status === 'online' ? 'animate-pulse' : ''}`}
          title={statusLabelMap[contact.status]}
        />
      </div>

      <div className="min-w-0 flex-1">
        <div className="flex items-center justify-between gap-2">
          <div className="truncate text-sm font-medium text-light-text dark:text-dark-text">
            {contact.nickname}
          </div>
          <span className="shrink-0 text-[10px] text-light-muted dark:text-dark-muted">
            #{contact.id}
          </span>
        </div>
        {contact.typing ? (
          <div className="truncate text-[11px] font-medium text-dark-highlight">{t('contactList.typing')}</div>
        ) : contact.lastMessage ? (
          <div className="truncate text-[11px] text-light-muted dark:text-dark-muted">
            {contact.lastMessage}
          </div>
        ) : (
          <div className="h-4" aria-hidden="true" />
        )}
      </div>

      {contact.unread > 0 && (
        <span className="flex h-5 min-w-[20px] flex-shrink-0 items-center justify-center rounded-full bg-dark-highlight px-1.5 text-[10px] font-bold text-white">
          <Counter value={contact.unread > 99 ? 99 : contact.unread} className="leading-none" />{contact.unread > 99 ? '+' : ''}
        </span>
      )}
      {isFriend ? (
        <button
          type="button"
          onClick={(event) => {
            event.stopPropagation();
            void removeFriend(contact.id);
          }}
          className="rounded bg-red-500/10 px-2 py-1 text-[10px] font-semibold text-red-500 opacity-70 hover:bg-red-500/20 hover:opacity-100 dark:text-red-300"
        >
          {t('contactList.remove')}
        </button>
      ) : (
        <button
          type="button"
          onClick={(event) => {
            event.stopPropagation();
            void sendFriendRequest(contact.id, t('contactList.defaultGreeting'));
          }}
          className="rounded bg-dark-highlight/10 px-2 py-1 text-[10px] font-semibold text-dark-highlight hover:bg-dark-highlight/20"
        >
          {t('contactList.add')}
        </button>
      )}
    </motion.div>
  );
});

export default ContactRow;
