import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import type { ReactNode } from 'react';
import { listen } from '@tauri-apps/api/event';
import { AnimatePresence, motion, MotionConfig } from 'framer-motion';
import { useUIStore } from './stores/uiStore';
import { useChatStore } from './stores/chatStore';
import { useConnectionStore } from './stores/connectionStore';
import { ConnectionBar } from './components/ConnectionBar';
import { LoginPanel } from './components/LoginPanel';
import { RegisterPanel } from './components/RegisterPanel';
import { ContactList } from './components/ContactList';
import { ChatArea } from './components/ChatArea';
import { GroupChatArea, type Member } from './components/GroupChatArea';
import { Sidebar } from './components/Sidebar';
import { ToastContainer, type ToastItem, type ToastType } from './lib/Toast';
import GradientText from './lib/GradientText';
import AnimatedContent from './lib/AnimatedContent';

const themeBg = {
  light: '#ffffff',
  dark: '#1a1a2e',
};

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
  const friendRequests = useChatStore((s) => s.friendRequests);
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
      {friendRequests.length > 0 && (
        <span className="rounded-full bg-dark-highlight px-2.5 py-1 text-[10px] font-bold text-white">
          {friendRequests.length} pending
        </span>
      )}
      <motion.button
        type="button"
        onClick={toggleSidebar}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={sidebarCollapsed ? 'Expand contact list' : 'Collapse contact list'}
      >
        {sidebarCollapsed ? 'Expand List' : 'Collapse List'}
      </motion.button>
      <motion.button
        type="button"
        onClick={toggleAIPanel}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={aiPanelOpen ? 'Hide AI panel' : 'Show AI panel'}
      >
        {aiPanelOpen ? 'Hide AI' : 'Show AI'}
      </motion.button>
    </div>
  );
}

function ReconnectBanner() {
  const status = useConnectionStore((s) => s.status);
  const retryDueAt = useConnectionStore((s) => s.retryDueAt);
  const connect = useConnectionStore((s) => s.connect);
  const host = useConnectionStore((s) => s.host);
  const port = useConnectionStore((s) => s.port);
  const clearReconnect = useConnectionStore((s) => s.clearReconnect);
  const [dismissed, setDismissed] = useState(false);
  const [secondsLeft, setSecondsLeft] = useState(0);

  useEffect(() => {
    if (!retryDueAt) {
      setDismissed(false);
      setSecondsLeft(0);
      return undefined;
    }

    const tick = () => setSecondsLeft(Math.max(0, Math.ceil((retryDueAt - Date.now()) / 1000)));
    tick();
    const timer = window.setInterval(tick, 500);
    return () => window.clearInterval(timer);
  }, [retryDueAt]);

  const visible = status === 'connecting' && Boolean(retryDueAt) && !dismissed;

  return (
    <AnimatePresence initial={false}>
      {visible && (
        <motion.div
          initial={{ height: 0, opacity: 0, y: -12 }}
          animate={{ height: 'auto', opacity: 1, y: 0 }}
          exit={{ height: 0, opacity: 0, y: -12 }}
          transition={{ duration: 0.22 }}
          className="overflow-hidden border-b border-amber-300/40 bg-amber-400/15 text-xs text-amber-800 dark:text-amber-200"
        >
          <div className="flex items-center justify-center gap-3 px-4 py-2">
            <span>Connection lost. Reconnecting in {secondsLeft}s...</span>
            <motion.button
              type="button"
              onClick={() => connect(host, port)}
              whileHover={{ scale: 1.04 }}
              whileTap={{ scale: 0.96 }}
              className="rounded bg-amber-500/20 px-2 py-0.5 font-medium hover:bg-amber-500/30"
            >
              Retry now
            </motion.button>
            <button
              type="button"
              onClick={() => {
                setDismissed(true);
                clearReconnect();
              }}
              className="rounded px-2 py-0.5 hover:bg-amber-500/20"
              aria-label="Dismiss reconnect banner"
            >
              x
            </button>
          </div>
        </motion.div>
      )}
    </AnimatePresence>
  );
}

function AppSplash() {
  return (
    <motion.div
      className="fixed inset-0 z-[60] flex flex-col items-center justify-center bg-light-bg dark:bg-dark-bg"
      initial={{ opacity: 1 }}
      exit={{ opacity: 0 }}
      transition={{ duration: 0.35 }}
      aria-label="Loading LanChat-Next"
    >
      <GradientText className="text-3xl font-black tracking-tight" animationSpeed={5}>
        LanChat-Next
      </GradientText>
      <motion.div
        className="mt-5 h-1 w-36 overflow-hidden rounded-full bg-light-border dark:bg-dark-border"
        initial={{ opacity: 0, y: 6 }}
        animate={{ opacity: 1, y: 0 }}
      >
        <motion.div
          className="h-full w-1/3 rounded-full bg-dark-highlight"
          animate={{ x: ['-100%', '320%'] }}
          transition={{ duration: 1.1, repeat: Infinity, ease: 'easeInOut' }}
        />
      </motion.div>
    </motion.div>
  );
}

function ShellFade({ showSplash, children }: { showSplash: boolean; children: ReactNode }) {
  return (
    <motion.div
      className="flex min-h-0 flex-1 flex-col"
      initial={{ opacity: 0 }}
      animate={{ opacity: showSplash ? 0 : 1 }}
      transition={{ duration: 0.24, ease: 'easeOut' }}
    >
      {children}
    </motion.div>
  );
}

function memberStatus(memberId: number, currentUserId: number, contacts: Array<{ id: number; status: Member['status'] }>): Member['status'] {
  if (memberId === currentUserId) return 'online';
  return contacts.find((contact) => contact.id === memberId)?.status ?? 'offline';
}

export function App() {
  const theme = useUIStore((s) => s.theme);
  const toggleTheme = useUIStore((s) => s.toggleTheme);
  const sidebarCollapsed = useUIStore((s) => s.sidebarCollapsed);
  const aiPanelOpen = useUIStore((s) => s.aiPanelOpen);
  const updateHeartbeat = useConnectionStore((s) => s.updateHeartbeat);
  const setDisconnected = useConnectionStore((s) => s.setDisconnected);
  const scheduleReconnect = useConnectionStore((s) => s.scheduleReconnect);
  const clearReconnect = useConnectionStore((s) => s.clearReconnect);
  const connectionError = useConnectionStore((s) => s.error);
  const connectionStatus = useConnectionStore((s) => s.status);

  const currentUser = useChatStore((s) => s.currentUser);
  const auth = useChatStore((s) => s.auth);
  const contacts = useChatStore((s) => s.contacts);
  const groups = useChatStore((s) => s.groups);
  const messagesByContact = useChatStore((s) => s.messagesByContact);
  const messagesByGroup = useChatStore((s) => s.messagesByGroup);
  const activeContactId = useChatStore((s) => s.activeContactId);
  const activeGroupId = useChatStore((s) => s.activeGroupId);
  const friendRequests = useChatStore((s) => s.friendRequests);
  const friends = useChatStore((s) => s.friends);

  const [toasts, setToasts] = useState<ToastItem[]>([]);
  const [showSplash, setShowSplash] = useState(true);
  const lastConnectionError = useRef<string | null>(null);
  const lastAuthError = useRef<string | null>(null);
  const lastFriendRequestCount = useRef(0);

  const addToast = useCallback((type: ToastType, message: string) => {
    setToasts((items) => [...items, { id: `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`, type, message }].slice(-4));
  }, []);

  const dismissToast = useCallback((id: string) => {
    setToasts((items) => items.filter((item) => item.id !== id));
  }, []);

  const autoLoginAttempted = useRef(false);

  useEffect(() => {
    const timer = window.setTimeout(() => setShowSplash(false), 700);
    return () => window.clearTimeout(timer);
  }, []);

  useEffect(() => {
    document.documentElement.classList.toggle('dark', theme === 'dark');
  }, [theme]);

  useEffect(() => {
    if (autoLoginAttempted.current) return;
    const savedId = window.localStorage.getItem('lanchat-last-user-id');
    const savedPassword = window.localStorage.getItem('lanchat-last-password');
    if (!savedId || !savedPassword) return;

    autoLoginAttempted.current = true;
    const conn = useConnectionStore.getState();
    if (conn.status !== 'disconnected') return;

    conn.connect(conn.host, conn.port).then(() => {
      useChatStore.getState().login(Number(savedId), savedPassword);
    }).catch(() => {
      // Connection failed — user handles manually via ConnectionBar
    });
  }, []);

  useEffect(() => {
    if (connectionError && connectionError !== lastConnectionError.current) {
      lastConnectionError.current = connectionError;
      addToast('error', connectionError);
    }
    if (!connectionError) lastConnectionError.current = null;
  }, [addToast, connectionError]);

  useEffect(() => {
    if (auth.error && auth.error !== lastAuthError.current) {
      lastAuthError.current = auth.error;
      addToast('error', auth.error);
    }
    if (!auth.error) lastAuthError.current = null;
  }, [addToast, auth.error]);

  useEffect(() => {
    if (friendRequests.length > lastFriendRequestCount.current) {
      addToast('info', 'New friend request received.');
    }
    lastFriendRequestCount.current = friendRequests.length;
  }, [addToast, friendRequests.length]);

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
      addToast('info', 'Connection lost. Reconnect scheduled.');
    });

    return () => {
      clearReconnect();
      unlistenMsg.then((fn) => fn());
      unlistenLost.then((fn) => fn());
    };
  }, [addToast, updateHeartbeat, setDisconnected, scheduleReconnect, clearReconnect]);

  const shellHeader = (
    <header
      className="flex h-10 items-center border-b border-light-border px-4 select-none dark:border-dark-border"
      data-tauri-drag-region
    >
      <GradientText className="text-sm font-bold" animationSpeed={7}>
        LanChat-Next
      </GradientText>
      <span className="ml-3 hidden text-xs text-light-muted dark:text-dark-muted sm:inline">
        {currentUser ? `Welcome, ${currentUser.nickname}` : 'Secure LAN messaging workspace'}
      </span>
      <motion.button
        type="button"
        onClick={toggleTheme}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="ml-3 rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={`Switch to ${theme === 'dark' ? 'light' : 'dark'} theme`}
      >
        {theme === 'dark' ? 'Light' : 'Dark'}
      </motion.button>
      <HeaderActions />
    </header>
  );

  const activeMessages = activeContactId ? messagesByContact[activeContactId] || [] : [];
  const activeGroupMessages = activeGroupId ? messagesByGroup[activeGroupId] || [] : [];
  const activeContact = contacts.find((c) => c.id === activeContactId);
  const activeGroup = groups.find((g) => g.groupId === activeGroupId);
  const connected = connectionStatus === 'connected';
  const activeGroupMembers = useMemo<Member[]>(() => {
    if (!currentUser || !activeGroup) return [];
    const users = activeGroup.users || [];
    const hasCurrentUser = users.some((user) => user.id === currentUser.id);
    const fullUsers = hasCurrentUser ? users : [{ id: currentUser.id, nickname: currentUser.nickname, headId: currentUser.headId }, ...users];
    return fullUsers.map((user) => ({
      id: user.id,
      nickname: user.nickname,
      status: memberStatus(user.id, currentUser.id, contacts),
    }));
  }, [activeGroup, contacts, currentUser]);

  if (!currentUser) {
    return (
      <MotionConfig reducedMotion="user">
        <motion.div
          animate={{ backgroundColor: themeBg[theme] }}
          transition={{ duration: 0.35 }}
          className="flex h-screen flex-col text-light-text dark:text-dark-text"
        >
          <AnimatePresence>{showSplash && <AppSplash />}</AnimatePresence>
          <ShellFade showSplash={showSplash}>
            {shellHeader}
            <ReconnectBanner />
            <ToastContainer toasts={toasts} onDismiss={dismissToast} />

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

            <footer className="flex min-h-8 items-center border-t border-light-border px-4 py-1 text-xs dark:border-dark-border">
              <ConnectionBar />
            </footer>
          </ShellFade>
        </motion.div>
      </MotionConfig>
    );
  }

  return (
    <MotionConfig reducedMotion="user">
      <motion.div
        animate={{ backgroundColor: themeBg[theme] }}
        transition={{ duration: 0.35 }}
        className="flex h-screen flex-col text-light-text dark:text-dark-text"
      >
        <AnimatePresence>{showSplash && <AppSplash />}</AnimatePresence>
        <ShellFade showSplash={showSplash}>
          {shellHeader}
          <ReconnectBanner />
          <ToastContainer toasts={toasts} onDismiss={dismissToast} />

          <div className="flex min-h-0 flex-1 overflow-hidden">
            <AnimatePresence initial={false}>
              {!sidebarCollapsed && (
                <motion.div
                  key="contact-list"
                  initial={{ width: 0, opacity: 0, x: -24 }}
                  animate={{ width: 240, opacity: 1, x: 0 }}
                  exit={{ width: 0, opacity: 0, x: -24 }}
                  transition={{ duration: 0.24, ease: 'easeOut' }}
                  className="overflow-hidden"
                >
                  <ContactList
                    contacts={contacts}
                    groups={groups}
                    selectedId={activeContactId}
                    selectedGroupId={activeGroupId}
                    loading={auth.loading}
                    onSelect={(id) => useChatStore.getState().setActiveContact(id)}
                    onSelectGroup={(id) => useChatStore.getState().selectGroup(id)}
                    onLogout={() => {
                      useChatStore.getState().logout();
                      useConnectionStore.getState().disconnect();
                    }}
                  />
                </motion.div>
              )}
            </AnimatePresence>

            <AnimatedContent
              key={activeGroup ? `group-${activeGroup.groupId}` : activeContact ? `contact-${activeContact.id}` : 'empty-chat'}
              className="flex min-w-0 flex-1"
              direction="horizontal"
              distance={18}
              threshold={0}
            >
              {activeGroup ? (
                <GroupChatArea
                  messages={activeGroupMessages}
                  members={activeGroupMembers}
                  currentUserId={currentUser.id}
                  groupName={activeGroup.name}
                  connected={connected}
                  onSend={(content) => {
                    void useChatStore.getState().sendGroupMessage(activeGroup.groupId, content).catch((e) => addToast('error', String(e)));
                  }}
                  onRetryFailed={(message) => {
                    void useChatStore.getState().sendGroupMessage(activeGroup.groupId, message.content).catch((e) => addToast('error', String(e)));
                  }}
                  friendIds={friends.map((friend) => friend.id)}
                  onAddFriend={(memberId) => {
                    void useChatStore.getState().sendFriendRequest(memberId, 'Hi from the group chat.').catch((e) => addToast('error', String(e)));
                  }}
                />
              ) : activeContact ? (
                <ChatArea
                  messages={activeMessages}
                  currentUserId={currentUser.id}
                  contactName={activeContact.nickname}
                  connected={connected}
                  onSend={(content) => {
                    void useChatStore.getState().sendPrivateMessage(activeContact.id, content).catch((e) => addToast('error', String(e)));
                  }}
                  onRetryFailed={(message) => {
                    void useChatStore.getState().sendPrivateMessage(activeContact.id, message.content).catch((e) => addToast('error', String(e)));
                  }}
                />
              ) : (
                <main className="flex flex-1 items-center justify-center px-6 text-center">
                  <p className="text-sm text-light-muted dark:text-dark-muted">
                    Select a contact or group to start chatting
                  </p>
                </main>
              )}
            </AnimatedContent>

            <AnimatePresence initial={false}>
              {aiPanelOpen && (
                <motion.div
                  key="ai-panel"
                  initial={{ width: 0, opacity: 0, x: 28 }}
                  animate={{ width: 320, opacity: 1, x: 0 }}
                  exit={{ width: 0, opacity: 0, x: 28 }}
                  transition={{ duration: 0.24, ease: 'easeOut' }}
                  className="overflow-hidden"
                >
                  <Sidebar />
                </motion.div>
              )}
            </AnimatePresence>
          </div>

          <footer className="flex min-h-8 items-center border-t border-light-border px-4 py-1 text-xs dark:border-dark-border">
            <ConnectionBar />
          </footer>
        </ShellFade>
      </motion.div>
    </MotionConfig>
  );
}

export default App;
