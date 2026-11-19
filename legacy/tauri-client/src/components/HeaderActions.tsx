import { motion } from 'framer-motion';
import { useUIStore } from '../stores/uiStore';
import { useAuthStore, useFriendStore } from '../stores/chatStore';
import { useConnectionStore } from '../stores/connectionStore';
import { t } from '../lib/i18n';

export function HeaderActions() {
  const sidebarCollapsed = useUIStore((s) => s.sidebarCollapsed);
  const aiPanelOpen = useUIStore((s) => s.aiPanelOpen);
  const toggleSidebar = useUIStore((s) => s.toggleSidebar);
  const toggleAIPanel = useUIStore((s) => s.toggleAIPanel);
  const currentUser = useAuthStore((s) => s.currentUser);
  const auth = useAuthStore((s) => s.auth);
  const friendRequests = useFriendStore((s) => s.friendRequests);
  const status = useConnectionStore((s) => s.status);

  const statusLabel = status === 'connected' ? t('app.header.status.live') : status === 'connecting' ? t('app.header.status.connecting') : t('app.header.status.offline');

  return (
    <div className="ml-auto flex items-center gap-2">
      <span className="rounded-full border border-light-border bg-light-sidebar px-2.5 py-1 text-[10px] font-medium text-light-muted dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-muted">
        {statusLabel}
      </span>
      {currentUser && (
        <span className="hidden rounded-full bg-dark-highlight/10 px-2.5 py-1 text-[10px] font-medium text-dark-highlight sm:inline-flex">
          {auth.loading ? t('app.header.syncing') : currentUser.nickname}
        </span>
      )}
      {friendRequests.length > 0 && (
        <span className="rounded-full bg-dark-highlight px-2.5 py-1 text-[10px] font-bold text-white">
          {t('app.header.pendingRequests', { count: friendRequests.length })}
        </span>
      )}
      <motion.button
        type="button"
        onClick={toggleSidebar}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={sidebarCollapsed ? t('aria.expandContactList') : t('aria.collapseContactList')}
      >
        {sidebarCollapsed ? t('app.header.expandList') : t('app.header.collapseList')}
      </motion.button>
      <motion.button
        type="button"
        onClick={toggleAIPanel}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={aiPanelOpen ? t('aria.hideAIPanel') : t('aria.showAIPanel')}
      >
        {aiPanelOpen ? t('app.header.hideAI') : t('app.header.showAI')}
      </motion.button>
    </div>
  );
}

export default HeaderActions;
