import { memo, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import type { ChatMessage } from './ChatArea';
import { AnimatedListItem } from '../lib/AnimatedList';
import { ChatContentSwitcher, MessageComposer, NewMessagesFAB } from '../lib/ChatComponents';
import { DateDivider, fmtTime, getInitials } from '../lib/utils';
import { useChatInput } from '../hooks/useChatInput';
import { useChatScroll } from '../hooks/useChatScroll';

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
  onRetryFailed?: (message: ChatMessage) => void;
  friendIds?: number[];
  onAddFriend?: (memberId: number) => void;
  loading?: boolean;
  connected?: boolean;
}

const statusDot: Record<NonNullable<Member['status']>, string> = {
  online: 'bg-light-online',
  offline: 'bg-light-muted dark:bg-dark-muted',
  busy: 'bg-amber-400',
};

const GroupMessageBubble = memo(function GroupMessageBubble({
  message,
  currentUserId,
  onRetryFailed,
}: {
  message: ChatMessage;
  currentUserId: number;
  onRetryFailed?: (message: ChatMessage) => void;
}) {
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

export function GroupChatArea({
  messages,
  members,
  currentUserId,
  groupName,
  onSend,
  onRetryFailed,
  friendIds = [],
  onAddFriend,
  loading = false,
  connected = true,
}: GroupChatAreaProps) {
  const [showMembers, setShowMembers] = useState(true);
  const chatInput = useChatInput(onSend, !connected);
  const chatScroll = useChatScroll(messages.length);
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
          <div ref={chatScroll.scrollRef} onScroll={chatScroll.syncNearBottom} className="h-full overflow-y-auto px-4 py-3">
            <ChatContentSwitcher
              loading={loading}
              empty={messages.length === 0}
              emptyText={`No group messages yet. Say hello to #${groupName}.`}
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
                        <GroupMessageBubble message={msg} currentUserId={currentUserId} onRetryFailed={onRetryFailed} />
                      </div>
                    </AnimatedListItem>
                  );
                })}
              </AnimatePresence>
            </ChatContentSwitcher>
            <div ref={chatScroll.bottomRef} />
          </div>

          <AnimatePresence>
            {chatScroll.showNewMessages && <NewMessagesFAB count={chatScroll.newMessageCount} label="new group messages" onClick={chatScroll.jumpToLatest} />}
          </AnimatePresence>
        </div>

        <MessageComposer
          id="group-message-input"
          label={`Message group ${groupName}`}
          input={chatInput.input}
          textareaRef={chatInput.textareaRef}
          placeholder="Message this group..."
          disabled={!connected}
          disabledTitle="Not connected"
          sendLabel="Send group message"
          onInput={chatInput.handleInput}
          onKeyDown={chatInput.handleKeyDown}
          onSend={chatInput.handleSend}
        />
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
                {members.length === 0 ? (
                  <div className="px-4 py-6 text-center text-xs text-light-muted dark:text-dark-muted">
                    No member data yet. It will appear after the group syncs.
                  </div>
                ) : (
                  members.map((m) => {
                    const canAddFriend = m.id !== currentUserId && !friendIds.includes(m.id) && Boolean(onAddFriend);
                    return (
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
                      {canAddFriend && (
                        <button
                          type="button"
                          onClick={() => onAddFriend?.(m.id)}
                          className="ml-auto rounded bg-dark-highlight/10 px-2 py-1 text-[10px] font-semibold text-dark-highlight hover:bg-dark-highlight/20"
                        >
                          Add
                        </button>
                      )}
                    </div>
                    );
                  })
                )}
              </div>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

export default GroupChatArea;
