mod commands;
mod message_codec;
mod notifications;
mod tcp_manager;

pub fn run() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![
            commands::connect,
            commands::disconnect,
            commands::send_raw_json,
        ])
        .run(tauri::generate_context!())
        .expect("failed to run LanChat-Next client");
}
