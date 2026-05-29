import { useCallback, useEffect, useMemo, useState } from 'react';
import type { AIService } from './types';
import { ClaudeAPIProvider } from './providers/ClaudeAPI';
import { LocalSearchProvider } from './providers/LocalSearch';

export function useAI() {
  const providers = useMemo<AIService[]>(() => [new LocalSearchProvider(), new ClaudeAPIProvider()], []);
  const [providerId, setProviderId] = useState(() => window.localStorage.getItem('lanchat-ai-provider') || providers[0].id);
  const [model, setModelState] = useState(() => window.localStorage.getItem('lanchat-ai-model') || 'claude-3-5-sonnet');
  const [apiKeyConfigured, setApiKeyConfigured] = useState(false);
  const provider = providers.find((item) => item.id === providerId) ?? providers[0];

  useEffect(() => {
    window.localStorage.setItem('lanchat-ai-provider', provider.id);
    setApiKeyConfigured(Boolean(window.localStorage.getItem(`lanchat-ai-key:${provider.id}`)));
  }, [provider.id]);

  const setModel = useCallback((value: string) => {
    setModelState(value);
    window.localStorage.setItem('lanchat-ai-model', value);
  }, []);

  const saveApiKey = useCallback((value: string) => {
    const trimmed = value.trim();
    if (!trimmed) return;
    window.localStorage.setItem(`lanchat-ai-key:${provider.id}`, window.btoa(trimmed));
    setApiKeyConfigured(true);
  }, [provider.id]);

  const clearApiKey = useCallback(() => {
    window.localStorage.removeItem(`lanchat-ai-key:${provider.id}`);
    setApiKeyConfigured(false);
  }, [provider.id]);

  return {
    providers,
    provider,
    providerId,
    setProviderId,
    model,
    setModel,
    apiKeyConfigured,
    saveApiKey,
    clearApiKey,
  };
}
