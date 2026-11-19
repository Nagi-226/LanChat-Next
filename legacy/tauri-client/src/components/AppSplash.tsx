import { motion } from 'framer-motion';
import GradientText from '../lib/GradientText';
import { t } from '../lib/i18n';

export function AppSplash() {
  return (
    <motion.div
      className="fixed inset-0 z-[60] flex flex-col items-center justify-center bg-light-bg dark:bg-dark-bg"
      initial={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.35 }}
      aria-label={t('app.splash.loading')}
    >
      <GradientText className="text-3xl font-black tracking-tight" animationSpeed={5}>
        LanChat-Next
      </GradientText>
      <motion.div
        className="mt-5 h-1 w-36 overflow-hidden rounded-full bg-light-border dark:bg-dark-border"
        initial={{ opacity: 0, y: 6 }}
        animate={{ opacity: 1, y: 0 }}
      >
        <motion.div
          className="h-full w-1/3 rounded-full bg-dark-highlight"
          animate={{ x: ['-100%', '320%'] }}
          transition={{ duration: 1.1, repeat: Infinity, ease: 'easeInOut' }}
        />
      </motion.div>
    </motion.div>
  );
}

export default AppSplash;
