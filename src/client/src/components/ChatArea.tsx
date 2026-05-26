import { memo, useCallback, useEffect, useRef, useState } from 'react';
import { AnimatePresence, motion, useReducedMotion } from 'framer-motion';
import ClickSpark from '../lib/ClickSpark';
import { AnimatedListItem } from '../lib/AnimatedList';
import { DateDivider, fmtTime, getInitials, MessageSkeleton } from '../lib/utils';
import { isNearScrollBottom, nextUnreadCount } from '../lib/scrollLogic';

export interface ChatMessage {
  id: string;
  fromId: number;
  nickname: string;
  content: string;
  contentType: 'text' | 'image' | 'file' | 'system';
  timestamp: number;
  status?: 'sending' | 'sent' | 'failed';
}

interface ChatAreaProps {
  messages: ChatMessage[];
  currentUserId: number;
  contactName: string;
  onSend: (content: string) => void;
  loading?: boolean;
}

interface MessageBubbleProps {
  message: ChatMessage;
  currentUserId: number;
  showSender?: boolean;
}

const MessageBubble = memo(function MessageBubble({ message, currentUserId, showSender = true }: MessageBubbleProps) {
  const isSelf = message.fromId === currentUserId;
  const isSystem = message.contentType === 'system';

  if (isSystem) {
    return (
      <div className="mb-3 flex justify-center">
        <span className="rounded-full bg-light-sidebar px-3 py-1 text-xs text-light-muted dark:bg-dark-sidebar dark:text-dark-muted">
          {message.content}
        </span>
      </div>
    );
  }

  return (
    <motion.div
      layout
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      className={`mb-3 flex gap-2 ${isSelf ? 'flex-row-reverse' : 'flex-row'}`}
    >
      <div className="flex h-9 w-9 flex-shrink-0 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text dark:bg-dark-accent">
        {getInitials(message.nickname)}
      </div>

      <div className={`max-w-[70%] ${isSelf ? 'items-end' : 'items-start'}`}>
        {showSender && (
          <div className="mb-0.5 text-xs text-light-muted dark:text-dark-muted">
            {isSelf ? 'You' : message.nickname}
          </div>
        )}
        <div
          className={`rounded-bubble px-3 py-2 text-sm leading-relaxed ${
            isSelf
              ? 'rounded-br-[2px] bg-light-bubble-self text-white dark:bg-dark-bubble-self'
              : 'rounded-bl-[2px] bg-light-bubble-other text-light-text dark:bg-dark-bubble-other dark:text-dark-text'
          }`}
        >
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

export function ChatArea({ messages, currentUserId, contactName, onSend, loading = false }: ChatAreaProps) {
  const [input, setInput] = useState('');
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

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
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
  const title = messageCount > 0 ? `${messageCount} messages` : 'No message history yet';
  let lastDivider = '';

  return (
    <div className="flex min-h-0 flex-1 flex-col">
      <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <div>
          <span className="text-sm font-semibold text-light-text dark:text-dark-text">
            {contactName}
          </span>
          <p className="text-[10px] text-light-muted dark:text-dark-muted">
            Direct chat - {title}
          </p>
        </div>
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
                  No message history with {contactName}. Send the first one.
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
                        <MessageBubble message={msg} currentUserId={currentUserId} />
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
              aria-label={`Jump to ${newMessageCount || ''} new messages`.trim()}
            >
              New messages{newMessageCount > 0 ? ` (${newMessageCount})` : ''}
            </motion.button>
          )}
        </AnimatePresence>
      </div>

      <div className="border-t border-light-border p-3 dark:border-dark-border">
        <div className="flex items-end gap-2">
          <label htmlFor="direct-message-input" className="sr-only">
            Message to {contactName}
          </label>
          <div className="flex flex-1 rounded-lg transition-shadow focus-within:shadow-[0_0_0_3px_rgba(233,69,96,0.16)]">
            <textarea
              id="direct-message-input"
              ref={textareaRef}
              value={input}
              onChange={handleInput}
              onKeyDown={handleKeyDown}
              placeholder="Type a message... (Enter to send, Shift+Enter for newline)"
              rows={1}
              className="max-h-[120px] min-h-[40px] flex-1 resize-none rounded-lg border border-light-border bg-white px-3 py-2 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
            />
          </div>
          <ClickSpark sparkColor="#e94560" sparkCount={6} sparkSize={6}>
            <motion.button
              type="button"
              onClick={handleSend}
              disabled={!input.trim()}
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
              className="flex h-10 w-10 flex-shrink-0 items-center justify-center rounded-lg bg-dark-highlight text-white transition-colors hover:bg-[#d63850] disabled:cursor-not-allowed disabled:opacity-50"
              aria-label="Send message"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true">
                <path d="M1.5 1.5L15 8L1.5 14.5L4.5 8L1.5 1.5Z" />
              </svg>
            </motion.button>
          </ClickSpark>
        </div>
      </div>
    </div>
  );
}

export default ChatArea;
