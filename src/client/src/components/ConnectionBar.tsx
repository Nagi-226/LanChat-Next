import { useConnectionStore } from '../stores/connectionStore';
import ClickSpark from '../lib/ClickSpark';

export function ConnectionBar() {
  const status = useConnectionStore((s) => s.status);
  const host = useConnectionStore((s) => s.host);
  const port = useConnectionStore((s) => s.port);
  const error = useConnectionStore((s) => s.error);
  const lastHeartbeat = useConnectionStore((s) => s.lastHeartbeat);
  const retryTimer = useConnectionStore((s) => s.retryTimer);
  const connect = useConnectionStore((s) => s.connect);
  const disconnect = useConnectionStore((s) => s.disconnect);
  const setHost = useConnectionStore((s) => s.setHost);
  const setPort = useConnectionStore((s) => s.setPort);

  const handleConnect = () => connect(host, port);
  const heartbeatAgo = lastHeartbeat ? Math.round((Date.now() - lastHeartbeat) / 1000) : null;

  const statusClass =
    status === 'connected'
      ? 'text-green-500'
      : status === 'connecting'
        ? 'text-amber-400'
        : 'text-light-muted dark:text-dark-muted';

  return (
    <div className="flex flex-wrap items-center gap-3 text-xs">
      <input
        value={host}
        onChange={(e) => setHost(e.target.value)}
        placeholder="127.0.0.1"
        className="w-28 rounded border border-light-border bg-light-sidebar px-2 py-1 text-light-text outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-text"
      />
      <input
        type="number"
        value={port}
        onChange={(e) => setPort(Number(e.target.value))}
        placeholder="12346"
        className="w-20 rounded border border-light-border bg-light-sidebar px-2 py-1 text-light-text outline-none transition-colors focus:border-dark-highlight dark:border-dark-border dark:bg-dark-sidebar dark:text-dark-text"
      />

      {status === 'disconnected' && (
        <ClickSpark sparkColor="#52c41a" sparkCount={6} sparkSize={5}>
          <button
            type="button"
            onClick={handleConnect}
            className="rounded bg-light-accent px-3 py-1 text-white transition-opacity hover:opacity-80 dark:bg-dark-accent"
          >
            Connect
          </button>
        </ClickSpark>
      )}
      {status === 'connecting' && (
        <span className={statusClass}>
          Connecting...{retryTimer ? ` retry in ${Math.ceil(retryTimer / 1000)}s` : ''}
        </span>
      )}
      {status === 'connected' && (
        <>
          <span className={statusClass}>Connected</span>
          {heartbeatAgo !== null && (
            <span className="text-light-muted dark:text-dark-muted">
              heartbeat {heartbeatAgo}s ago
            </span>
          )}
          <button
            type="button"
            onClick={disconnect}
            className="rounded bg-dark-highlight px-3 py-1 text-white transition-opacity hover:opacity-80"
          >
            Disconnect
          </button>
        </>
      )}
      {error && <span className="max-w-[320px] truncate text-red-400">{error}</span>}
    </div>
  );
}
