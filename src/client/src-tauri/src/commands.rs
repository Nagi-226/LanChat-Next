use tauri::State;

use crate::tcp_manager::SharedTcpManager;

#[tauri::command]
pub async fn connect(tcp: State<'_, SharedTcpManager>, host: String, port: u16) -> Result<(), String> {
    tcp.connect(host, port).await
}

#[tauri::command]
pub async fn disconnect(tcp: State<'_, SharedTcpManager>) -> Result<(), String> {
    tcp.disconnect().await;
    Ok(())
}

#[tauri::command]
pub async fn send_raw_json(tcp: State<'_, SharedTcpManager>, json: String) -> Result<(), String> {
    tcp.send_json(json).await
}
