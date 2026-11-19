import { HTMLAttributes, ReactNode, useEffect, useRef } from 'react';
import { useReducedMotion } from 'framer-motion';
import { gsap } from 'gsap';
import { ScrollTrigger } from 'gsap/ScrollTrigger';

gsap.registerPlugin(ScrollTrigger);

interface FadeContentProps extends HTMLAttributes<HTMLDivElement> {
  children: ReactNode;
  container?: Element | string | null;
  blur?: boolean;
  duration?: number;
  delay?: number;
  threshold?: number;
  initialOpacity?: number;
}

export default function FadeContent({
  children,
  container,
  blur = false,
  duration = 0.32,
  delay = 0,
  threshold = 0.08,
  initialOpacity = 0,
  className = '',
  ...props
}: FadeContentProps) {
  const ref = useRef<HTMLDivElement>(null);
  const reduceMotion = useReducedMotion();

  useEffect(() => {
    const el = ref.current;
    if (!el) return undefined;

    if (reduceMotion) {
      gsap.set(el, { autoAlpha: 1, clearProps: 'filter,transform,willChange' });
      return undefined;
    }

    let scroller: Element | Window = window;
    if (typeof container === 'string') {
      scroller = document.querySelector(container) ?? window;
    } else if (container) {
      scroller = container;
    }

    gsap.set(el, {
      autoAlpha: initialOpacity,
      y: 8,
      filter: blur ? 'blur(8px)' : 'blur(0px)',
      willChange: 'opacity, transform, filter',
    });

    const tween = gsap.to(el, {
      autoAlpha: 1,
      y: 0,
      filter: 'blur(0px)',
      duration,
      delay,
      ease: 'power2.out',
      paused: true,
      onComplete: () => gsap.set(el, { clearProps: 'willChange' }),
    });

    const trigger = ScrollTrigger.create({
      trigger: el,
      scroller,
      start: `top ${(1 - threshold) * 100}%`,
      once: true,
      onEnter: () => tween.play(),
    });

    return () => {
      trigger.kill();
      tween.kill();
      gsap.killTweensOf(el);
    };
  }, [blur, container, delay, duration, initialOpacity, reduceMotion, threshold]);

  return (
    <div ref={ref} className={className} {...props}>
      {children}
    </div>
  );
}
