import { AnimatePresence, motion } from 'framer-motion';
import { useConnectionStore } from '../stores/connectionStore';
import ClickSpark from '../lib/ClickSpark';
import ShinyText from '../lib/ShinyText';

export function ConnectionBar() {
  const status = useConnectionStore((s) => s.status);
  const host = useConnectionStore((s) => s.host);
  const port = useConnectionStore((s) => s.port);
  const error = useConnectionStore((s) => s.error);
  const lastHeartbeat = useConnectionStore((s) => s.lastHeartbeat);
  const retryCount = useConnectionStore((s) => s.retryCount);
  const retryTimer = useConnectionStore((s) => s.retryTimer);
  const recentServers = useConnectionStore((s) => s.recentServers);
  const connect = useConnectionStore((s) => s.connect);
  const disconnect = useConnectionStore((s) => s.disconnect);
  const setHost = useConnectionStore((s) => s.setHost);
  const setPort = useConnectionStore((s) => s.setPort);
  const selectServer = useConnectionStore((s) => s.selectServer);
  const dismissError = useConnectionStore((s) => s.dismissError);

  const handleConnect = () => connect(host, port);
  const heartbeatAgo = lastHeartbeat ? Math.round((Date.now() - lastHeartbeat) / 1000) : null;
  const selectedRecent = recentServers.findIndex((server) => server.host === host && server.port === port);

  return (
    <div className="flex flex-wrap items-center gap-2 text-xs">
      <label htmlFor="server-host" className="sr-only">
        Server host
      </label>
      <input
        id="server-host"
        value={host}
        onChange={(e) => setHost(e.target.value)}
        placeholder="127.0.0.1"
        className="w-28 rounded border border-light-border bg-light-sidebar px-2 py-1 text-light-text outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-text"
      />
      <label htmlFor="server-port" className="sr-only">
        Server port
      </label>
      <input
        id="server-port"
        type="number"
        value={port}
        onChange={(e) => setPort(Number(e.target.value))}
        placeholder="12346"
        className="w-20 rounded border border-light-border bg-light-sidebar px-2 py-1 text-light-text outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-text"
      />

      {recentServers.length > 0 && (
        <label className="flex items-center gap-1 text-light-muted dark:text-dark-muted">
          <span className="sr-only">Recent server</span>
          <select
            value={selectedRecent >= 0 ? selectedRecent : ''}
            onChange={(e) => {
              const index = Number(e.target.value);
              const server = recentServers[index];
              if (server) selectServer(server);
            }}
            className="max-w-[150px] rounded border border-light-border bg-light-sidebar px-2 py-1 text-light-text outline-none focus:border-dark-highlight dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-text"
            aria-label="Select recent server"
          >
            <option value="">Recent</option>
            {recentServers.map((server, index) => (
              <option key={`${server.host}:${server.port}`} value={index}>
                {server.host}:{server.port}
              </option>
            ))}
          </select>
        </label>
      )}

      {status === 'disconnected' && (
        <ClickSpark sparkColor="#52c41a" sparkCount={6} sparkSize={5}>
          <motion.button
            type="button"
            onClick={handleConnect}
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="rounded bg-light-accent px-3 py-1 text-white transition-opacity hover:opacity-80 dark:bg-dark-accent"
          >
            Connect
          </motion.button>
        </ClickSpark>
      )}
      {status === 'connecting' && (
        <span className="flex items-center gap-1">
          <ShinyText text={`Connecting...${retryTimer ? ` retry in ${Math.ceil(retryTimer / 1000)}s` : ''}`} color="#f59e0b" />
          {retryCount > 0 && (
            <span className="text-light-muted dark:text-dark-muted">
              attempt {retryCount}/5
            </span>
          )}
        </span>
      )}
      {status === 'connected' && (
        <>
          <span>
            <ShinyText text="Connected" color="#22c55e" shineColor="#ffffff" />
          </span>
          {heartbeatAgo !== null && (
            <span className="text-light-muted dark:text-dark-muted">
              heartbeat {heartbeatAgo}s ago
            </span>
          )}
          <motion.button
            type="button"
            onClick={disconnect}
            whileHover={{ scale: 1.05 }}
            whileTap={{ scale: 0.95 }}
            className="rounded bg-dark-highlight px-3 py-1 text-white transition-opacity hover:opacity-80"
          >
            Disconnect
          </motion.button>
        </>
      )}
      <AnimatePresence initial={false}>
        {error && (
          <motion.span
            initial={{ opacity: 0, y: 6, scale: 0.96 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: -6, scale: 0.96 }}
            className="flex max-w-[320px] items-center gap-1 rounded-full border border-red-400/30 bg-red-500/10 px-2 py-0.5 text-red-500 dark:text-red-300"
          >
            <span className="truncate">! {error}</span>
            <button
              type="button"
              onClick={dismissError}
              className="rounded px-1 hover:bg-red-500/10"
              aria-label="Dismiss connection error"
            >
              x
            </button>
          </motion.span>
        )}
      </AnimatePresence>
    </div>
  );
}
