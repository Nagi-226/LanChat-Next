use std::sync::Arc;

use tauri::Emitter;
use tokio::{
    io::{AsyncReadExt, AsyncWriteExt},
    net::{tcp::OwnedWriteHalf, TcpStream},
    sync::Mutex,
    task::JoinHandle,
};

use crate::message_codec::{encode_frame, try_decode_frame};

pub struct TcpManager {
    writer: Arc<Mutex<Option<OwnedWriteHalf>>>,
    app_handle: Mutex<Option<tauri::AppHandle>>,
    read_task: Mutex<Option<JoinHandle<()>>>,
}

impl TcpManager {
    pub fn set_app_handle(&self, handle: tauri::AppHandle) {
        *self.app_handle.blocking_lock() = Some(handle);
    }

    pub async fn connect(&self, host: String, port: u16) -> Result<(), String> {
        self.disconnect().await;

        let stream = TcpStream::connect((host.as_str(), port))
            .await
            .map_err(|err| err.to_string())?;
        let (mut reader, writer) = stream.into_split();

        *self.writer.lock().await = Some(writer);

        let handle = self.app_handle.lock().await.clone();
        let writer_arc = Arc::clone(&self.writer);

        let task = tokio::spawn(async move {
            let mut buf = vec![0u8; 8192];
            let mut accumulator: Vec<u8> = Vec::new();

            loop {
                let n = match reader.read(&mut buf).await {
                    Ok(0) => {
                        *writer_arc.lock().await = None;
                        if let Some(ref h) = handle {
                            let _ = h.emit("connection-lost", "server closed");
                        }
                        return;
                    }
                    Ok(n) => n,
                    Err(_) => {
                        *writer_arc.lock().await = None;
                        if let Some(ref h) = handle {
                            let _ = h.emit("connection-lost", "read error");
                        }
                        return;
                    }
                };

                accumulator.extend_from_slice(&buf[..n]);

                // Decode complete frames
                while let Some(json) = try_decode_frame(&mut accumulator) {
                    if let Some(ref h) = handle {
                        let _ = h.emit("message-received", &json);
                    }
                }

                // Prevent unbounded memory growth
                if accumulator.len() > 4 * 1024 * 1024 {
                    accumulator.clear();
                }
            }
        });

        *self.read_task.lock().await = Some(task);
        Ok(())
    }

    pub async fn disconnect(&self) {
        if let Some(task) = self.read_task.lock().await.take() {
            task.abort();
        }
        *self.writer.lock().await = None;
    }

    pub async fn send_json(&self, json: String) -> Result<(), String> {
        let mut guard = self.writer.lock().await;
        let Some(writer) = guard.as_mut() else {
            return Err("not connected".to_string());
        };
        writer
            .write_all(&encode_frame(&json))
            .await
            .map_err(|err| err.to_string())
    }
}

impl Default for TcpManager {
    fn default() -> Self {
        Self {
            writer: Arc::new(Mutex::new(None)),
            app_handle: Mutex::new(None),
            read_task: Mutex::new(None),
        }
    }
}

pub type SharedTcpManager = Arc<TcpManager>;

pub fn new_shared_manager() -> SharedTcpManager {
    Arc::new(TcpManager::default())
}
