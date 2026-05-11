import type { Config } from 'tailwindcss';

export default {
  darkMode: 'class',
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    extend: {
      colors: {
        'dark-bg': '#1a1a2e',
        'dark-sidebar': '#16213e',
        'dark-accent': '#0f3460',
        'dark-highlight': '#e94560',
        'dark-hover': 'rgba(255,255,255,0.05)',
        'dark-text': '#e0e0e0',
        'dark-muted': '#a0a0b0',
        'dark-border': 'rgba(255,255,255,0.08)',
        'dark-bubble-self': '#0f3460',
        'dark-bubble-other': '#252545',
        'light-bg': '#ffffff',
        'light-sidebar': '#f0f2f5',
        'light-accent': '#1677ff',
        'light-online': '#52c41a',
        'light-text': '#1a1a1a',
        'light-muted': '#8c8c8c',
        'light-border': '#e5e7eb',
        'light-bubble-self': '#1677ff',
        'light-bubble-other': '#f0f2f5',
      },
      width: {
        sidebar: '240px',
        panel: '320px',
      },
      borderRadius: {
        bubble: '12px',
      },
      transitionDuration: {
        theme: '300ms',
      },
    },
  },
  plugins: [],
} satisfies Config;
