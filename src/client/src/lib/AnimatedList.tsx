import { ReactNode, useRef } from 'react';
import { motion, useInView, useReducedMotion } from 'framer-motion';
import type { MotionProps } from 'framer-motion';

interface AnimatedListItemProps {
  children: ReactNode;
  index?: number;
  className?: string;
  exit?: MotionProps['exit'];
}

export function AnimatedListItem({ children, index = 0, className = '', exit }: AnimatedListItemProps) {
  const ref = useRef<HTMLDivElement>(null);
  const reduceMotion = useReducedMotion();
  const inView = useInView(ref, { amount: 0.2, once: true });

  return (
    <motion.div
      ref={ref}
      className={className}
      initial={reduceMotion ? false : { opacity: 0, y: 14, scale: 0.98 }}
      animate={reduceMotion || inView ? { opacity: 1, y: 0, scale: 1 } : { opacity: 0, y: 14, scale: 0.98 }}
      exit={reduceMotion ? { opacity: 1 } : exit}
      transition={{ duration: 0.22, delay: Math.min(index * 0.018, 0.16), ease: 'easeOut' }}
    >
      {children}
    </motion.div>
  );
}
