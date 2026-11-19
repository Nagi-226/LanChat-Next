import type { ChangeEvent, KeyboardEvent } from 'react';
import { useCallback, useRef, useState } from 'react';

export function useChatInput(onSend: (content: string) => void, disabled = false, onTyping?: () => void) {
  const [input, setInput] = useState('');
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const lastTypingAt = useRef(0);

  const resetInput = useCallback(() => {
    setInput('');
    if (textareaRef.current) textareaRef.current.style.height = 'auto';
  }, []);

  const handleSend = useCallback(() => {
    const trimmed = input.trim();
    if (!trimmed || disabled) return;
    onSend(trimmed);
    resetInput();
  }, [disabled, input, onSend, resetInput]);

  const handleKeyDown = useCallback((e: KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  }, [handleSend]);

  const handleInput = useCallback((e: ChangeEvent<HTMLTextAreaElement>) => {
    setInput(e.target.value);
    const now = Date.now();
    if (e.target.value.trim() && onTyping && now - lastTypingAt.current > 2500) {
      lastTypingAt.current = now;
      onTyping();
    }
    const el = e.target;
    el.style.height = 'auto';
    el.style.height = `${Math.min(el.scrollHeight, 120)}px`;
  }, [onTyping]);

  return {
    input,
    textareaRef,
    handleInput,
    handleKeyDown,
    handleSend,
  };
}
