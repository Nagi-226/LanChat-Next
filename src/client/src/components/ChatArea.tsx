import { memo } from 'react';
import { useTranslation } from '../lib/i18n';
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
  status?: 'queued' | 'sending' | 'sent' | 'failed';
  edited?: boolean;
  deleted?: boolean;
  reactions?: string[];
  read?: boolean;
}

interface ChatAreaProps {
  messages: ChatMessage[];
  currentUserId: number;
  contactName: string;
  onSend: (content: string) => void;
  onRetryFailed?: (message: ChatMessage) => void;
  onTyping?: () => void;
  onEditMessage?: (message: ChatMessage, content: string) => void;
  onDeleteMessage?: (message: ChatMessage) => void;
  onReactMessage?: (message: ChatMessage, reaction: string) => void;
  loading?: boolean;
  connected?: boolean;
}

interface MessageBubbleProps {
  message: ChatMessage;
  currentUserId: number;
  showSender?: boolean;
  onRetryFailed?: (message: ChatMessage) => void;
  onEditMessage?: (message: ChatMessage, content: string) => void;
  onDeleteMessage?: (message: ChatMessage) => void;
  onReactMessage?: (message: ChatMessage, reaction: string) => void;
}

const MessageBubble = memo(function MessageBubble({ message, currentUserId, showSender = true, onRetryFailed, onEditMessage, onDeleteMessage, onReactMessage }: MessageBubbleProps) {
  const { t } = useTranslation();
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
            {isSelf ? t('chat.you') : message.nickname}
          </div>
        )}
        <div
          className={`rounded-bubble px-3 py-2 text-sm leading-relaxed ${
            isSelf
              ? 'rounded-br-[2px] bg-light-bubble-self text-white dark:bg-dark-bubble-self'
              : 'rounded-bl-[2px] bg-light-bubble-other text-light-text dark:bg-dark-bubble-other dark:text-dark-text'
          }`}
        >
          {message.deleted ? t('chat.deletedMessage') : message.content}
        </div>
        {message.reactions && message.reactions.length > 0 && (
          <div className="mt-1 flex flex-wrap gap-1">
            {message.reactions.map((reaction) => (
              <span key={reaction} className="rounded-full bg-dark-highlight/10 px-2 py-0.5 text-[10px] text-dark-highlight">
                {reaction}
              </span>
            ))}
          </div>
        )}
        <div className="mt-0.5 flex items-center gap-1 text-[10px] text-light-muted dark:text-dark-muted">
          <span>{fmtTime(message.timestamp)}</span>
          {message.edited && !message.deleted && <span>{t('chat.edited')}</span>}
          {isSelf && message.read && <span>{t('chat.read')}</span>}
          {message.status === 'queued' && <span>{t('chat.queued')}</span>}
          {message.status === 'sending' && <span>{t('chat.sending')}</span>}
          {message.status === 'failed' && <span className="text-red-500 dark:text-red-300">{t('chat.failed')}</span>}
          {message.status === 'failed' && onRetryFailed && (
            <button
              type="button"
              onClick={() => onRetryFailed(message)}
              className="rounded px-1 text-red-500 underline-offset-2 hover:underline dark:text-red-300"
            >
              {t('chat.retry')}
            </button>
          )}
          {!message.deleted && (
            <button type="button" onClick={() => onReactMessage?.(message, '👍')} className="rounded px-1 hover:text-dark-highlight">
              {t('chat.react')}
            </button>
          )}
          {isSelf && !message.deleted && onEditMessage && (
            <button
              type="button"
              onClick={() => {
                const next = window.prompt(t('chat.editPrompt'), message.content);
                if (next?.trim()) onEditMessage(message, next.trim());
              }}
              className="rounded px-1 hover:text-dark-highlight"
            >
              {t('chat.edit')}
            </button>
          )}
          {isSelf && !message.deleted && onDeleteMessage && (
            <button type="button" onClick={() => onDeleteMessage(message)} className="rounded px-1 hover:text-red-500">
              {t('chat.delete')}
            </button>
          )}
        </div>
      </div>
    </motion.div>
  );
});

export function ChatArea({ messages, currentUserId, contactName, onSend, onRetryFailed, onTyping, onEditMessage, onDeleteMessage, onReactMessage, loading = false, connected = true }: ChatAreaProps) {
  const { t } = useTranslation();
  const chatInput = useChatInput(onSend, false, onTyping);
  const chatScroll = useChatScroll(messages.length);
  const messageCount = messages.length;
  const title = messageCount > 0 ? t('chat.messagesCount', { count: messageCount }) : t('chat.noMessages');
  let lastDivider = '';

  return (
    <div className="flex min-h-0 flex-1 flex-col">
      <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <div>
          <span className="text-sm font-semibold text-light-text dark:text-dark-text">
            {contactName}
          </span>
          <p className="text-[10px] text-light-muted dark:text-dark-muted">
            {t('chat.directChat', { name: title })}
          </p>
        </div>
      </div>

      <div className="relative min-h-0 flex-1">
        <div ref={chatScroll.scrollRef} onScroll={chatScroll.syncNearBottom} className="h-full overflow-y-auto px-4 py-3">
          <ChatContentSwitcher
            loading={loading}
            empty={messages.length === 0}
            emptyText={t('chat.emptyState', { name: contactName })}
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
                      <MessageBubble
                        message={msg}
                        currentUserId={currentUserId}
                        onRetryFailed={onRetryFailed}
                        onEditMessage={onEditMessage}
                        onDeleteMessage={onDeleteMessage}
                        onReactMessage={onReactMessage}
                      />
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
        placeholder={t('chat.placeholder')}
        disabled={false}
        disabledTitle={connected ? undefined : t('chat.queuedWhenOffline')}
        sendLabel={t('chat.sendMessage')}
        onInput={chatInput.handleInput}
        onKeyDown={chatInput.handleKeyDown}
        onSend={chatInput.handleSend}
      />
    </div>
  );
}

export default ChatArea;
