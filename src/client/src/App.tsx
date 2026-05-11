import { useEffect } from 'react';
import { useUIStore } from './stores/uiStore';

function App() {
  const theme = useUIStore((state) => state.theme);
  const toggleTheme = useUIStore((state) => state.toggleTheme);

  useEffect(() => {
    document.documentElement.classList.toggle('dark', theme === 'dark');
  }, [theme]);

  return (
    <div className="flex h-screen flex-col bg-light-bg text-light-text dark:bg-dark-bg dark:text-dark-text">
      <header
        className="flex h-10 items-center border-b border-light-border px-4 select-none dark:border-dark-border"
        data-tauri-drag-region
      >
        <span className="text-sm font-semibold">LanChat-Next</span>
        <button
          type="button"
          onClick={toggleTheme}
          className="ml-auto rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        >
          {theme === 'dark' ? '☀ 浅色' : '🌙 暗色'}
        </button>
      </header>

      <div className="flex flex-1 overflow-hidden">
        <aside className="w-sidebar flex-shrink-0 border-r border-light-border bg-light-sidebar p-4 dark:border-dark-border dark:bg-dark-sidebar">
          <h2 className="mb-3 text-sm font-semibold uppercase tracking-wider text-light-muted dark:text-dark-muted">
            频道 / 联系人
          </h2>
          <div className="text-xs text-light-muted dark:text-dark-muted">占位 — 频道列表</div>
        </aside>

        <main className="flex flex-1 items-center justify-center">
          <p className="text-sm text-light-muted dark:text-dark-muted">选择一个对话开始聊天</p>
        </main>

        <aside className="hidden w-panel flex-shrink-0 border-l border-light-border bg-light-sidebar p-4 dark:border-dark-border dark:bg-dark-sidebar">
          {/* v1.7.0 AI 面板在此 */}
        </aside>
      </div>

      <footer className="flex h-8 items-center border-t border-light-border px-4 text-xs text-light-muted dark:border-dark-border dark:text-dark-muted">
        🟢 在线 | 等待连接
      </footer>
    </div>
  );
}

export default App;
