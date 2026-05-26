import { motion, useMotionValue, useSpring, useTransform, useReducedMotion } from 'framer-motion';
import { useEffect } from 'react';

interface CounterProps {
  value: number;
  className?: string;
  duration?: number;
  formatter?: (value: number) => string;
}

export default function Counter({ value, className = '', duration = 0.45, formatter = (latest) => String(Math.round(latest)) }: CounterProps) {
  const reduceMotion = useReducedMotion();
  const motionValue = useMotionValue(value);
  const spring = useSpring(motionValue, { duration: reduceMotion ? 0 : duration, bounce: 0 });
  const display = useTransform(spring, (latest) => formatter(latest));

  useEffect(() => {
    motionValue.set(value);
  }, [motionValue, value]);

  return <motion.span className={`inline-block tabular-nums ${className}`}>{display}</motion.span>;
}
