import { ReactNode, useMemo } from 'react';
import { motion, useReducedMotion } from 'framer-motion';

interface GradientTextProps {
  children: ReactNode;
  className?: string;
  colors?: string[];
  animationSpeed?: number;
  showBorder?: boolean;
}

export default function GradientText({
  children,
  className = '',
  colors = ['#e94560', '#1677ff', '#52c41a'],
  animationSpeed = 8,
  showBorder = false,
}: GradientTextProps) {
  const reduceMotion = useReducedMotion();
  const gradient = useMemo(() => `linear-gradient(90deg, ${[...colors, colors[0]].join(', ')})`, [colors]);

  return (
    <motion.span
      className={`relative inline-flex max-w-fit items-center overflow-hidden rounded-full ${showBorder ? 'border border-dark-highlight/30 px-2 py-0.5' : ''} ${className}`}
      style={{
        backgroundImage: gradient,
        backgroundSize: '300% 100%',
        WebkitBackgroundClip: 'text',
        backgroundClip: 'text',
        color: 'transparent',
      }}
      animate={reduceMotion ? undefined : { backgroundPosition: ['0% 50%', '100% 50%', '0% 50%'] }}
      transition={{ duration: animationSpeed, repeat: Infinity, ease: 'linear' }}
    >
      {children}
    </motion.span>
  );
}
