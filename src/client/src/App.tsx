import { useEffect } from 'react';
import { listen } from '@tauri-apps/api/event';
import { useUIStore } from './stores/uiStore';
import { useChatStore } from './stores/chatStore';
import { useConnectionStore } from './stores/connectionStore';
import { ConnectionBar } from './components/ConnectionBar';
import { LoginPanel } from './components/LoginPanel';
import { RegisterPanel } from './components/RegisterPanel';
import { ContactList } from './components/ContactList';
import { ChatArea } from './components/ChatArea';
import { Sidebar } from './components/Sidebar';

function tryParseType(json: string): number | null {
  try {
    const msg = JSON.parse(json);
    return typeof msg.type === 'number' ? msg.type : null;
  } catch {
    return null;
  }
}

function HeaderActions() {
  const sidebarCollapsed = useUIStore((s) => s.sidebarCollapsed);
  const aiPanelOpen = useUIStore((s) => s.aiPanelOpen);
  const toggleSidebar = useUIStore((s) => s.toggleSidebar);
  const toggleAIPanel = useUIStore((s) => s.toggleAIPanel);
  const currentUser = useChatStore((s) => s.currentUser);
  const auth = useChatStore((s) => s.auth);
  const status = useConnectionStore((s) => s.status);

  const statusLabel = status === 'connected' ? 'Live' : status === 'connecting' ? 'Connecting' : 'Offline';

  return (
    <div className="ml-auto flex items-center gap-2">
      <span className="rounded-full border border-light-border bg-light-sidebar px-2.5 py-1 text-[10px] font-medium text-light-muted dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-muted">
        {statusLabel}
      </span>
      {currentUser && (
        <span className="hidden rounded-full bg-dark-highlight/10 px-2.5 py-1 text-[10px] font-medium text-dark-highlight sm:inline-flex">
          {auth.loading ? 'Syncing...' : currentUser.nickname}
        </span>
      )}
      <button
        type="button"
        onClick={toggleSidebar}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
      >
        {sidebarCollapsed ? 'Expand List' : 'Collapse List'}
      </button>
      <button
        type="button"
        onClick={toggleAIPanel}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
      >
        {aiPanelOpen ? 'Hide AI' : 'Show AI'}
      </button>
    </div>
  );
}

export function App() {
  const theme = useUIStore((s) => s.theme);
  const toggleTheme = useUIStore((s) => s.toggleTheme);
  const sidebarCollapsed = useUIStore((s) => s.sidebarCollapsed);
  const updateHeartbeat = useConnectionStore((s) => s.updateHeartbeat);
  const setDisconnected = useConnectionStore((s) => s.setDisconnected);
  const scheduleReconnect = useConnectionStore((s) => s.scheduleReconnect);
  const clearReconnect = useConnectionStore((s) => s.clearReconnect);

  const currentUser = useChatStore((s) => s.currentUser);
  const auth = useChatStore((s) => s.auth);
  const contacts = useChatStore((s) => s.contacts);
  const messagesByContact = useChatStore((s) => s.messagesByContact);
  const activeContactId = useChatStore((s) => s.activeContactId);

  useEffect(() => {
    document.documentElement.classList.toggle('dark', theme === 'dark');
  }, [theme]);

  useEffect(() => {
    const unlistenMsg = listen<string>('message-received', (event) => {
      const msgType = tryParseType(event.payload);
      if (msgType === 21) {
        updateHeartbeat();
        return;
      }
      try {
        const parsed = JSON.parse(event.payload);
        useChatStore.getState().handleIncomingMessage(parsed);
      } catch {
        // Ignore malformed messages until schema validation is wired into the UI.
      }
    });

    const unlistenLost = listen<string>('connection-lost', () => {
      setDisconnected();
      scheduleReconnect();
    });

    return () => {
      clearReconnect();
      unlistenMsg.then((fn) => fn());
      unlistenLost.then((fn) => fn());
    };
  }, [updateHeartbeat, setDisconnected, scheduleReconnect, clearReconnect]);

  const shellHeader = (
    <header
      className="flex h-10 items-center border-b border-light-border px-4 select-none dark:border-dark-border"
      data-tauri-drag-region
    >
      <span className="text-sm font-semibold">LanChat-Next</span>
      <span className="ml-3 hidden text-xs text-light-muted dark:text-dark-muted sm:inline">
        {currentUser ? `Welcome, ${currentUser.nickname}` : 'Secure LAN messaging workspace'}
      </span>
      <button
        type="button"
        onClick={toggleTheme}
        className="ml-3 rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
      >
        {theme === 'dark' ? 'Light' : 'Dark'}
      </button>
      <HeaderActions />
    </header>
  );

  if (!currentUser) {
    return (
      <div className="flex h-screen flex-col bg-light-bg text-light-text dark:bg-dark-bg dark:text-dark-text">
        {shellHeader}

        {auth.view === 'register' ? (
          <RegisterPanel
            onRegister={(nickname, password) => useChatStore.getState().register(nickname, password)}
            onSwitchToLogin={() => useChatStore.getState().setAuthView('login')}
            error={auth.error ?? undefined}
            loading={auth.loading}
          />
        ) : (
          <LoginPanel
            onLogin={(id, password) => useChatStore.getState().login(id, password)}
            onSwitchToRegister={() => useChatStore.getState().setAuthView('register')}
            error={auth.error ?? undefined}
            loading={auth.loading}
          />
        )}

        <footer className="flex h-8 items-center border-t border-light-border px-4 text-xs dark:border-dark-border">
          <ConnectionBar />
        </footer>
      </div>
    );
  }

  const activeMessages = activeContactId ? messagesByContact[activeContactId] || [] : [];
  const activeContact = contacts.find((c) => c.id === activeContactId);

  return (
    <div className="flex h-screen flex-col bg-light-bg text-light-text dark:bg-dark-bg dark:text-dark-text">
      {shellHeader}

      <div className="flex min-h-0 flex-1 overflow-hidden">
        {!sidebarCollapsed && (
          <ContactList
            contacts={contacts}
            selectedId={activeContactId}
            onSelect={(id) => {
              useChatStore.getState().setActiveContact(id);
              useChatStore.setState((s) => ({
                contacts: s.contacts.map((c) => (c.id === id ? { ...c, unread: 0 } : c)),
              }));
            }}
            onLogout={() => {
              useChatStore.getState().logout();
              useConnectionStore.getState().disconnect();
            }}
          />
        )}

        {activeContact ? (
          <ChatArea
            messages={activeMessages}
            currentUserId={currentUser.id}
            contactName={activeContact.nickname}
            onSend={(content) => useChatStore.getState().sendPrivateMessage(activeContact.id, content)}
          />
        ) : (
          <main className="flex flex-1 items-center justify-center">
            <p className="text-sm text-light-muted dark:text-dark-muted">
              Select a conversation to start chatting
            </p>
          </main>
        )}

        <Sidebar />
      </div>

      <footer className="flex h-8 items-center border-t border-light-border px-4 text-xs dark:border-dark-border">
        <ConnectionBar />
      </footer>
    </div>
  );
}

export default App;
