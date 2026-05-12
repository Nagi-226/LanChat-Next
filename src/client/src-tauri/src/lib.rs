mod commands;
mod message_codec;
mod notifications;
mod tcp_manager;

use tcp_manager::new_shared_manager;

pub fn run() {
    let manager = new_shared_manager();
    let manager_for_setup = manager.clone();

    tauri::Builder::default()
        .setup(move |app| {
            manager_for_setup.set_app_handle(app.handle().clone());
            notifications::notify_skeleton_ready();
            Ok(())
        })
        .manage(manager)
        .invoke_handler(tauri::generate_handler![
            commands::connect,
            commands::disconnect,
            commands::send_raw_json,
        ])
        .run(tauri::generate_context!())
        .expect("failed to run LanChat-Next client");
}
