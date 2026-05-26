import { memo, useCallback, useEffect, useRef, useState } from 'react';
import { AnimatePresence, motion, useReducedMotion } from 'framer-motion';
import type { ChatMessage } from './ChatArea';
import ClickSpark from '../lib/ClickSpark';
import { AnimatedListItem } from '../lib/AnimatedList';
import { DateDivider, fmtTime, getInitials, MessageSkeleton } from '../lib/utils';
import { isNearScrollBottom, nextUnreadCount } from '../lib/scrollLogic';

export interface Member {
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
  loading?: boolean;
}

const statusDot: Record<NonNullable<Member['status']>, string> = {
  online: 'bg-light-online',
  offline: 'bg-light-muted dark:bg-dark-muted',
  busy: 'bg-amber-400',
};

const GroupMessageBubble = memo(function GroupMessageBubble({ message, currentUserId }: { message: ChatMessage; currentUserId: number }) {
  const isSelf = message.fromId === currentUserId;

  if (message.contentType === 'system') {
    return (
      <div className="mb-3 flex justify-center">
        <span className="rounded-full bg-light-sidebar px-3 py-1 text-xs text-light-muted dark:bg-dark-sidebar dark:text-dark-muted">
          {message.content}
        </span>
      </div>
    );
  }

  return (
    <motion.div layout initial={{ opacity: 0, y: 8 }} animate={{ opacity: 1, y: 0 }} className={`mb-3 flex gap-2 ${isSelf ? 'flex-row-reverse' : 'flex-row'}`}>
      <div className="flex h-9 w-9 flex-shrink-0 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text">
        {getInitials(message.nickname)}
      </div>
      <div className={`max-w-[70%] ${isSelf ? 'items-end' : 'items-start'}`}>
        <div className="mb-0.5 text-xs text-light-muted dark:text-dark-muted">
          {isSelf ? 'You' : message.nickname}
        </div>
        <div className={`rounded-bubble px-3 py-2 text-sm leading-relaxed ${isSelf ? 'rounded-br-[2px] bg-light-bubble-self text-white dark:bg-dark-bubble-self' : 'rounded-bl-[2px] bg-light-bubble-other text-light-text dark:bg-dark-bubble-other dark:text-dark-text'}`}>
          {message.content}
        </div>
        <div className="mt-0.5 flex items-center gap-1 text-[10px] text-light-muted dark:text-dark-muted">
          <span>{fmtTime(message.timestamp)}</span>
          {message.status === 'sending' && <span>sending...</span>}
          {message.status === 'failed' && <span className="text-red-500 dark:text-red-300">failed</span>}
        </div>
      </div>
    </motion.div>
  );
});

export function GroupChatArea({ messages, members, currentUserId, groupName, onSend, loading = false }: GroupChatAreaProps) {
  const [input, setInput] = useState('');
  const [showMembers, setShowMembers] = useState(true);
  const [showNewMessages, setShowNewMessages] = useState(false);
  const [newMessageCount, setNewMessageCount] = useState(0);
  const bottomRef = useRef<HTMLDivElement>(null);
  const scrollRef = useRef<HTMLDivElement>(null);
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const wasNearBottomRef = useRef(true);
  const previousMessageCountRef = useRef(messages.length);
  const reduceMotion = useReducedMotion();

  const scrollToBottom = useCallback((behavior: ScrollBehavior = 'smooth') => {
    bottomRef.current?.scrollIntoView({ behavior: reduceMotion ? 'auto' : behavior });
  }, [reduceMotion]);

  const syncNearBottom = useCallback(() => {
    const el = scrollRef.current;
    if (!el) return true;
    const nearBottom = isNearScrollBottom(el);
    wasNearBottomRef.current = nearBottom;
    if (nearBottom) {
      setShowNewMessages(false);
      setNewMessageCount(0);
    }
    return nearBottom;
  }, []);

  useEffect(() => {
    const previousCount = previousMessageCountRef.current;
    const hasNewMessages = messages.length > previousCount;

    if (wasNearBottomRef.current || !hasNewMessages) {
      scrollToBottom(hasNewMessages ? 'smooth' : 'auto');
      setShowNewMessages(false);
      setNewMessageCount(0);
    } else {
      setNewMessageCount((count) => nextUnreadCount(count, previousCount, messages.length, false));
      setShowNewMessages(true);
    }

    previousMessageCountRef.current = messages.length;
  }, [messages.length, scrollToBottom]);

  const handleSend = () => {
    const trimmed = input.trim();
    if (!trimmed) return;
    onSend(trimmed);
    setInput('');
    if (textareaRef.current) textareaRef.current.style.height = 'auto';
  };

  const handleInput = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
    setInput(e.target.value);
    const el = e.target;
    el.style.height = 'auto';
    el.style.height = `${Math.min(el.scrollHeight, 120)}px`;
  };

  const handleJumpToLatest = () => {
    scrollToBottom('smooth');
    wasNearBottomRef.current = true;
    setShowNewMessages(false);
    setNewMessageCount(0);
  };

  const messageCount = messages.length;
  const memberCount = members.length;
  let lastDivider = '';

  return (
    <div className="flex min-h-0 flex-1">
      <div className="flex min-h-0 flex-1 flex-col">
        <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
          <div>
            <span className="text-sm font-semibold text-light-text dark:text-dark-text"># {groupName}</span>
            <p className="text-[10px] text-light-muted dark:text-dark-muted">
              {messageCount} messages - {memberCount} members
            </p>
          </div>
          <motion.button
            type="button"
            onClick={() => setShowMembers((v) => !v)}
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="rounded px-2 py-1 text-xs text-light-muted hover:text-dark-highlight dark:text-dark-muted"
            aria-label={showMembers ? 'Hide group members' : 'Show group members'}
          >
            {showMembers ? 'Hide members' : 'Show members'}
          </motion.button>
        </div>

        <div className="relative min-h-0 flex-1">
          <div ref={scrollRef} onScroll={syncNearBottom} className="h-full overflow-y-auto px-4 py-3">
            <AnimatePresence mode="wait" initial={false}>
              {loading ? (
                <motion.div key="loading" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
                  <MessageSkeleton />
                </motion.div>
              ) : messages.length === 0 ? (
                <motion.div key="empty" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }} className="flex h-full items-center justify-center px-6 text-center">
                  <p className="text-sm text-light-muted dark:text-dark-muted">
                    No group messages yet. Say hello to #{groupName}.
                  </p>
                </motion.div>
              ) : (
                <motion.div key="messages" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
                  <AnimatePresence initial={false} mode="popLayout">
                    {messages.map((msg, index) => {
                      const divider = new Date(msg.timestamp).toDateString();
                      const shouldShowDivider = divider !== lastDivider;
                      lastDivider = divider;
                      return (
                        <AnimatedListItem key={msg.id} index={index} exit={{ opacity: 0, y: -10, transition: { duration: 0.12, ease: 'easeIn' } }}>
                          {shouldShowDivider && <DateDivider timestamp={msg.timestamp} />}
                          <GroupMessageBubble message={msg} currentUserId={currentUserId} />
                        </AnimatedListItem>
                      );
                    })}
                  </AnimatePresence>
                </motion.div>
              )}
            </AnimatePresence>
            <div ref={bottomRef} />
          </div>

          <AnimatePresence>
            {showNewMessages && (
              <motion.button
                type="button"
                onClick={handleJumpToLatest}
                initial={{ opacity: 0, y: 18, scale: 0.95 }}
                animate={{ opacity: 1, y: 0, scale: 1 }}
                exit={{ opacity: 0, y: 12, scale: 0.95 }}
                transition={{ type: 'spring', stiffness: 420, damping: 30 }}
                className="absolute bottom-3 right-4 rounded-full border border-dark-highlight/30 bg-dark-highlight/90 px-3 py-1.5 text-xs font-semibold text-white shadow-lg backdrop-blur hover:bg-dark-highlight"
                aria-label={`Jump to ${newMessageCount || ''} new group messages`.trim()}
              >
                New messages{newMessageCount > 0 ? ` (${newMessageCount})` : ''}
              </motion.button>
            )}
          </AnimatePresence>
        </div>

        <div className="border-t border-light-border p-3 dark:border-dark-border">
          <div className="flex items-end gap-2">
            <label htmlFor="group-message-input" className="sr-only">
              Message group {groupName}
            </label>
            <div className="flex flex-1 rounded-lg transition-shadow focus-within:shadow-[0_0_0_3px_rgba(233,69,96,0.16)]">
              <textarea
                id="group-message-input"
                ref={textareaRef}
                value={input}
                onChange={handleInput}
                onKeyDown={(e) => {
                  if (e.key === 'Enter' && !e.shiftKey) {
                    e.preventDefault();
                    handleSend();
                  }
                }}
                placeholder="Message this group..."
                rows={1}
                className="max-h-[120px] min-h-[40px] flex-1 resize-none rounded-lg border border-light-border bg-white px-3 py-2 text-sm text-light-text placeholder-light-muted focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted"
              />
            </div>
            <ClickSpark sparkColor="#e94560" sparkCount={6} sparkSize={6}>
              <motion.button
                type="button"
                onClick={handleSend}
                disabled={!input.trim()}
                whileHover={{ scale: 1.05 }}
                whileTap={{ scale: 0.95 }}
                className="flex h-10 w-10 items-center justify-center rounded-lg bg-dark-highlight text-white transition-opacity hover:opacity-80 disabled:opacity-50"
                aria-label="Send group message"
              >
                <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true">
                  <path d="M1.5 1.5L15 8L1.5 14.5L4.5 8L1.5 1.5Z" />
                </svg>
              </motion.button>
            </ClickSpark>
          </div>
        </div>
      </div>

      <AnimatePresence initial={false}>
        {showMembers && (
          <motion.div
            key="members"
            initial={{ width: 0, opacity: 0, x: 24 }}
            animate={{ width: 320, opacity: 1, x: 0 }}
            exit={{ width: 0, opacity: 0, x: 24 }}
            transition={{ duration: 0.22 }}
            className="overflow-hidden border-l border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar"
          >
            <div className="w-panel">
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
                        {getInitials(m.nickname)}
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
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

export default GroupChatArea;
