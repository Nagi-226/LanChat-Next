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
        <div>
          <span className="text-xs font-semibold text-light-muted dark:text-dark-muted">
            AI Assistant
          </span>
          <p className="text-[10px] text-light-muted/80 dark:text-dark-muted/80">
            Reserved for v1.7.0
          </p>
        </div>
        <button
          type="button"
          onClick={toggleAIPanel}
          className="rounded px-2 py-0.5 text-[10px] text-light-muted transition-colors hover:text-dark-highlight dark:text-dark-muted"
        >
          Close
        </button>
      </div>

      <div className="flex flex-1 items-center justify-center p-4">
        <div className="max-w-[180px] text-center">
          <p className="text-sm text-light-muted dark:text-dark-muted">
            AI features are scheduled for v1.7.0.
          </p>
        </div>
      </div>
    </aside>
  );
}

export default Sidebar;
