import { useUIStore } from '../stores/uiStore';

export function Sidebar() {
  const aiPanelOpen = useUIStore((s) => s.aiPanelOpen);
  const toggleAIPanel = useUIStore((s) => s.toggleAIPanel);

  return (
    <aside
      className={`w-panel flex-shrink-0 border-l border-light-border bg-light-sidebar dark:border-dark-border dark:bg-dark-sidebar ${
        aiPanelOpen ? 'flex flex-col' : 'hidden'
      }`}
    >
      <div className="flex h-12 items-center justify-between border-b border-light-border px-4 dark:border-dark-border">
        <span className="text-xs font-semibold text-light-muted dark:text-dark-muted">
          AI Assistant
        </span>
        <button
          type="button"
          onClick={toggleAIPanel}
          className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:text-dark-highlight dark:text-dark-muted"
        >
          Close
        </button>
      </div>

      <div className="flex flex-1 items-center justify-center p-4">
        <div className="text-center">
          <p className="text-sm text-light-muted dark:text-dark-muted">
            AI features are scheduled for v1.7.0
          </p>
        </div>
      </div>
    </aside>
  );
}

export default Sidebar;
