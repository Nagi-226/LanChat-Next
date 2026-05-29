import { memo } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import { AnimatedListItem } from '../lib/AnimatedList';
import { ChatContentSwitcher, MessageComposer, NewMessagesFAB } from '../lib/ChatComponents';
import { DateDivider, fmtTime, getInitials } from '../lib/utils';
import { useChatInput } from '../hooks/useChatInput';
import { useChatScroll } from '../hooks/useChatScroll';

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
  onRetryFailed?: (message: ChatMessage) => void;
  loading?: boolean;
  connected?: boolean;
}

interface MessageBubbleProps {
  message: ChatMessage;
  currentUserId: number;
  showSender?: boolean;
  onRetryFailed?: (message: ChatMessage) => void;
}

const MessageBubble = memo(function MessageBubble({ message, currentUserId, showSender = true, onRetryFailed }: MessageBubbleProps) {
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
          {message.status === 'failed' && onRetryFailed && (
            <button
              type="button"
              onClick={() => onRetryFailed(message)}
              className="rounded px-1 text-red-500 underline-offset-2 hover:underline dark:text-red-300"
            >
              retry
            </button>
          )}
        </div>
      </div>
    </motion.div>
  );
});

export function ChatArea({ messages, currentUserId, contactName, onSend, onRetryFailed, loading = false, connected = true }: ChatAreaProps) {
  const chatInput = useChatInput(onSend, !connected);
  const chatScroll = useChatScroll(messages.length);
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
        <div ref={chatScroll.scrollRef} onScroll={chatScroll.syncNearBottom} className="h-full overflow-y-auto px-4 py-3">
          <ChatContentSwitcher
            loading={loading}
            empty={messages.length === 0}
            emptyText={`No message history with ${contactName}. Send the first one.`}
          >
            <AnimatePresence initial={false} mode="popLayout">
              {messages.map((msg, index) => {
                const divider = new Date(msg.timestamp).toDateString();
                const shouldShowDivider = divider !== lastDivider;
                lastDivider = divider;
                return (
                  <AnimatedListItem key={msg.id} index={index} exit={{ opacity: 0, y: -10, transition: { duration: 0.12, ease: 'easeIn' } }}>
                    {shouldShowDivider && <DateDivider timestamp={msg.timestamp} />}
                    <div data-message-id={msg.id}>
                      <MessageBubble message={msg} currentUserId={currentUserId} onRetryFailed={onRetryFailed} />
                    </div>
                  </AnimatedListItem>
                );
              })}
            </AnimatePresence>
          </ChatContentSwitcher>
          <div ref={chatScroll.bottomRef} />
        </div>

        <AnimatePresence>
          {chatScroll.showNewMessages && <NewMessagesFAB count={chatScroll.newMessageCount} onClick={chatScroll.jumpToLatest} />}
        </AnimatePresence>
      </div>

      <MessageComposer
        id="direct-message-input"
        label={`Message to ${contactName}`}
        input={chatInput.input}
        textareaRef={chatInput.textareaRef}
        placeholder="Type a message... (Enter to send, Shift+Enter for newline)"
        disabled={!connected}
        disabledTitle="Not connected"
        sendLabel="Send message"
        onInput={chatInput.handleInput}
        onKeyDown={chatInput.handleKeyDown}
        onSend={chatInput.handleSend}
      />
    </div>
  );
}

export default ChatArea;
