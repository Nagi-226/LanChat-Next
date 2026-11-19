import { ElementType, createElement, useEffect, useMemo, useRef, useState } from 'react';
import { gsap } from 'gsap';
import { useReducedMotion } from 'framer-motion';

interface TextTypeProps extends React.HTMLAttributes<HTMLElement> {
  text: string | string[];
  as?: ElementType;
  typingSpeed?: number;
  deletingSpeed?: number;
  pauseDuration?: number;
  initialDelay?: number;
  loop?: boolean;
  showCursor?: boolean;
  cursorCharacter?: React.ReactNode;
  cursorClassName?: string;
  className?: string;
}

export default function TextType({
  text,
  as: Component = 'span',
  typingSpeed = 48,
  deletingSpeed = 28,
  pauseDuration = 1800,
  initialDelay = 180,
  loop = true,
  showCursor = true,
  cursorCharacter = '|',
  cursorClassName = '',
  className = '',
  ...props
}: TextTypeProps) {
  const reduceMotion = useReducedMotion();
  const textArray = useMemo(() => (Array.isArray(text) ? text : [text]), [text]);
  const [displayedText, setDisplayedText] = useState(reduceMotion ? textArray[0] ?? '' : '');
  const [textIndex, setTextIndex] = useState(0);
  const [charIndex, setCharIndex] = useState(0);
  const [deleting, setDeleting] = useState(false);
  const cursorRef = useRef<HTMLSpanElement>(null);

  useEffect(() => {
    if (!showCursor || !cursorRef.current || reduceMotion) return undefined;
    const tween = gsap.to(cursorRef.current, {
      opacity: 0,
      duration: 0.55,
      repeat: -1,
      yoyo: true,
      ease: 'power2.inOut',
    });
    return () => {
      tween.kill();
    };
  }, [reduceMotion, showCursor]);

  useEffect(() => {
    if (reduceMotion) {
      setDisplayedText(textArray[0] ?? '');
      return undefined;
    }

    const currentText = textArray[textIndex] ?? '';
    let timeout: ReturnType<typeof setTimeout>;

    if (!deleting && charIndex === 0 && displayedText === '') {
      timeout = setTimeout(() => setCharIndex(1), initialDelay);
    } else if (!deleting && charIndex <= currentText.length) {
      timeout = setTimeout(() => {
        setDisplayedText(currentText.slice(0, charIndex));
        setCharIndex((value) => value + 1);
      }, typingSpeed);
    } else if (!deleting) {
      if (!loop && textIndex === textArray.length - 1) return undefined;
      timeout = setTimeout(() => setDeleting(true), pauseDuration);
    } else if (displayedText.length > 0) {
      timeout = setTimeout(() => setDisplayedText((value) => value.slice(0, -1)), deletingSpeed);
    } else {
      setDeleting(false);
      setTextIndex((value) => (value + 1) % textArray.length);
      setCharIndex(0);
      timeout = setTimeout(() => undefined, pauseDuration);
    }

    return () => clearTimeout(timeout);
  }, [charIndex, deleting, deletingSpeed, displayedText, initialDelay, loop, pauseDuration, reduceMotion, textArray, textIndex, typingSpeed]);

  return createElement(
    Component,
    { className: `inline-block whitespace-pre-wrap ${className}`, ...props },
    <span>{displayedText}</span>,
    showCursor && !reduceMotion && (
      <span ref={cursorRef} className={`ml-1 inline-block ${cursorClassName}`} aria-hidden="true">
        {cursorCharacter}
      </span>
    ),
  );
}
