import { HTMLAttributes, ReactNode, useEffect, useRef } from 'react';
import { useReducedMotion } from 'framer-motion';
import { gsap } from 'gsap';
import { ScrollTrigger } from 'gsap/ScrollTrigger';

interface AnimatedContentProps extends HTMLAttributes<HTMLDivElement> {
  children: ReactNode;
  container?: Element | string | null;
  distance?: number;
  direction?: 'vertical' | 'horizontal';
  reverse?: boolean;
  duration?: number;
  delay?: number;
  threshold?: number;
}

export default function AnimatedContent({
  children,
  container,
  distance = 28,
  direction = 'vertical',
  reverse = false,
  duration = 0.42,
  delay = 0,
  threshold = 0.05,
  className = '',
  ...props
}: AnimatedContentProps) {
  const ref = useRef<HTMLDivElement>(null);
  const reduceMotion = useReducedMotion();

  useEffect(() => {
    const el = ref.current;
    if (!el) return undefined;

    if (reduceMotion) {
      gsap.set(el, { autoAlpha: 1, clearProps: 'transform,willChange' });
      return undefined;
    }

    let scroller: Element | Window = window;
    if (typeof container === 'string') {
      scroller = document.querySelector(container) ?? window;
    } else if (container) {
      scroller = container;
    }

    const axis = direction === 'horizontal' ? 'x' : 'y';
    const offset = reverse ? -distance : distance;

    gsap.set(el, {
      autoAlpha: 0,
      [axis]: offset,
      scale: 0.985,
      willChange: 'opacity, transform',
    });

    const tween = gsap.to(el, {
      autoAlpha: 1,
      [axis]: 0,
      scale: 1,
      duration,
      delay,
      ease: 'power3.out',
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

    window.requestAnimationFrame(() => ScrollTrigger.refresh());

    return () => {
      trigger.kill();
      tween.kill();
      gsap.killTweensOf(el);
    };
  }, [container, delay, direction, distance, duration, reduceMotion, reverse, threshold]);

  return (
    <div ref={ref} className={className} {...props}>
      {children}
    </div>
  );
}
