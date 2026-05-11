use std::sync::OnceLock;

use crate::tcp_manager::{new_shared_manager, SharedTcpManager};

fn tcp() -> &'static SharedTcpManager {
    static TCP: OnceLock<SharedTcpManager> = OnceLock::new();
    TCP.get_or_init(new_shared_manager)
}

#[tauri::command]
pub async fn connect(host: String, port: u16) -> Result<(), String> {
    tcp().connect(host, port).await
}

#[tauri::command]
pub async fn disconnect() -> Result<(), String> {
    tcp().disconnect().await;
    Ok(())
}

#[tauri::command]
pub async fn send_raw_json(json: String) -> Result<(), String> {
    tcp().send_json(json).await
}
