import type { ChangeEvent, KeyboardEvent, ReactNode, RefObject } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import ClickSpark from './ClickSpark';
import { MessageSkeleton } from './utils';

interface NewMessagesFABProps {
  count: number;
  label?: string;
  onClick: () => void;
}

export function NewMessagesFAB({ count, label = 'new messages', onClick }: NewMessagesFABProps) {
  return (
    <motion.button
      type="button"
      onClick={onClick}
      initial={{ opacity: 0, y: 18, scale: 0.95 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      exit={{ opacity: 0, y: 12, scale: 0.95 }}
      transition={{ type: 'spring', stiffness: 420, damping: 30 }}
      className="absolute bottom-3 right-4 rounded-full border border-dark-highlight/30 bg-dark-highlight/90 px-3 py-1.5 text-xs font-semibold text-white shadow-lg backdrop-blur hover:bg-dark-highlight"
      aria-label={`Jump to ${count || ''} ${label}`.trim()}
    >
      New messages{count > 0 ? ` (${count})` : ''}
    </motion.button>
  );
}

interface ChatContentSwitcherProps {
  loading: boolean;
  empty: boolean;
  emptyText: ReactNode;
  children: ReactNode;
}

export function ChatContentSwitcher({ loading, empty, emptyText, children }: ChatContentSwitcherProps) {
  return (
    <AnimatePresence mode="wait" initial={false}>
      {loading ? (
        <motion.div key="loading" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
          <MessageSkeleton />
        </motion.div>
      ) : empty ? (
        <motion.div
          key="empty"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.18 }}
          className="flex h-full items-center justify-center px-6 text-center"
        >
          <p className="text-sm text-light-muted dark:text-dark-muted">{emptyText}</p>
        </motion.div>
      ) : (
        <motion.div key="messages" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} transition={{ duration: 0.18 }}>
          {children}
        </motion.div>
      )}
    </AnimatePresence>
  );
}

interface MessageComposerProps {
  id: string;
  label: string;
  input: string;
  textareaRef: RefObject<HTMLTextAreaElement>;
  placeholder: string;
  disabled?: boolean;
  disabledTitle?: string;
  sendLabel: string;
  onInput: (event: ChangeEvent<HTMLTextAreaElement>) => void;
  onKeyDown: (event: KeyboardEvent<HTMLTextAreaElement>) => void;
  onSend: () => void;
}

export function MessageComposer({
  id,
  label,
  input,
  textareaRef,
  placeholder,
  disabled = false,
  disabledTitle,
  sendLabel,
  onInput,
  onKeyDown,
  onSend,
}: MessageComposerProps) {
  const sendDisabled = disabled || !input.trim();

  return (
    <div className="border-t border-light-border p-3 dark:border-dark-border">
      <div className="flex items-end gap-2">
        <label htmlFor={id} className="sr-only">
          {label}
        </label>
        <div className="flex flex-1 rounded-lg transition-shadow focus-within:shadow-[0_0_0_3px_rgba(233,69,96,0.16)]">
          <textarea
            id={id}
            ref={textareaRef}
            value={input}
            onChange={onInput}
            onKeyDown={onKeyDown}
            placeholder={disabled ? 'Disconnected...' : placeholder}
            disabled={disabled}
            title={disabled ? disabledTitle : undefined}
            rows={1}
            className="max-h-[120px] min-h-[40px] flex-1 resize-none rounded-lg border border-light-border bg-white px-3 py-2 text-sm text-light-text placeholder-light-muted transition-colors focus:border-dark-highlight focus:outline-none disabled:cursor-not-allowed disabled:opacity-60 dark:border-dark-accent dark:bg-dark-accent dark:text-dark-text dark:placeholder-dark-muted dark:focus:border-dark-highlight"
          />
        </div>
        <ClickSpark sparkColor="#e94560" sparkCount={6} sparkSize={6}>
          <motion.button
            type="button"
            onClick={onSend}
            disabled={sendDisabled}
            title={disabled ? (disabledTitle ?? 'Not connected') : undefined}
            whileHover={{ scale: sendDisabled ? 1 : 1.05 }}
            whileTap={{ scale: sendDisabled ? 1 : 0.95 }}
            className="flex h-10 w-10 flex-shrink-0 items-center justify-center rounded-lg bg-dark-highlight text-white transition-colors hover:bg-[#d63850] disabled:cursor-not-allowed disabled:opacity-50"
            aria-label={sendLabel}
          >
            <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true">
              <path d="M1.5 1.5L15 8L1.5 14.5L4.5 8L1.5 1.5Z" />
            </svg>
          </motion.button>
        </ClickSpark>
      </div>
    </div>
  );
}
