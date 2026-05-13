import { useState, useRef, useEffect } from 'react';
import ClickSpark from '../lib/ClickSpark';

export interface ChatMessage {
  id: string;
  fromId: number;
  nickname: string;
  content: string;
  contentType: 'text' | 'image' | 'file' | 'system';
  timestamp: number;
}

interface ChatAreaProps {
  messages: ChatMessage[];
  currentUserId: number;
  contactName: string;
  onSend: (content: string) => void;
}

function fmtTime(ts: number): string {
  const d = new Date(ts);
  return d.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' });
}

function getInitials(name: string): string {
  const trimmed = name.trim();
  if (!trimmed) return '?';
  const parts = trimmed.split(/\s+/).slice(0, 2);
  return parts.map((part) => part.charAt(0)).join('').toUpperCase();
}

export function ChatArea({ messages, currentUserId, contactName, onSend }: ChatAreaProps) {
  const [input, setInput] = useState('');
  const bottomRef = useRef<HTMLDivElement>(null);
  const textareaRef = useRef<HTMLTextAreaElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [messages]);

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

  const messageCount = messages.length;
  const title = messageCount > 0 ? `${messageCount} messages` : 'No message history yet';

  return (
    <div className="flex min-h-0 flex-1 flex-col">
      <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <div>
          <span className="text-sm font-semibold text-light-text dark:text-dark-text">
            {contactName}
          </span>
          <p className="text-[10px] text-light-muted dark:text-dark-muted">
            Direct chat · {title}
          </p>
        </div>
      </div>

      <div className="flex-1 overflow-y-auto px-4 py-3">
        {messages.length === 0 && (
          <div className="flex h-full items-center justify-center">
            <p className="text-sm text-light-muted dark:text-dark-muted">
              No messages yet. Send the first one.
            </p>
          </div>
        )}
        {messages.map((msg) => {
          const isSelf = msg.fromId === currentUserId;
          return (
            <div
              key={msg.id}
              className={`mb-3 flex gap-2 ${isSelf ? 'flex-row-reverse' : 'flex-row'}`}
            >
              <div className="flex h-9 w-9 flex-shrink-0 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text dark:bg-dark-accent">
                {getInitials(msg.nickname)}
              </div>

              <div className={`max-w-[70%] ${isSelf ? 'items-end' : 'items-start'}`}>
                <div className="mb-0.5 text-xs text-light-muted dark:text-dark-muted">
                  {isSelf ? 'You' : msg.nickname}
                </div>
                <div
                  className={`rounded-bubble px-3 py-2 text-sm leading-relaxed ${
                    isSelf
                      ? 'rounded-br-[2px] bg-light-bubble-self text-white dark:bg-dark-bubble-self'
                      : 'rounded-bl-[2px] bg-light-bubble-other text-light-text dark:bg-dark-bubble-other dark:text-dark-text'
                  }`}
                >
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
            ref={textareaRef}
            value={input}
            onChange={handleInput}
            onKeyDown={handleKeyDown}
            placeholder="Type a message... (Enter to send, Shift+Enter for newline)"
            rows={1}
            className="max-h-[120px] min-h-[40px] flex-1 resize-none rounded-lg border border-light-border bg-white px-3 py-2 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
          />
          <ClickSpark sparkColor="#e94560" sparkCount={6} sparkSize={6}>
            <button
              type="button"
              onClick={handleSend}
              disabled={!input.trim()}
              className="flex h-10 w-10 flex-shrink-0 items-center justify-center rounded-lg bg-dark-highlight text-white transition-colors hover:bg-[#d63850] disabled:cursor-not-allowed disabled:opacity-50"
              aria-label="Send message"
            >
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M1.5 1.5L15 8L1.5 14.5L4.5 8L1.5 1.5Z" />
              </svg>
            </button>
          </ClickSpark>
        </div>
      </div>
    </div>
  );
}

export default ChatArea;
