import { useTranslation } from '../lib/i18n';

export function ContactSkeleton() {
  const { t } = useTranslation();
  return (
    <div className="space-y-2 px-4 py-3" aria-label={t('contactList.loadingContacts')}>
      {Array.from({ length: 5 }, (_, index) => (
        <div key={index} className="flex items-center gap-3">
          <div className="h-10 w-10 animate-pulse rounded-full bg-light-border dark:bg-dark-border" />
          <div className="min-w-0 flex-1 space-y-2">
            <div className="h-3 w-2/3 animate-pulse rounded bg-light-border dark:bg-dark-border" />
            <div className="h-2 w-1/2 animate-pulse rounded bg-light-border dark:bg-dark-border" />
          </div>
        </div>
      ))}
    </div>
  );
}

export default ContactSkeleton;
