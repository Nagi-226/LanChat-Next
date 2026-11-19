import { useEffect } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import { useTranslation } from './i18n';

export type ToastType = 'info' | 'error' | 'success';

export interface ToastItem {
  id: string;
  type: ToastType;
  message: string;
}

interface ToastContainerProps {
  toasts: ToastItem[];
  onDismiss: (id: string) => void;
}

const toneClass: Record<ToastType, string> = {
  info: 'border-light-accent/40 bg-light-accent/10 text-light-text dark:border-dark-accent dark:bg-dark-accent/30 dark:text-dark-text',
  error: 'border-red-400/40 bg-red-500/10 text-red-700 dark:text-red-300',
  success: 'border-green-400/40 bg-green-500/10 text-green-700 dark:text-green-300',
};

function ToastCard({ toast, onDismiss }: { toast: ToastItem; onDismiss: (id: string) => void }) {
  const { t } = useTranslation();
  useEffect(() => {
    const timer = setTimeout(() => onDismiss(toast.id), 4200);
    return () => clearTimeout(timer);
  }, [onDismiss, toast.id]);

  return (
    <motion.div
      role="status"
      layout
      initial={{ opacity: 0, x: 48, scale: 0.96 }}
      animate={{ opacity: 1, x: 0, scale: 1 }}
      exit={{ opacity: 0, y: -18, scale: 0.96 }}
      transition={{ duration: 0.2, ease: 'easeOut' }}
      className={`flex w-80 max-w-[calc(100vw-2rem)] items-start gap-3 rounded-lg border px-3 py-2.5 text-sm shadow-lg backdrop-blur will-change-transform ${toneClass[toast.type]}`}
    >
      <span className="mt-0.5 text-xs font-bold uppercase tracking-wide">{toast.type}</span>
      <span className="min-w-0 flex-1 break-words">{toast.message}</span>
      <button
        type="button"
        onClick={() => onDismiss(toast.id)}
        className="rounded px-1 text-xs opacity-70 hover:opacity-100"
        aria-label={t('toast.dismiss')}
      >
        x
      </button>
    </motion.div>
  );
}

export function ToastContainer({ toasts, onDismiss }: ToastContainerProps) {
  return (
    <div className="pointer-events-none fixed right-4 top-14 z-50 flex flex-col gap-2">
      <AnimatePresence initial={false} mode="popLayout">
        {toasts.map((toast) => (
          <div key={toast.id} className="pointer-events-auto">
            <ToastCard toast={toast} onDismiss={onDismiss} />
          </div>
        ))}
      </AnimatePresence>
    </div>
  );
}

export default ToastContainer;

