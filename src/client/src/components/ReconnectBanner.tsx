import { useEffect, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import { useConnectionStore } from '../stores/connectionStore';
import { t } from '../lib/i18n';

export function ReconnectBanner() {
  const status = useConnectionStore((s) => s.status);
  const retryDueAt = useConnectionStore((s) => s.retryDueAt);
  const connect = useConnectionStore((s) => s.connect);
  const host = useConnectionStore((s) => s.host);
  const port = useConnectionStore((s) => s.port);
  const clearReconnect = useConnectionStore((s) => s.clearReconnect);
  const [dismissed, setDismissed] = useState(false);
  const [secondsLeft, setSecondsLeft] = useState(0);

  useEffect(() => {
    if (!retryDueAt) {
      setDismissed(false);
      setSecondsLeft(0);
      return undefined;
    }

    const tick = () => setSecondsLeft(Math.max(0, Math.ceil((retryDueAt - Date.now()) / 1000)));
    tick();
    const timer = window.setInterval(tick, 500);
    return () => window.clearInterval(timer);
  }, [retryDueAt]);

  const visible = status === 'connecting' && Boolean(retryDueAt) && !dismissed;

  return (
    <AnimatePresence initial={false}>
      {visible && (
        <motion.div
          initial={{ height: 0, opacity: 0, y: -12 }}
          animate={{ height: 'auto', opacity: 1, y: 0 }}
          exit={{ height: 0, opacity: 0, y: -12 }}
          transition={{ duration: 0.22 }}
          className="overflow-hidden border-b border-amber-300/40 bg-amber-400/15 text-xs text-amber-800 dark:text-amber-200"
        >
          <div className="flex items-center justify-center gap-3 px-4 py-2">
            <span>{t('app.reconnect.message', { seconds: secondsLeft })}</span>
            <motion.button
              type="button"
              onClick={() => connect(host, port)}
              whileHover={{ scale: 1.04 }}
              whileTap={{ scale: 0.96 }}
              className="rounded bg-amber-500/20 px-2 py-0.5 font-medium hover:bg-amber-500/30"
            >
              {t('app.reconnect.retryNow')}
            </motion.button>
            <button
              type="button"
              onClick={() => {
                setDismissed(true);
                clearReconnect();
              }}
              className="rounded px-2 py-0.5 hover:bg-amber-500/20"
              aria-label={t('aria.dismissReconnect')}
            >
              x
            </button>
          </div>
        </motion.div>
      )}
    </AnimatePresence>
  );
}

export default ReconnectBanner;
