import { useCallback, useEffect, useMemo, useState } from 'react';
import type { AIService } from './types';
import { DeepSeekAPIProvider } from './providers/DeepSeekAPI';
import { LocalSearchProvider } from './providers/LocalSearch';
import { secureStorage } from '../secureStorage';

export function useAI() {
  const providers = useMemo<AIService[]>(() => [new LocalSearchProvider(), new DeepSeekAPIProvider()], []);
  const [providerId, setProviderId] = useState(() => window.localStorage.getItem('lanchat-ai-provider') || providers[0].id);
  const [model, setModelState] = useState(() => window.localStorage.getItem('lanchat-ai-model') || 'deepseek-chat');
  const [apiKeyConfigured, setApiKeyConfigured] = useState(false);
  const provider = providers.find((item) => item.id === providerId) ?? providers[0];

  useEffect(() => {
    window.localStorage.setItem('lanchat-ai-provider', provider.id);
    let cancelled = false;
    void secureStorage.getApiKey(provider.id).then((value) => {
      if (!cancelled) setApiKeyConfigured(Boolean(value));
    }).catch(() => {
      if (!cancelled) setApiKeyConfigured(false);
    });
    return () => {
      cancelled = true;
    };
  }, [provider.id]);

  const setModel = useCallback((value: string) => {
    setModelState(value);
    window.localStorage.setItem('lanchat-ai-model', value);
  }, []);

  const saveApiKey = useCallback(async (value: string) => {
    const trimmed = value.trim();
    if (!trimmed) return;
    await secureStorage.setApiKey(provider.id, trimmed);
    setApiKeyConfigured(true);
  }, [provider.id]);

  const clearApiKey = useCallback(async () => {
    await secureStorage.deleteApiKey(provider.id);
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
