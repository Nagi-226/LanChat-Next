import { useEffect, useMemo, useState } from 'react';
import { motion } from 'framer-motion';
import { useUIStore } from '../stores/uiStore';
import { type SearchResult, useAuthStore, useMessageStore, useSearchStore } from '../stores/chatStore';
import { fmtTime } from '../lib/utils';
import { useAI } from '../lib/ai/useAI';
import { DeepSeekAPIProvider } from '../lib/ai/providers/DeepSeekAPI';
import { useTranslation } from '../lib/i18n';

type SearchScope = 'all' | 'direct' | 'group';
type Tab = 'search' | 'summary';

function highlight(text: string, query: string) {
  const q = query.trim();
  if (!q) return text;
  const idx = text.toLowerCase().indexOf(q.toLowerCase());
  if (idx < 0) return text;
  return (
    <>
      {text.slice(0, idx)}
      <mark className="rounded bg-dark-highlight/20 px-0.5 text-dark-highlight">{text.slice(idx, idx + q.length)}</mark>
      {text.slice(idx + q.length)}
    </>
  );
}

function SearchRow({ result, query, onJump }: { result: SearchResult; query: string; onJump: (result: SearchResult) => void }) {
  return (
    <motion.button
      type="button"
      layout
      onClick={() => onJump(result)}
      whileHover={{ x: 2 }}
      whileTap={{ scale: 0.99 }}
      className="w-full rounded-lg border border-light-border bg-white/70 p-3 text-left text-xs transition-colors hover:border-dark-highlight/50 dark:border-dark-border dark:bg-dark-accent/60"
    >
      <div className="mb-1 flex items-center justify-between gap-2">
        <span className="truncate font-semibold text-light-text dark:text-dark-text">{result.sourceName}</span>
        <span className="shrink-0 text-[10px] text-light-muted dark:text-dark-muted">{fmtTime(result.timestamp)}</span>
      </div>
      <p className="line-clamp-3 text-light-muted dark:text-dark-muted">{highlight(result.content, query)}</p>
    </motion.button>
  );
}

export function Sidebar() {
  const { t } = useTranslation();
  const toggleAIPanel = useUIStore((s) => s.toggleAIPanel);
  const { providers, providerId, setProviderId, model, setModel, apiKeyConfigured, saveApiKey, clearApiKey } = useAI();
  const contacts = useMessageStore((s) => s.contacts);
  const groups = useMessageStore((s) => s.groups);
  const currentUser = useAuthStore((s) => s.currentUser);
  const activeContactId = useMessageStore((s) => s.activeContactId);
  const activeGroupId = useMessageStore((s) => s.activeGroupId);
  const results = useSearchStore((s) => s.searchResults);
  const messagesByContact = useMessageStore((s) => s.messagesByContact);
  const messagesByGroup = useMessageStore((s) => s.messagesByGroup);
  const isSearching = useSearchStore((s) => s.isSearching);
  const searchMessages = useSearchStore((s) => s.searchMessages);
  const requestHistory = useMessageStore((s) => s.requestHistory);
  const selectContact = useMessageStore((s) => s.setActiveContact);
  const selectGroup = useMessageStore((s) => s.selectGroup);
  const summaryText = useSearchStore((s) => s.summaryText);
  const isSummarizing = useSearchStore((s) => s.isSummarizing);
  const summarizeConversation = useSearchStore((s) => s.summarizeConversation);

  const [tab, setTab] = useState<Tab>('search');
  const [query, setQuery] = useState('');
  const [scope, setScope] = useState<SearchScope>('all');
  const [apiKeyInput, setApiKeyInput] = useState('');
  const [summaryLocal, setSummaryLocal] = useState('');
  const [isSummarizingLocal, setIsSummarizingLocal] = useState(false);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void searchMessages(query, scope).catch(() => undefined);
    }, 300);
    return () => window.clearTimeout(timer);
  }, [query, scope, searchMessages]);

  const conversations = useMemo(() => [
    ...contacts.map((contact) => ({ type: 'direct' as const, id: contact.id, label: contact.nickname })),
    ...groups.map((group) => ({ type: 'group' as const, id: group.groupId, label: `# ${group.name}` })),
  ], [contacts, groups]);

  const activeConversation = activeGroupId
    ? { type: 'group' as const, id: activeGroupId }
    : activeContactId
      ? { type: 'direct' as const, id: activeContactId }
      : conversations[0];

  const jumpToResult = (result: SearchResult) => {
    let target: { type: 'direct' | 'group'; id: number } | null = null;
    let hasMessage = false;
    if (result.groupId) {
      target = { type: 'group', id: result.groupId };
      hasMessage = Boolean(messagesByGroup[result.groupId]?.some((message) => message.id === result.messageId));
      selectGroup(result.groupId);
    } else {
      const peerId = result.fromId === currentUser?.id ? result.toId : result.fromId;
      if (peerId) {
        target = { type: 'direct', id: peerId };
        hasMessage = Boolean(messagesByContact[peerId]?.some((message) => message.id === result.messageId));
        selectContact(peerId);
      }
    }
    if (target && !hasMessage) {
      void requestHistory({ ...target, limit: 100 }).catch(() => undefined);
    }
    window.setTimeout(() => {
      window.dispatchEvent(new CustomEvent('lanchat-scroll-message', { detail: { id: result.messageId } }));
    }, 80);
    window.setTimeout(() => {
      window.dispatchEvent(new CustomEvent('lanchat-scroll-message', { detail: { id: result.messageId } }));
    }, 420);
  };

  const tabLabel: Record<Tab, string> = {
    search: t('sidebar.search'),
    summary: t('sidebar.summary'),
  };

  const scopeLabel: Record<SearchScope, string> = {
    all: t('sidebar.scopeAll'),
    direct: t('sidebar.scopeDirect'),
    group: t('sidebar.scopeGroup'),
  };

  return (
    <aside className="flex h-full w-panel flex-shrink-0 flex-col border-l border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar">
      <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <div>
          <span className="text-xs font-semibold text-light-muted dark:text-dark-muted">{t('sidebar.aiAssistant')}</span>
          <p className="text-[10px] text-light-muted/80 dark:text-dark-muted/80">{t('sidebar.aiSubtitle')}</p>
        </div>
        <motion.button
          type="button"
          onClick={toggleAIPanel}
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
          className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:text-dark-highlight dark:text-dark-muted"
          aria-label={t('sidebar.closePanel')}
        >
          {t('sidebar.close')}
        </motion.button>
      </div>

      <div className="border-b border-light-border p-3 dark:border-dark-border">
        <div className="mb-3 grid grid-cols-2 gap-1 rounded-lg bg-light-bg p-1 dark:bg-dark-bg">
          {(['search', 'summary'] as const).map((item) => (
            <button
              key={item}
              type="button"
              onClick={() => setTab(item)}
              className={`rounded-md px-2 py-1 text-xs font-medium capitalize ${tab === item ? 'bg-dark-highlight text-white' : 'text-light-muted dark:text-dark-muted'}`}
            >
              {tabLabel[item]}
            </button>
          ))}
        </div>
        <label className="text-[10px] font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
          {t('sidebar.provider')}
          <select
            value={providerId}
            onChange={(event) => setProviderId(event.target.value)}
            className="mt-1 w-full rounded-lg border border-light-border bg-white px-2 py-1.5 text-xs normal-case text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text"
          >
            {providers.map((provider) => (
              <option key={provider.id} value={provider.id}>{provider.name}</option>
            ))}
          </select>
        </label>
        <div className="mt-3 grid grid-cols-[1fr_auto] gap-2">
          <label className="text-[10px] font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
            {t('sidebar.model')}
            <select
              value={model}
              onChange={(event) => setModel(event.target.value)}
              className="mt-1 w-full rounded-lg border border-light-border bg-white px-2 py-1.5 text-xs normal-case text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text"
            >
              <option value="deepseek-v4-pro">{t('sidebar.modelOption.v4pro')}</option>
              <option value="deepseek-chat">{t('sidebar.modelOption.v3')}</option>
              <option value="deepseek-reasoner">{t('sidebar.modelOption.r1')}</option>
              <option value="deepseek-flash">{t('sidebar.modelOption.flash')}</option>
            </select>
          </label>
          <div className="self-end rounded-lg border border-light-border px-2 py-1.5 text-[10px] text-light-muted dark:border-dark-border dark:text-dark-muted">
            Key {apiKeyConfigured ? t('sidebar.keySet') : t('sidebar.keyUnset')}
          </div>
        </div>
        <div className="mt-2 flex gap-2">
          <input
            value={apiKeyInput}
            onChange={(event) => setApiKeyInput(event.target.value)}
            type="password"
            placeholder={t('sidebar.apiKeyPlaceholder')}
            className="min-w-0 flex-1 rounded-lg border border-light-border bg-white px-2 py-1.5 text-xs text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text"
          />
          <button
            type="button"
            onClick={() => {
              void saveApiKey(apiKeyInput);
              setApiKeyInput('');
            }}
            className="rounded-lg bg-dark-highlight px-2 py-1.5 text-[10px] font-semibold text-white"
          >
            {t('sidebar.save')}
          </button>
          <button
            type="button"
            onClick={() => void clearApiKey()}
            className="rounded-lg border border-light-border px-2 py-1.5 text-[10px] font-semibold text-light-muted hover:text-red-500 dark:border-dark-border dark:text-dark-muted"
          >
            {t('sidebar.clear')}
          </button>
        </div>
      </div>

      {tab === 'search' ? (
        <div className="flex min-h-0 flex-1 flex-col p-3">
          <input
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            placeholder={t('sidebar.searchPlaceholder')}
            className="mb-2 rounded-lg border border-light-border bg-white px-3 py-2 text-xs text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text"
          />
          <div className="mb-3 flex gap-1">
            {(['all', 'direct', 'group'] as const).map((item) => (
              <button
                key={item}
                type="button"
                onClick={() => setScope(item)}
                className={`rounded-full px-2 py-1 text-[10px] font-medium ${scope === item ? 'bg-dark-highlight text-white' : 'bg-light-bg text-light-muted dark:bg-dark-bg dark:text-dark-muted'}`}
              >
                {scopeLabel[item]}
              </button>
            ))}
          </div>
          <div className="min-h-0 flex-1 space-y-2 overflow-y-auto">
            {isSearching && Array.from({ length: 3 }, (_, index) => (
              <div key={index} className="h-20 animate-pulse rounded-lg bg-light-border dark:bg-dark-border" />
            ))}
            {!isSearching && query.trim() && results.length === 0 && (
              <div className="rounded-lg border border-dashed border-light-border px-3 py-8 text-center text-xs text-light-muted dark:border-dark-border dark:text-dark-muted">
                {t('sidebar.noResults')}
              </div>
            )}
            {!query.trim() && (
              <div className="rounded-lg border border-dashed border-light-border px-3 py-8 text-center text-xs text-light-muted dark:border-dark-border dark:text-dark-muted">
                {t('sidebar.searchEmptyHint')}
              </div>
            )}
            {results.map((result) => <SearchRow key={result.id} result={result} query={query} onJump={jumpToResult} />)}
          </div>
        </div>
      ) : (
        <div className="flex min-h-0 flex-1 flex-col p-3">
          <select
            value={activeConversation ? `${activeConversation.type}:${activeConversation.id}` : ''}
            onChange={(event) => {
              const [type, rawId] = event.target.value.split(':') as ['direct' | 'group', string];
              if (type === 'group') selectGroup(Number(rawId));
              if (type === 'direct') selectContact(Number(rawId));
            }}
            className="mb-2 rounded-lg border border-light-border bg-white px-3 py-2 text-xs text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-accent dark:text-dark-text"
          >
            {conversations.map((conversation) => (
              <option key={`${conversation.type}:${conversation.id}`} value={`${conversation.type}:${conversation.id}`}>
                {conversation.label}
              </option>
            ))}
          </select>
          <motion.button
            type="button"
            onClick={async () => {
              if (!activeConversation) return;
              setIsSummarizingLocal(true);
              setSummaryLocal('');
              try {
                // gather messages from local store
                const store = useMessageStore.getState();
                const msgs =
                  activeConversation.type === 'group'
                    ? (store.messagesByGroup[activeConversation.id] ?? [])
                    : (store.messagesByContact[activeConversation.id] ?? []);
                if (msgs.length === 0) {
                  // try loading history first
                  await store.requestHistory({ ...activeConversation, limit: 100 });
                  // wait a tick for store update
                  await new Promise((r) => setTimeout(r, 500));
                  const updated =
                    activeConversation.type === 'group'
                      ? (useMessageStore.getState().messagesByGroup[activeConversation.id] ?? [])
                      : (useMessageStore.getState().messagesByContact[activeConversation.id] ?? []);
                  if (updated.length === 0) {
                    setSummaryLocal(t('sidebar.noConversationMessages'));
                    return;
                  }
                  const transcript = updated
                    .slice(-60)
                    .map((m) => `[${m.nickname ?? t('sidebar.fallbackName')}] (${new Date(m.timestamp).toLocaleString()}): ${m.content}`)
                    .join('\n');
                  const deepseek = new DeepSeekAPIProvider();
                  const result = await deepseek.execute({
                    type: 29 as const,
                    request_id: `summarize-${Date.now()}`,
                    ai_type: 'summarize',
                    msg: `Summarize the following conversation:\n\n${transcript}`,
                  });
                  setSummaryLocal(result?.msg ?? t('sidebar.noSummaryReturned'));
                  return;
                }
                const transcript = msgs
                  .slice(-60)
                  .map((m) => `[${m.nickname ?? t('sidebar.fallbackName')}] (${new Date(m.timestamp).toLocaleString()}): ${m.content}`)
                  .join('\n');
                const deepseek = new DeepSeekAPIProvider();
                const result = await deepseek.execute({
                  type: 29 as const,
                  request_id: `summarize-${Date.now()}`,
                  ai_type: 'summarize',
                  msg: `Summarize the following conversation:\n\n${transcript}`,
                });
                setSummaryLocal(result?.msg ?? t('sidebar.noSummaryReturned'));
              } catch (e) {
                setSummaryLocal(`Error: ${String(e)}`);
              } finally {
                setIsSummarizingLocal(false);
              }
            }}
            disabled={!activeConversation || isSummarizingLocal}
            whileHover={{ scale: isSummarizingLocal ? 1 : 1.02 }}
            whileTap={{ scale: isSummarizingLocal ? 1 : 0.98 }}
            className="mb-3 rounded-lg bg-dark-highlight px-3 py-2 text-xs font-semibold text-white disabled:cursor-not-allowed disabled:opacity-60"
          >
            {isSummarizingLocal ? t('sidebar.generatingSummary') : t('sidebar.generateSummary')}
          </motion.button>
          <div className="min-h-0 flex-1 overflow-y-auto rounded-lg border border-light-border bg-white/70 p-3 text-sm leading-relaxed text-light-text dark:border-dark-border dark:bg-dark-accent/60 dark:text-dark-text">
            {summaryLocal || t('sidebar.summaryDefault')}
          </div>
        </div>
      )}
    </aside>
  );
}

export default Sidebar;
