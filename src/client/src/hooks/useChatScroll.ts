import { useCallback, useEffect, useRef, useState } from 'react';
import { useReducedMotion } from 'framer-motion';
import { isNearScrollBottom, nextUnreadCount } from '../lib/scrollLogic';

export function useChatScroll(messageCount: number) {
  const [showNewMessages, setShowNewMessages] = useState(false);
  const [newMessageCount, setNewMessageCount] = useState(0);
  const bottomRef = useRef<HTMLDivElement>(null);
  const scrollRef = useRef<HTMLDivElement>(null);
  const wasNearBottomRef = useRef(true);
  const previousMessageCountRef = useRef(messageCount);
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

  const jumpToLatest = useCallback(() => {
    scrollToBottom('smooth');
    wasNearBottomRef.current = true;
    setShowNewMessages(false);
    setNewMessageCount(0);
  }, [scrollToBottom]);

  useEffect(() => {
    const previousCount = previousMessageCountRef.current;
    const hasNewMessages = messageCount > previousCount;

    if (wasNearBottomRef.current || !hasNewMessages) {
      scrollToBottom(hasNewMessages ? 'smooth' : 'auto');
      setShowNewMessages(false);
      setNewMessageCount(0);
    } else {
      setNewMessageCount((count) => nextUnreadCount(count, previousCount, messageCount, false));
      setShowNewMessages(true);
    }

    previousMessageCountRef.current = messageCount;
  }, [messageCount, scrollToBottom]);

  useEffect(() => {
    const handler = (event: Event) => {
      const id = (event as CustomEvent<{ id?: string }>).detail?.id;
      if (!id || !scrollRef.current) return;
      const target = scrollRef.current.querySelector<HTMLElement>(`[data-message-id="${CSS.escape(id)}"]`);
      target?.scrollIntoView({ behavior: reduceMotion ? 'auto' : 'smooth', block: 'center' });
      target?.animate(
        [
          { outlineColor: 'rgba(233,69,96,0)', outlineWidth: '0px' },
          { outlineColor: 'rgba(233,69,96,0.85)', outlineWidth: '2px' },
          { outlineColor: 'rgba(233,69,96,0)', outlineWidth: '0px' },
        ],
        { duration: 1200, easing: 'ease-out' },
      );
    };
    window.addEventListener('lanchat-scroll-message', handler);
    return () => window.removeEventListener('lanchat-scroll-message', handler);
  }, [reduceMotion]);

  return {
    bottomRef,
    scrollRef,
    showNewMessages,
    newMessageCount,
    syncNearBottom,
    jumpToLatest,
  };
}
