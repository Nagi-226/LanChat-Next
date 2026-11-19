import { memo } from 'react';
import { motion } from 'framer-motion';
import type { GroupInfo } from '../../../../protocol/message_types';
import { useTranslation } from '../lib/i18n';

interface GroupRowProps {
  group: GroupInfo;
  active: boolean;
  onSelect?: (id: number) => void;
}

export const GroupRow = memo(function GroupRow({ group, active, onSelect }: GroupRowProps) {
  const { t } = useTranslation();
  const members = group.users?.length ?? 0;
  return (
    <motion.button
      type="button"
      layout
      onClick={() => onSelect?.(group.groupId)}
      disabled={!onSelect}
      whileHover={{ x: onSelect ? 2 : 0 }}
      whileTap={{ scale: onSelect ? 0.99 : 1 }}
      className={`relative flex w-full items-center gap-3 px-4 py-2.5 text-left transition-colors disabled:cursor-not-allowed disabled:opacity-60 ${
        active ? 'bg-dark-highlight/10 dark:bg-dark-highlight/15' : 'hover:bg-dark-hover'
      }`}
      aria-label={t('contactList.openGroup', { name: group.name })}
    >
      {active && <motion.span layoutId="active-conversation" className="absolute inset-y-1 left-0 w-1 rounded-r-full bg-dark-highlight" />}
      <div className="flex h-10 w-10 flex-shrink-0 items-center justify-center rounded-full bg-light-accent text-sm font-bold text-white dark:bg-dark-accent">
        #
      </div>
      <div className="min-w-0 flex-1">
        <div className="truncate text-sm font-medium text-light-text dark:text-dark-text">{group.name}</div>
        <div className="truncate text-[11px] text-light-muted dark:text-dark-muted">
          {members > 0 ? t('contactList.membersCount', { count: members }) : t('contactList.groupFallback', { id: group.groupId })}
        </div>
      </div>
    </motion.button>
  );
});

export default GroupRow;
