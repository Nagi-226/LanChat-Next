import type { TranslationKey, TranslationParams } from './types';
import { en } from './en';
import { zh } from './zh';
import { useUIStore } from '../../stores/uiStore';
import { useCallback } from 'react';

export type Language = 'en' | 'zh';
export const FALLBACK_LANGUAGE: Language = 'en';
export type { TranslationKey, TranslationParams } from './types';

const dictionaries: Record<Language, Record<TranslationKey, string>> = { en, zh };

// module-level cache for non-React callers (stores, utilities)
let cachedLanguage: Language = FALLBACK_LANGUAGE;

export function setLanguage(lang: Language): void {
  cachedLanguage = lang;
  document.documentElement.lang = lang === 'zh' ? 'zh-CN' : 'en';
}

export function getLanguage(): Language {
  return cachedLanguage;
}

function interpolate(template: string, params?: TranslationParams): string {
  if (!params) return template;
  return template.replace(/\{\{(\w+)\}\}/g, (_match, key: string) =>
    String(params[key] ?? `{{${key}}}`),
  );
}

/** standalone translation function — callable from React components AND Zustand stores */
export function t(key: TranslationKey, params?: TranslationParams): string {
  const dict = dictionaries[cachedLanguage] ?? dictionaries[FALLBACK_LANGUAGE];
  const template = dict[key];
  if (template === undefined) {
    console.warn(`[i18n] Missing key "${key}" for language "${cachedLanguage}", falling back to en`);
    const fallback = dictionaries[FALLBACK_LANGUAGE][key];
    return fallback !== undefined ? interpolate(fallback, params) : key;
  }
  return interpolate(template, params);
}

/** React hook — subscribes to uiStore.language for reactive re-renders */
export function useTranslation() {
  const language = useUIStore((s) => s.language);
  setLanguage(language);

  const translate = useCallback(
    (key: TranslationKey, params?: TranslationParams) => t(key, params),
    [language],
  );

  return { t: translate, language };
}
