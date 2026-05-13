import BorderGlow from '../lib/BorderGlow';

export interface Contact {
  id: number;
  nickname: string;
  headId?: number;
  status: 'online' | 'offline' | 'busy';
  unread: number;
  lastMessage?: string;
}

interface ContactListProps {
  contacts: Contact[];
  selectedId: number | null;
  onSelect: (id: number) => void;
  onLogout: () => void;
}

const statusColor: Record<Contact['status'], string> = {
  online: 'bg-[#52c41a]',
  offline: 'bg-[#666666]',
  busy: 'bg-[#faad14]',
};

const statusLabel: Record<Contact['status'], string> = {
  online: 'Online',
  offline: 'Offline',
  busy: 'Busy',
};

function getInitials(name: string): string {
  const trimmed = name.trim();
  if (!trimmed) return '?';
  const parts = trimmed.split(/\s+/).slice(0, 2);
  return parts.map((part) => part.charAt(0)).join('').toUpperCase();
}

export function ContactList({ contacts, selectedId, onSelect, onLogout }: ContactListProps) {
  return (
    <aside className="flex w-sidebar flex-shrink-0 flex-col border-r border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar">
      <div className="flex h-10 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <div>
          <h2 className="text-xs font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
            Contacts
          </h2>
          <p className="text-[10px] text-light-muted/70 dark:text-dark-muted/70">
            {contacts.length} available
          </p>
        </div>
        <button
          type="button"
          onClick={onLogout}
          className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:bg-dark-highlight/10 hover:text-dark-highlight dark:text-dark-muted"
        >
          Logout
        </button>
      </div>

      <div className="flex-1 overflow-y-auto py-1">
        {contacts.length === 0 && (
          <p className="px-4 py-6 text-center text-xs text-light-muted dark:text-dark-muted">
            No contacts yet
          </p>
        )}
        {contacts.map((c) => {
          const active = c.id === selectedId;
          const btn = (
            <button
              key={c.id}
              type="button"
              onClick={() => onSelect(c.id)}
              className={`flex w-full items-center gap-3 px-4 py-2.5 text-left transition-colors ${
                active ? 'bg-dark-highlight/10 dark:bg-dark-highlight/15' : 'hover:bg-dark-hover'
              }`}
            >
              <div className="relative flex-shrink-0">
                <div className="flex h-10 w-10 items-center justify-center rounded-full bg-dark-accent text-sm text-dark-text">
                  {getInitials(c.nickname)}
                </div>
                <span
                  className={`absolute bottom-0 right-0 h-3 w-3 rounded-full border-2 border-light-sidebar dark:border-dark-sidebar ${statusColor[c.status]}`}
                  title={statusLabel[c.status]}
                />
              </div>

              <div className="min-w-0 flex-1">
                <div className="flex items-center justify-between gap-2">
                  <div className="truncate text-sm font-medium text-light-text dark:text-dark-text">
                    {c.nickname}
                  </div>
                  <span className="shrink-0 text-[10px] text-light-muted dark:text-dark-muted">
                    #{c.id}
                  </span>
                </div>
                {c.lastMessage ? (
                  <div className="truncate text-[11px] text-light-muted dark:text-dark-muted">
                    {c.lastMessage}
                  </div>
                ) : (
                  <div className="truncate text-[11px] text-light-muted/60 dark:text-dark-muted/60">
                    No message preview yet
                  </div>
                )}
              </div>

              {c.unread > 0 && (
                <span className="flex h-5 min-w-[20px] flex-shrink-0 items-center justify-center rounded-full bg-dark-highlight px-1.5 text-[10px] font-bold text-white">
                  {c.unread > 99 ? '99+' : c.unread}
                </span>
              )}
            </button>
          );
          return active ? (
            <BorderGlow key={c.id} borderRadius={0} glowRadius={20} glowIntensity={0.6} animated>
              {btn}
            </BorderGlow>
          ) : (
            btn
          );
        })}
      </div>
    </aside>
  );
}

export default ContactList;
