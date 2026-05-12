import { useConnectionStore } from '../stores/connectionStore';

export function ConnectionBar() {
  const status = useConnectionStore((s) => s.status);
  const host = useConnectionStore((s) => s.host);
  const port = useConnectionStore((s) => s.port);
  const error = useConnectionStore((s) => s.error);
  const lastHeartbeat = useConnectionStore((s) => s.lastHeartbeat);
  const connect = useConnectionStore((s) => s.connect);
  const disconnect = useConnectionStore((s) => s.disconnect);
  const setHost = useConnectionStore((s) => s.setHost);
  const setPort = useConnectionStore((s) => s.setPort);

  const handleConnect = () => connect(host, port);
  const heartbeatAgo = lastHeartbeat ? Math.round((Date.now() - lastHeartbeat) / 1000) : null;

  return (
    <div className="flex items-center gap-3 text-xs">
      <input
        value={host}
        onChange={(e) => setHost(e.target.value)}
        placeholder="127.0.0.1"
        className="w-24 px-2 py-1 rounded bg-light-sidebar dark:bg-dark-sidebar
                   border border-light-border dark:border-dark-border
                   text-light-text dark:text-dark-text outline-none"
      />
      <input
        type="number"
        value={port}
        onChange={(e) => setPort(Number(e.target.value))}
        placeholder="12346"
        className="w-16 px-2 py-1 rounded bg-light-sidebar dark:bg-dark-sidebar
                   border border-light-border dark:border-dark-border
                   text-light-text dark:text-dark-text outline-none"
      />

      {status === 'disconnected' && (
        <button
          onClick={handleConnect}
          className="px-3 py-1 rounded bg-light-accent dark:bg-dark-accent text-white
                     hover:opacity-80 transition-opacity"
        >
          Connect
        </button>
      )}
      {status === 'connecting' && (
        <span className="text-light-muted dark:text-dark-muted">Connecting...</span>
      )}
      {status === 'connected' && (
        <>
          <span className="text-green-500">Connected</span>
          {heartbeatAgo !== null && (
            <span className="text-light-muted dark:text-dark-muted">
              heartbeat {heartbeatAgo}s ago
            </span>
          )}
          <button
            onClick={disconnect}
            className="px-3 py-1 rounded bg-dark-highlight text-white
                       hover:opacity-80 transition-opacity"
          >
            Disconnect
          </button>
        </>
      )}
      {error && (
        <span className="text-red-400">{error}</span>
      )}
    </div>
  );
}
