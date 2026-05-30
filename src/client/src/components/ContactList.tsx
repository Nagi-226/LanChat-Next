import { useEffect, useMemo, useRef, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import type { GroupInfo } from '../../../../protocol/message_types';
import BorderGlow from '../lib/BorderGlow';
import Counter from '../lib/Counter';
import FadeContent from '../lib/FadeContent';
import { useTranslation } from '../lib/i18n';
import { useFriendStore } from '../stores/chatStore';
import { ContactRow } from './ContactRow';
import { FriendRequestRow } from './FriendRequestRow';
import { GroupRow } from './GroupRow';
import { ContactSkeleton } from './ContactSkeleton';

export interface Contact {
  id: number;
  nickname: string;
  headId?: number;
  status: 'online' | 'offline' | 'busy';
  unread: number;
  lastMessage?: string;
  typing?: boolean;
}

interface ContactListProps {
  contacts: Contact[];
  groups?: GroupInfo[];
  selectedId: number | null;
  selectedGroupId?: number | null;
  loading?: boolean;
  onSelect: (id: number) => void;
  onSelectGroup?: (id: number) => void;
  onCreateGroup?: (name: string) => void;
  onJoinGroup?: (groupId: number) => void;
  onLeaveGroup?: (groupId: number) => void;
  onLogout: () => void;
}

const statusColor: Record<Contact['status'], string> = {
  online: 'bg-light-online',
  offline: 'bg-light-muted dark:bg-dark-muted',
  busy: 'bg-amber-400',
};

const statusRank: Record<Contact['status'], number> = {
  online: 0,
  busy: 1,
  offline: 2,
};

function sortContacts(a: Contact, b: Contact): number {
  const status = statusRank[a.status] - statusRank[b.status];
  if (status !== 0) return status;
  const unread = Number(b.unread > 0) - Number(a.unread > 0);
  if (unread !== 0) return unread;
  return a.nickname.localeCompare(b.nickname, undefined, { sensitivity: 'base' });
}

export function ContactList({
  contacts,
  groups = [],
  selectedId,
  selectedGroupId = null,
  loading = false,
  onSelect,
  onSelectGroup,
  onCreateGroup,
  onJoinGroup,
  onLeaveGroup,
  onLogout,
}: ContactListProps) {
  const { t } = useTranslation();
  const [query, setQuery] = useState('');
  const listRef = useRef<HTMLDivElement>(null);
  const savedScrollTopRef = useRef(0);
  const normalizedQuery = query.trim().toLowerCase();
  const onlineCount = contacts.filter((contact) => contact.status === 'online').length;
  const unreadCount = contacts.reduce((total, contact) => total + contact.unread, 0);
  const allContactsOffline = contacts.length > 0 && onlineCount === 0;

  useEffect(() => {
    const frame = window.requestAnimationFrame(() => {
      if (listRef.current) listRef.current.scrollTop = savedScrollTopRef.current;
    });
    return () => window.cancelAnimationFrame(frame);
  }, [selectedId, selectedGroupId]);

  const filteredContacts = useMemo(() => {
    return contacts
      .filter((contact) => {
        if (!normalizedQuery) return true;
        return contact.nickname.toLowerCase().includes(normalizedQuery) || String(contact.id).includes(normalizedQuery);
      })
      .sort(sortContacts);
  }, [contacts, normalizedQuery]);

  const filteredGroups = useMemo(() => {
    return groups.filter((group) => {
      if (!normalizedQuery) return true;
      return group.name.toLowerCase().includes(normalizedQuery) || String(group.groupId).includes(normalizedQuery);
    });
  }, [groups, normalizedQuery]);

  const hasSearch = normalizedQuery.length > 0;
  const friends = useFriendStore((s) => s.friends);
  const friendRequests = useFriendStore((s) => s.friendRequests);
  const friendIds = new Set(friends.map((friend) => friend.id));
  const onlineFriends = filteredContacts.filter((contact) => friendIds.has(contact.id) && contact.status === 'online');
  const offlineFriends = filteredContacts.filter((contact) => friendIds.has(contact.id) && contact.status !== 'online');
  const nonFriends = filteredContacts.filter((contact) => !friendIds.has(contact.id));
  const contactSections = [
    { label: t('contactList.onlineFriends'), items: onlineFriends },
    { label: t('contactList.offlineFriends'), items: offlineFriends },
    { label: t('contactList.otherContacts'), items: nonFriends },
  ].filter((section) => section.items.length > 0);

  return (
    <aside className="flex h-full w-sidebar flex-shrink-0 flex-col border-r border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar">
      <div className="border-b border-light-border px-4 py-3 dark:border-dark-border">
        <div className="mb-3 flex items-center justify-between">
          <div>
            <h2 className="text-xs font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
              {t('contactList.contactsHeading')}
            </h2>
            <p className="text-[10px] text-light-muted/70 dark:text-dark-muted/70">
              <Counter value={onlineCount} className="font-semibold text-light-online" /> {t('contactList.onlineCount', { count: onlineCount })}<Counter value={unreadCount} className="font-semibold text-dark-highlight" /> {t('contactList.unreadCount', { count: unreadCount })}
            </p>
          </div>
          <motion.button
            type="button"
            onClick={onLogout}
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:bg-dark-highlight/10 hover:text-dark-highlight dark:text-dark-muted"
          >
            {t('contactList.logout')}
          </motion.button>
        </div>
        <label htmlFor="contact-search" className="sr-only">
          {t('contactList.searchLabel')}
        </label>
        <input
          id="contact-search"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          placeholder={t('contactList.searchPlaceholder')}
          className="w-full rounded-lg border border-light-border bg-white px-3 py-2 text-xs text-light-text placeholder-light-muted outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted"
        />
        {allContactsOffline && (
          <div className="mt-2 rounded-lg border border-amber-400/30 bg-amber-400/10 px-3 py-2 text-[11px] text-amber-700 dark:text-amber-200">
            {t('contactList.offlineWarning')}
          </div>
        )}
      </div>

      <div
        ref={listRef}
        onScroll={() => {
          savedScrollTopRef.current = listRef.current?.scrollTop ?? 0;
        }}
        className="flex-1 overflow-y-auto py-1"
      >
        <AnimatePresence mode="wait" initial={false}>
          {loading ? (
            <motion.div key="loading" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
              <ContactSkeleton />
            </motion.div>
          ) : (
            <motion.div key="contacts" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
              {filteredContacts.length === 0 && (
                <p className="px-4 py-6 text-center text-xs text-light-muted dark:text-dark-muted">
                  {hasSearch ? t('contactList.noMatch') : t('contactList.noContacts')}
                </p>
              )}
              {friendRequests.length > 0 && !hasSearch && (
                <div className="mb-2">
                  <div className="px-4 pb-1 pt-2 text-[10px] font-semibold uppercase tracking-wider text-dark-highlight">
                    {t('contactList.friendRequests', { count: friendRequests.length })}
                  </div>
                  {friendRequests.map((request) => (
                    <FriendRequestRow key={`request-${request.id}`} request={request} />
                  ))}
                </div>
              )}
              <AnimatePresence initial={false} mode="popLayout">
                {contactSections.map((section) => (
                  <div key={section.label}>
                    <div className="px-4 pb-1 pt-2 text-[10px] font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
                      {section.label}
                    </div>
                    {section.items.map((contact) => {
                      const active = contact.id === selectedId && selectedGroupId === null;
                      const row = <ContactRow contact={contact} active={active} onSelect={onSelect} />;
                      const wrapped = (
                        <FadeContent duration={0.24} threshold={0.01}>
                          {row}
                        </FadeContent>
                      );
                      const content = active ? (
                        <BorderGlow borderRadius={0} glowRadius={20} glowIntensity={0.6} animated>
                          {wrapped}
                        </BorderGlow>
                      ) : (
                        wrapped
                      );
                      return (
                        <motion.div
                          key={`contact-${contact.id}`}
                          layout
                          initial={{ opacity: 0, y: 8 }}
                          animate={{ opacity: 1, y: 0 }}
                          exit={{ opacity: 0, y: -8, transition: { duration: 0.12, ease: 'easeIn' } }}
                        >
                          {content}
                        </motion.div>
                      );
                    })}
                  </div>
                ))}
              </AnimatePresence>

              <div className="mt-2 border-t border-light-border pt-2 dark:border-dark-border">
                <div className="px-4 pb-1 text-[10px] font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
                  <span>{t('contactList.groupsHeading')}</span>
                  <div className="mt-1 flex flex-wrap gap-1">
                    <button
                      type="button"
                      onClick={() => {
                        const name = window.prompt(t('contactList.createGroupPrompt'));
                        if (name?.trim()) onCreateGroup?.(name.trim());
                      }}
                      className="rounded bg-dark-highlight/10 px-2 py-0.5 text-[10px] text-dark-highlight hover:bg-dark-highlight/20"
                    >
                      {t('contactList.createGroup')}
                    </button>
                    <button
                      type="button"
                      onClick={() => {
                        const raw = window.prompt(t('contactList.joinGroupPrompt'));
                        const groupId = Number(raw);
                        if (Number.isFinite(groupId) && groupId > 0) onJoinGroup?.(groupId);
                      }}
                      className="rounded bg-dark-highlight/10 px-2 py-0.5 text-[10px] text-dark-highlight hover:bg-dark-highlight/20"
                    >
                      {t('contactList.joinGroup')}
                    </button>
                    {selectedGroupId && (
                      <button
                        type="button"
                        onClick={() => onLeaveGroup?.(selectedGroupId)}
                        className="rounded bg-red-500/10 px-2 py-0.5 text-[10px] text-red-500 hover:bg-red-500/20"
                      >
                        {t('contactList.leaveGroup')}
                      </button>
                    )}
                  </div>
                </div>
                {filteredGroups.length === 0 && (
                  <div className="mx-3 rounded-lg border border-dashed border-light-border px-3 py-4 text-xs text-light-muted dark:border-dark-border dark:text-dark-muted">
                    {hasSearch ? t('contactList.noGroupsMatch') : t('contactList.noGroups')}
                  </div>
                )}
                <AnimatePresence initial={false} mode="popLayout">
                  {filteredGroups.map((group) => {
                    const active = group.groupId === selectedGroupId;
                    const row = <GroupRow group={group} active={active} onSelect={onSelectGroup} />;
                    const wrapped = (
                      <FadeContent duration={0.24} threshold={0.01}>
                        {row}
                      </FadeContent>
                    );
                    const content = active ? (
                      <BorderGlow borderRadius={0} glowRadius={20} glowIntensity={0.6} animated>
                        {wrapped}
                      </BorderGlow>
                    ) : (
                      wrapped
                    );
                    return (
                      <motion.div
                        key={`group-${group.groupId}`}
                        layout
                        initial={{ opacity: 0, y: 8 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: -8, transition: { duration: 0.12, ease: 'easeIn' } }}
                      >
                        {content}
                      </motion.div>
                    );
                  })}
                </AnimatePresence>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </div>
    </aside>
  );
}

export default ContactList;
