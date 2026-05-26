import { memo, useEffect, useMemo, useRef, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import type { GroupInfo } from '../../../../protocol/message_types';
import BorderGlow from '../lib/BorderGlow';
import Counter from '../lib/Counter';
import FadeContent from '../lib/FadeContent';
import { getInitials } from '../lib/utils';

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
  onLogout: () => void;
}

const statusColor: Record<Contact['status'], string> = {
  online: 'bg-light-online',
  offline: 'bg-light-muted dark:bg-dark-muted',
  busy: 'bg-amber-400',
};

const statusLabel: Record<Contact['status'], string> = {
  online: 'Online',
  offline: 'Offline',
  busy: 'Busy',
};

const statusRank: Record<Contact['status'], number> = {
  online: 0,
  busy: 1,
  offline: 2,
};

interface ContactRowProps {
  contact: Contact;
  active: boolean;
  onSelect: (id: number) => void;
}

const ContactRow = memo(function ContactRow({ contact, active, onSelect }: ContactRowProps) {
  return (
    <motion.button
      type="button"
      layout
      onClick={() => onSelect(contact.id)}
      whileHover={{ x: 2 }}
      whileTap={{ scale: 0.99 }}
      className={`relative flex w-full items-center gap-3 px-4 py-2.5 text-left transition-colors ${
        active ? 'bg-dark-highlight/10 dark:bg-dark-highlight/15' : 'hover:bg-dark-hover'
      }`}
      aria-label={`Open chat with ${contact.nickname}`}
    >
      {active && <motion.span layoutId="active-conversation" className="absolute inset-y-1 left-0 w-1 rounded-r-full bg-dark-highlight" />}
      <div className="relative flex-shrink-0">
        <div className="flex h-10 w-10 items-center justify-center rounded-full bg-dark-accent text-sm text-dark-text">
          {getInitials(contact.nickname)}
        </div>
        <span
          className={`absolute bottom-0 right-0 h-3 w-3 rounded-full border-2 border-light-sidebar dark:border-dark-sidebar ${statusColor[contact.status]} ${contact.status === 'online' ? 'animate-pulse' : ''}`}
          title={statusLabel[contact.status]}
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
          <div className="truncate text-[11px] font-medium text-dark-highlight">typing...</div>
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
    </motion.button>
  );
});

interface GroupRowProps {
  group: GroupInfo;
  active: boolean;
  onSelect?: (id: number) => void;
}

const GroupRow = memo(function GroupRow({ group, active, onSelect }: GroupRowProps) {
  const members = group.users?.length ?? 0;
  return (
    <motion.button
      type="button"
      layout
      onClick={() => onSelect?.(group.groupId)}
      disabled={!onSelect}
      whileHover={{ x: onSelect ? 2 : 0 }}
      whileTap={{ scale: onSelect ? 0.99 : 1 }}
      className={`relative flex w-full items-center gap-3 px-4 py-2.5 text-left transition-colors disabled:cursor-not-allowed disabled:opacity-60 ${
        active ? 'bg-dark-highlight/10 dark:bg-dark-highlight/15' : 'hover:bg-dark-hover'
      }`}
      aria-label={`Open group ${group.name}`}
    >
      {active && <motion.span layoutId="active-conversation" className="absolute inset-y-1 left-0 w-1 rounded-r-full bg-dark-highlight" />}
      <div className="flex h-10 w-10 flex-shrink-0 items-center justify-center rounded-full bg-light-accent text-sm font-bold text-white dark:bg-dark-accent">
        #
      </div>
      <div className="min-w-0 flex-1">
        <div className="truncate text-sm font-medium text-light-text dark:text-dark-text">{group.name}</div>
        <div className="truncate text-[11px] text-light-muted dark:text-dark-muted">
          {members > 0 ? `${members} members` : `Group #${group.groupId}`}
        </div>
      </div>
    </motion.button>
  );
});

function ContactSkeleton() {
  return (
    <div className="space-y-2 px-4 py-3" aria-label="Loading contacts">
      {Array.from({ length: 5 }, (_, index) => (
        <div key={index} className="flex items-center gap-3">
          <div className="h-10 w-10 animate-pulse rounded-full bg-light-border dark:bg-dark-border" />
          <div className="min-w-0 flex-1 space-y-2">
            <div className="h-3 w-2/3 animate-pulse rounded bg-light-border dark:bg-dark-border" />
            <div className="h-2 w-1/2 animate-pulse rounded bg-light-border dark:bg-dark-border" />
          </div>
        </div>
      ))}
    </div>
  );
}

function sortContacts(a: Contact, b: Contact): number {
  const status = statusRank[a.status] - statusRank[b.status];
  if (status !== 0) return status;
  const unread = Number(b.unread > 0) - Number(a.unread > 0);
  if (unread !== 0) return unread;
  return a.nickname.localeCompare(b.nickname, undefined, { sensitivity: 'base' });
}

export function ContactList({ contacts, groups = [], selectedId, selectedGroupId = null, loading = false, onSelect, onSelectGroup, onLogout }: ContactListProps) {
  const [query, setQuery] = useState('');
  const listRef = useRef<HTMLDivElement>(null);
  const savedScrollTopRef = useRef(0);
  const normalizedQuery = query.trim().toLowerCase();
  const onlineCount = contacts.filter((contact) => contact.status === 'online').length;
  const unreadCount = contacts.reduce((total, contact) => total + contact.unread, 0);

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

  return (
    <aside className="flex h-full w-sidebar flex-shrink-0 flex-col border-r border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar">
      <div className="border-b border-light-border px-4 py-3 dark:border-dark-border">
        <div className="mb-3 flex items-center justify-between">
          <div>
            <h2 className="text-xs font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
              Contacts
            </h2>
            <p className="text-[10px] text-light-muted/70 dark:text-dark-muted/70">
              <Counter value={onlineCount} className="font-semibold text-light-online" /> online - <Counter value={unreadCount} className="font-semibold text-dark-highlight" /> unread
            </p>
          </div>
          <motion.button
            type="button"
            onClick={onLogout}
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:bg-dark-highlight/10 hover:text-dark-highlight dark:text-dark-muted"
          >
            Logout
          </motion.button>
        </div>
        <label htmlFor="contact-search" className="sr-only">
          Search contacts or groups
        </label>
        <input
          id="contact-search"
          value={query}
          onChange={(e) => setQuery(e.target.value)}
          placeholder="Search name or ID"
          className="w-full rounded-lg border border-light-border bg-white px-3 py-2 text-xs text-light-text placeholder-light-muted outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted"
        />
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
                  {hasSearch ? 'No contacts match your search' : 'No contacts yet'}
                </p>
              )}
              <AnimatePresence initial={false} mode="popLayout">
                {filteredContacts.map((contact) => {
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
              </AnimatePresence>

              <div className="mt-2 border-t border-light-border pt-2 dark:border-dark-border">
                <div className="px-4 pb-1 text-[10px] font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
                  Groups
                </div>
                {filteredGroups.length === 0 && (
                  <p className="px-4 py-3 text-xs text-light-muted dark:text-dark-muted">
                    {hasSearch ? 'No groups match your search' : 'No joined groups'}
                  </p>
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
