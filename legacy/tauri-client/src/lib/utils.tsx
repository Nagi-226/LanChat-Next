import { memo } from 'react';
import { getLanguage, t } from './i18n';

function localeTag(): string {
  return getLanguage() === 'zh' ? 'zh-CN' : 'en-US';
}

export function fmtTime(ts: number): string {
  return new Date(ts).toLocaleTimeString(localeTag(), { hour: '2-digit', minute: '2-digit' });
}

export function getInitials(name: string): string {
  const trimmed = name.trim();
  if (!trimmed) return '?';
  const parts = trimmed.split(/\s+/).slice(0, 2);
  return parts.map((part) => part.charAt(0)).join('').toUpperCase();
}

export function dateKey(ts: number): string {
  const d = new Date(ts);
  return `${d.getFullYear()}-${d.getMonth() + 1}-${d.getDate()}`;
}

export function fmtDateDivider(ts: number): string {
  const d = new Date(ts);
  const today = new Date();
  const yesterday = new Date();
  yesterday.setDate(today.getDate() - 1);

  if (dateKey(d.getTime()) === dateKey(today.getTime())) return t('date.today');
  if (dateKey(d.getTime()) === dateKey(yesterday.getTime())) return t('date.yesterday');

  const loc = localeTag();
  return d.toLocaleDateString(loc, {
    month: 'short',
    day: 'numeric',
    year: d.getFullYear() === today.getFullYear() ? undefined : 'numeric',
  });
}

export const DateDivider = memo(function DateDivider({ timestamp }: { timestamp: number }) {
  return (
    <div className="my-4 flex items-center gap-3" aria-label={fmtDateDivider(timestamp)}>
      <div className="h-px flex-1 bg-light-border dark:bg-dark-border" />
      <span className="rounded-full bg-light-sidebar px-2.5 py-1 text-[10px] font-medium uppercase tracking-wide text-light-muted dark:bg-dark-sidebar dark:text-dark-muted">
        {fmtDateDivider(timestamp)}
      </span>
      <div className="h-px flex-1 bg-light-border dark:bg-dark-border" />
    </div>
  );
});

export function ButtonSpinner(): JSX.Element {
  return <span className="mr-2 inline-block h-3 w-3 animate-spin rounded-full border-2 border-white/40 border-t-white" aria-hidden="true" />;
}

export function MessageSkeleton() {
  return (
    <div className="space-y-4 px-1 py-3" aria-label={t('date.loadingMessages')}>
      {Array.from({ length: 3 }, (_, index) => (
        <div key={index} className={`flex gap-2 ${index % 2 === 0 ? 'flex-row' : 'flex-row-reverse'}`}>
          <div className="h-9 w-9 animate-pulse rounded-full bg-light-border dark:bg-dark-border" />
          <div className="max-w-[70%] space-y-2">
            <div className="h-3 w-20 animate-pulse rounded bg-light-border dark:bg-dark-border" />
            <div className="h-10 w-48 animate-pulse rounded-bubble bg-light-border dark:bg-dark-border" />
          </div>
        </div>
      ))}
    </div>
  );
}
