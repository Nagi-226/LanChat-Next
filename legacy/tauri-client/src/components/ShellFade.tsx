import type { ReactNode } from 'react';
import { motion } from 'framer-motion';

export function ShellFade({ showSplash, children }: { showSplash: boolean; children: ReactNode }) {
  return (
    <motion.div
      className="flex min-h-0 flex-1 flex-col"
      initial={{ opacity: 0 }}
      animate={{ opacity: showSplash ? 0 : 1 }}
      transition={{ duration: 0.24, ease: 'easeOut' }}
    >
      {children}
    </motion.div>
  );
}

export default ShellFade;
