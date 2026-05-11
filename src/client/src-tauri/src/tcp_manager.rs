use std::sync::Arc;

use tokio::{
    io::AsyncWriteExt,
    net::TcpStream,
    sync::Mutex,
};

use crate::message_codec::encode_frame;

#[derive(Default)]
pub struct TcpManager {
    stream: Mutex<Option<TcpStream>>,
}

impl TcpManager {
    pub async fn connect(&self, host: String, port: u16) -> Result<(), String> {
        let stream = TcpStream::connect((host.as_str(), port)).await.map_err(|err| err.to_string())?;
        *self.stream.lock().await = Some(stream);
        Ok(())
    }

    pub async fn disconnect(&self) {
        *self.stream.lock().await = None;
    }

    pub async fn send_json(&self, json: String) -> Result<(), String> {
        let mut guard = self.stream.lock().await;
        let Some(stream) = guard.as_mut() else {
            return Err("not connected".to_string());
        };
        stream.write_all(&encode_frame(&json)).await.map_err(|err| err.to_string())
    }
}

pub type SharedTcpManager = Arc<TcpManager>;

pub fn new_shared_manager() -> SharedTcpManager {
    Arc::new(TcpManager::default())
}
