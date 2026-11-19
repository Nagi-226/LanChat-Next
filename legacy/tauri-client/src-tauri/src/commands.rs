use tauri::State;

use crate::secure_store;
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

#[tauri::command]
pub fn secure_store_set(app: tauri::AppHandle, key: String, value: String) -> Result<(), String> {
    secure_store::set(app, key, value)
}

#[tauri::command]
pub fn secure_store_get(app: tauri::AppHandle, key: String) -> Result<Option<String>, String> {
    secure_store::get(app, key)
}

#[tauri::command]
pub fn secure_store_delete(app: tauri::AppHandle, key: String) -> Result<(), String> {
    secure_store::delete(app, key)
}
