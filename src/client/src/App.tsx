import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { listen } from '@tauri-apps/api/event';
import { AnimatePresence, motion, MotionConfig } from 'framer-motion';
import { useUIStore } from './stores/uiStore';
import { useAuthStore, useMessageStore, useFriendStore, useSearchStore } from './stores/chatStore';
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
import { AppSplash } from './components/AppSplash';
import { ShellFade } from './components/ShellFade';
import { HeaderActions } from './components/HeaderActions';
import { ReconnectBanner } from './components/ReconnectBanner';
import AnimatedContent from './lib/AnimatedContent';
import { useTranslation, t } from './lib/i18n';
import { notifyNative } from './lib/nativeNotifications';
import { themeBg } from './lib/theme';
import { tryParseType } from './lib/tryParseType';

function memberStatus(memberId: number, currentUserId: number, contacts: Array<{ id: number; status: Member['status'] }>): Member['status'] {
  if (memberId === currentUserId) return 'online';
  return contacts.find((contact) => contact.id === memberId)?.status ?? 'offline';
}

export function App() {
  const theme = useUIStore((s) => s.theme);
  const toggleTheme = useUIStore((s) => s.toggleTheme);
  const language = useUIStore((s) => s.language);
  const setLanguage = useUIStore((s) => s.setLanguage);
  const sidebarCollapsed = useUIStore((s) => s.sidebarCollapsed);
  const aiPanelOpen = useUIStore((s) => s.aiPanelOpen);
  useTranslation(); // sync language cache on mount/change
  const updateHeartbeat = useConnectionStore((s) => s.updateHeartbeat);
  const setDisconnected = useConnectionStore((s) => s.setDisconnected);
  const scheduleReconnect = useConnectionStore((s) => s.scheduleReconnect);
  const clearReconnect = useConnectionStore((s) => s.clearReconnect);
  const connectionError = useConnectionStore((s) => s.error);
  const connectionStatus = useConnectionStore((s) => s.status);

  const currentUser = useAuthStore((s) => s.currentUser);
  const auth = useAuthStore((s) => s.auth);
  const contacts = useMessageStore((s) => s.contacts);
  const groups = useMessageStore((s) => s.groups);
  const messagesByContact = useMessageStore((s) => s.messagesByContact);
  const messagesByGroup = useMessageStore((s) => s.messagesByGroup);
  const activeContactId = useMessageStore((s) => s.activeContactId);
  const activeGroupId = useMessageStore((s) => s.activeGroupId);
  const friendRequests = useFriendStore((s) => s.friendRequests);
  const friends = useFriendStore((s) => s.friends);
  const offlineQueue = useMessageStore((s) => s.offlineQueue);

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
      useAuthStore.getState().login(Number(savedId), savedPassword);
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
      addToast('info', t('app.toast.friendRequest'));
    }
    lastFriendRequestCount.current = friendRequests.length;
  }, [addToast, friendRequests.length]);

  const activeMessages = activeContactId ? messagesByContact[activeContactId] || [] : [];
  const activeGroupMessages = activeGroupId ? messagesByGroup[activeGroupId] || [] : [];
  const activeContact = contacts.find((c) => c.id === activeContactId);
  const activeGroup = groups.find((g) => g.groupId === activeGroupId);
  const connected = connectionStatus === 'connected';

  useEffect(() => {
    if (connectionStatus === 'connected') {
      void useMessageStore.getState().flushOfflineQueue();
      void useMessageStore.getState().negotiateProtocol();
    }
  }, [connectionStatus]);

  useEffect(() => {
    if (!currentUser || connectionStatus !== 'connected') return;
    const latestIncoming = [...activeMessages].reverse().find((message) => message.fromId !== currentUser.id);
    if (activeContactId && latestIncoming) {
      void useMessageStore.getState().markMessageRead({ type: 'direct', id: activeContactId }, latestIncoming.id);
    }
  }, [activeContactId, activeMessages, connectionStatus, currentUser]);

  useEffect(() => {
    if (!currentUser || connectionStatus !== 'connected') return;
    const latestIncoming = [...activeGroupMessages].reverse().find((message) => message.fromId !== currentUser.id);
    if (activeGroupId && latestIncoming) {
      void useMessageStore.getState().markMessageRead({ type: 'group', id: activeGroupId }, latestIncoming.id);
    }
  }, [activeGroupId, activeGroupMessages, connectionStatus, currentUser]);

  useEffect(() => {
    const unlistenMsg = listen<string>('message-received', (event) => {
      const msgType = tryParseType(event.payload);
      if (msgType === 21) {
        updateHeartbeat();
        return;
      }
      try {
        const parsed = JSON.parse(event.payload);
        if ((parsed.type === 6 || parsed.type === 17) && parsed.fromId !== useAuthStore.getState().currentUser?.id) {
          const title = parsed.nickname ?? (parsed.groupId ? `Group #${parsed.groupId}` : `User #${parsed.fromId}`);
          void notifyNative(title, String(parsed.msg ?? 'New message'));
        }
        useMessageStore.getState().handleIncomingMessage(parsed);
      } catch {
        // Ignore malformed messages until schema validation is wired into the UI.
      }
    });

    const unlistenLost = listen<string>('connection-lost', () => {
      setDisconnected();
      scheduleReconnect();
      addToast('info', t('app.toast.connectionLost'));
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
        {currentUser ? t('app.header.welcome', { nickname: currentUser.nickname }) : t('app.header.secureMessaging')}
      </span>
      {offlineQueue.length > 0 && (
        <span className="ml-3 rounded-full bg-amber-400/20 px-2 py-0.5 text-[10px] font-semibold text-amber-700 dark:text-amber-200">
          {t('app.header.queuedMessages', { count: offlineQueue.length })}
        </span>
      )}
      <motion.button
        type="button"
        onClick={toggleTheme}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="ml-3 rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={t('app.header.themeSwitchTo', { theme: theme === 'dark' ? t('app.header.themeLabel.light') : t('app.header.themeLabel.dark') })}
      >
        {theme === 'dark' ? t('app.header.themeLabel.light') : t('app.header.themeLabel.dark')}
      </motion.button>
      <motion.button
        type="button"
        onClick={() => setLanguage(language === 'en' ? 'zh' : 'en')}
        whileHover={{ scale: 1.03 }}
        whileTap={{ scale: 0.97 }}
        className="ml-3 rounded-md bg-light-sidebar px-3 py-1 text-xs transition-opacity hover:opacity-80 dark:bg-dark-sidebar"
        aria-label={t('app.header.toggleLanguage')}
      >
        {language === 'en' ? '中文' : 'EN'}
      </motion.button>
      <HeaderActions />
    </header>
  );

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
                onRegister={(nickname, password) => useAuthStore.getState().register(nickname, password)}
                onSwitchToLogin={() => useAuthStore.getState().setAuthView('login')}
                error={auth.error ?? undefined}
                loading={auth.loading}
              />
            ) : (
              <LoginPanel
                onLogin={(id, password) => useAuthStore.getState().login(id, password)}
                onSwitchToRegister={() => useAuthStore.getState().setAuthView('register')}
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
                    onSelect={(id) => useMessageStore.getState().setActiveContact(id)}
                    onSelectGroup={(id) => useMessageStore.getState().selectGroup(id)}
                    onCreateGroup={(name) => {
                      void useMessageStore.getState().createGroup(name).catch((e) => addToast('error', String(e)));
                    }}
                    onJoinGroup={(groupId) => {
                      void useMessageStore.getState().joinGroup(groupId).catch((e) => addToast('error', String(e)));
                    }}
                    onLeaveGroup={(groupId) => {
                      void useMessageStore.getState().leaveGroup(groupId).catch((e) => addToast('error', String(e)));
                    }}
                    onLogout={() => {
                      useAuthStore.getState().logout();
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
                    void useMessageStore.getState().sendGroupMessage(activeGroup.groupId, content).catch((e) => addToast('error', String(e)));
                  }}
                  onTyping={() => {
                    void useMessageStore.getState().sendTypingIndicator({ type: 'group', id: activeGroup.groupId });
                  }}
                  onRetryFailed={(message) => {
                    void useMessageStore.getState().sendGroupMessage(activeGroup.groupId, message.content).catch((e) => addToast('error', String(e)));
                  }}
                  onEditMessage={(message, content) => {
                    void useMessageStore.getState().editMessage({ type: 'group', id: activeGroup.groupId }, message.id, content).catch((e) => addToast('error', String(e)));
                  }}
                  onDeleteMessage={(message) => {
                    void useMessageStore.getState().deleteMessage({ type: 'group', id: activeGroup.groupId }, message.id).catch((e) => addToast('error', String(e)));
                  }}
                  onReactMessage={(message, reaction) => {
                    void useMessageStore.getState().reactToMessage({ type: 'group', id: activeGroup.groupId }, message.id, reaction).catch((e) => addToast('error', String(e)));
                  }}
                  friendIds={friends.map((friend) => friend.id)}
                  onAddFriend={(memberId) => {
                    void useFriendStore.getState().sendFriendRequest(memberId, t('store.groupChatGreeting')).catch((e) => addToast('error', String(e)));
                  }}
                />
              ) : activeContact ? (
                <ChatArea
                  messages={activeMessages}
                  currentUserId={currentUser.id}
                  contactName={activeContact.nickname}
                  connected={connected}
                  onSend={(content) => {
                    void useMessageStore.getState().sendPrivateMessage(activeContact.id, content).catch((e) => addToast('error', String(e)));
                  }}
                  onTyping={() => {
                    void useMessageStore.getState().sendTypingIndicator({ type: 'direct', id: activeContact.id });
                  }}
                  onRetryFailed={(message) => {
                    void useMessageStore.getState().sendPrivateMessage(activeContact.id, message.content).catch((e) => addToast('error', String(e)));
                  }}
                  onEditMessage={(message, content) => {
                    void useMessageStore.getState().editMessage({ type: 'direct', id: activeContact.id }, message.id, content).catch((e) => addToast('error', String(e)));
                  }}
                  onDeleteMessage={(message) => {
                    void useMessageStore.getState().deleteMessage({ type: 'direct', id: activeContact.id }, message.id).catch((e) => addToast('error', String(e)));
                  }}
                  onReactMessage={(message, reaction) => {
                    void useMessageStore.getState().reactToMessage({ type: 'direct', id: activeContact.id }, message.id, reaction).catch((e) => addToast('error', String(e)));
                  }}
                />
              ) : (
                <main className="flex flex-1 items-center justify-center px-6 text-center">
                  <p className="text-sm text-light-muted dark:text-dark-muted">
                    {t('app.emptyChat')}
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
