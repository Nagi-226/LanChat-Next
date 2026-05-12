import { useEffect, useRef, useState } from 'react';
import type { ChatMessage } from './ChatArea';

interface Member {
  id: number;
  nickname: string;
  status?: 'online' | 'offline' | 'busy';
}

interface GroupChatAreaProps {
  messages: ChatMessage[];
  members: Member[];
  currentUserId: number;
  groupName: string;
  onSend: (content: string) => void;
}

const statusDot: Record<string, string> = {
  online: 'bg-[#52c41a]',
  offline: 'bg-[#666666]',
  busy: 'bg-[#faad14]',
};

function fmtTime(ts: number): string {
  return new Date(ts).toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
}

export function GroupChatArea({ messages, members, currentUserId, groupName, onSend }: GroupChatAreaProps) {
  const [input, setInput] = useState('');
  const [showMembers, setShowMembers] = useState(true);
  const bottomRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

  const handleSend = () => {
    const trimmed = input.trim();
    if (!trimmed) return;
    onSend(trimmed);
    setInput('');
  };

  return (
    <div className="flex min-h-0 flex-1">
      <div className="flex min-h-0 flex-1 flex-col">
        <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
          <span className="text-sm font-semibold text-light-text dark:text-dark-text"># {groupName}</span>
          <button
            type="button"
            onClick={() => setShowMembers((v) => !v)}
            className="rounded px-2 py-1 text-xs text-light-muted hover:text-dark-highlight dark:text-dark-muted"
          >
            {showMembers ? 'Hide members' : 'Show members'}
          </button>
        </div>

        <div className="flex-1 overflow-y-auto px-4 py-3">
          {messages.map((msg) => {
            const isSelf = msg.fromId === currentUserId;
            return (
              <div key={msg.id} className={`mb-3 flex gap-2 ${isSelf ? 'flex-row-reverse' : 'flex-row'}`}>
                <div className="flex h-9 w-9 flex-shrink-0 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text">
                  {msg.nickname.charAt(0)}
                </div>
                <div className={`max-w-[70%] ${isSelf ? 'items-end' : 'items-start'}`}>
                  <div className="mb-0.5 text-xs text-light-muted dark:text-dark-muted">
                    {isSelf ? 'You' : msg.nickname}
                  </div>
                  <div className={`rounded-bubble px-3 py-2 text-sm leading-relaxed ${isSelf ? 'bg-light-bubble-self text-white dark:bg-dark-bubble-self' : 'bg-light-bubble-other text-light-text dark:bg-dark-bubble-other dark:text-dark-text'}`}>
                    {msg.content}
                  </div>
                  <div className="mt-0.5 text-[10px] text-light-muted dark:text-dark-muted">
                    {fmtTime(msg.timestamp)}
                  </div>
                </div>
              </div>
            );
          })}
          <div ref={bottomRef} />
        </div>

        <div className="border-t border-light-border p-3 dark:border-dark-border">
          <div className="flex items-end gap-2">
            <textarea
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && !e.shiftKey) {
                  e.preventDefault();
                  handleSend();
                }
              }}
              placeholder="Message this group..."
              rows={1}
              className="max-h-[120px] min-h-[40px] flex-1 resize-none rounded-lg border border-light-border bg-white px-3 py-2 text-sm text-light-text placeholder-light-muted focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text"
            />
            <button
              type="button"
              onClick={handleSend}
              disabled={!input.trim()}
              className="flex h-10 w-10 items-center justify-center rounded-lg bg-dark-highlight text-white disabled:opacity-50"
              aria-label="Send group message"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M1.5 1.5L15 8L1.5 14.5L4.5 8L1.5 1.5Z" />
              </svg>
            </button>
          </div>
        </div>
      </div>

      {showMembers && (
        <div className="w-panel flex-shrink-0 border-l border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar">
          <div className="flex h-12 items-center border-b border-light-border px-4 dark:border-dark-border">
            <span className="text-xs font-semibold text-light-muted dark:text-dark-muted">
              Members ({members.length})
            </span>
          </div>
          <div className="overflow-y-auto py-1">
            {members.map((m) => (
              <div key={m.id} className="flex items-center gap-3 px-4 py-2.5">
                <div className="relative flex-shrink-0">
                  <div className="flex h-8 w-8 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text">
                    {m.nickname.charAt(0)}
                  </div>
                  <span className={`absolute bottom-0 right-0 h-2.5 w-2.5 rounded-full border-2 border-light-sidebar dark:border-dark-sidebar ${statusDot[m.status ?? 'offline']}`} />
                </div>
                <span className="text-sm text-light-text dark:text-dark-text">
                  {m.nickname}{m.id === currentUserId ? ' (you)' : ''}
                </span>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

export default GroupChatArea;
