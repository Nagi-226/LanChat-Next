mod commands;
mod message_codec;
mod notifications;
mod secure_store;
mod tcp_manager;

use std::process::{Child, Command};
use std::sync::Mutex;
use tauri::Manager;
use tcp_manager::new_shared_manager;

/// Holds the C++ server child process so we can kill it on shutdown.
struct ServerProcess(Mutex<Option<Child>>);

impl ServerProcess {
    fn kill_server(&self) {
        if let Ok(mut guard) = self.0.lock() {
            if let Some(ref mut child) = *guard {
                let _ = child.kill();
                let _ = child.wait();
            }
        }
    }
}

impl Drop for ServerProcess {
    fn drop(&mut self) {
        self.kill_server();
    }
}

/// Find the C++ server binary.
/// - Production: next to the current executable
/// - Development: in src-tauri/binaries/ relative to the manifest dir
fn find_server_binary() -> Option<std::path::PathBuf> {
    // 1. Try next to the current executable (production / installed)
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let candidate = dir.join("lanchat_server_next.exe");
            if candidate.exists() {
                return Some(candidate);
            }
            // Also try without .exe extension
            let candidate2 = dir.join("lanchat_server_next");
            if candidate2.exists() {
                return Some(candidate2);
            }
        }
    }

    // 2. Try the binaries directory (development)
    let manifest_dir = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dev_path = manifest_dir.join("binaries")
        .join("lanchat_server_next-x86_64-pc-windows-msvc.exe");
    if dev_path.exists() {
        return Some(dev_path);
    }

    // 3. Try without target triple (alternative dev config)
    let dev_path2 = manifest_dir.join("binaries").join("lanchat_server_next.exe");
    if dev_path2.exists() {
        return Some(dev_path2);
    }

    None
}

pub fn run() {
    let manager = new_shared_manager();
    let manager_for_setup = manager.clone();

    // Find and spawn the C++ server (without a visible console window)
    let server_path = find_server_binary();
    let server_proc = server_path.and_then(|path| {
        let mut cmd = Command::new(&path);
        cmd.args(["--port", "12346"])
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::null());
        #[cfg(windows)]
        {
            use std::os::windows::process::CommandExt;
            const CREATE_NO_WINDOW: u32 = 0x08000000;
            cmd.creation_flags(CREATE_NO_WINDOW);
        }
        cmd.spawn().ok()
    });
    let server_holder = ServerProcess(Mutex::new(server_proc));

    let app = tauri::Builder::default()
        .manage(server_holder)
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
            commands::secure_store_set,
            commands::secure_store_get,
            commands::secure_store_delete,
        ])
        .build(tauri::generate_context!())
        .expect("failed to build LanChat-Next client");

    app.run(|app_handle, event| {
        if let tauri::RunEvent::Exit = event {
            // Explicitly kill the C++ server on app exit
            if let Some(state) = app_handle.try_state::<ServerProcess>() {
                state.kill_server();
            }
        }
    });
}
