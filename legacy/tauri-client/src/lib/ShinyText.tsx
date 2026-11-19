import { motion, useReducedMotion } from 'framer-motion';

interface ShinyTextProps {
  text: string;
  disabled?: boolean;
  speed?: number;
  className?: string;
  color?: string;
  shineColor?: string;
}

export default function ShinyText({
  text,
  disabled = false,
  speed = 2.4,
  className = '',
  color = '#a0a0b0',
  shineColor = '#ffffff',
}: ShinyTextProps) {
  const reduceMotion = useReducedMotion();
  const off = disabled || reduceMotion;

  return (
    <motion.span
      className={`inline-block ${className}`}
      style={{
        backgroundImage: `linear-gradient(110deg, ${color} 0%, ${color} 35%, ${shineColor} 50%, ${color} 65%, ${color} 100%)`,
        backgroundSize: '220% auto',
        WebkitBackgroundClip: 'text',
        backgroundClip: 'text',
        WebkitTextFillColor: 'transparent',
      }}
      animate={off ? { backgroundPosition: '0% center' } : { backgroundPosition: ['150% center', '-50% center'] }}
      transition={{ duration: speed, repeat: off ? 0 : Infinity, ease: 'linear', repeatDelay: 0.8 }}
    >
      {text}
    </motion.span>
  );
}
