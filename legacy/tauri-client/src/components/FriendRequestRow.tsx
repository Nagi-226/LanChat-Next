import { memo } from 'react';
import { getInitials } from '../lib/utils';
import { useTranslation } from '../lib/i18n';
import { useFriendStore } from '../stores/chatStore';

interface FriendRequestRowProps {
  request: { id: number; nickname: string; headId?: number; msg?: string };
}

export const FriendRequestRow = memo(function FriendRequestRow({ request }: FriendRequestRowProps) {
  const { t } = useTranslation();
  const respondToRequest = useFriendStore((s) => s.respondToRequest);

  return (
    <div className="mx-3 mb-2 rounded-xl border border-dark-highlight/20 bg-dark-highlight/10 p-3">
      <div className="mb-2 flex items-center gap-2">
        <div className="flex h-8 w-8 items-center justify-center rounded-full bg-dark-accent text-xs text-dark-text">
          {getInitials(request.nickname)}
        </div>
        <div className="min-w-0 flex-1">
          <div className="truncate text-xs font-semibold text-light-text dark:text-dark-text">
            {request.nickname}
          </div>
          <div className="truncate text-[10px] text-light-muted dark:text-dark-muted">
            {request.msg || t('contactList.friendRequestFallback', { id: request.id })}
          </div>
        </div>
      </div>
      <div className="grid grid-cols-2 gap-2">
        <button
          type="button"
          onClick={() => void respondToRequest(request.id, true)}
          className="rounded-md bg-dark-highlight px-2 py-1.5 text-[10px] font-semibold text-white"
        >
          {t('contactList.accept')}
        </button>
        <button
          type="button"
          onClick={() => void respondToRequest(request.id, false)}
          className="rounded-md border border-light-border px-2 py-1.5 text-[10px] font-semibold text-light-muted hover:border-red-400 hover:text-red-500 dark:border-dark-border dark:text-dark-muted"
        >
          {t('contactList.reject')}
        </button>
      </div>
    </div>
  );
});

export default FriendRequestRow;
