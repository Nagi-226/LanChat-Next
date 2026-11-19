pub fn encode_frame(json: &str) -> Vec<u8> {
    let body = json.as_bytes();
    let len = body.len() as u32;
    let mut frame = Vec::with_capacity(4 + body.len());
    frame.extend_from_slice(&len.to_be_bytes());
    frame.extend_from_slice(body);
    frame
}

pub fn try_decode_frame(buffer: &mut Vec<u8>) -> Option<String> {
    if buffer.len() < 4 {
        return None;
    }
    let len = u32::from_be_bytes([buffer[0], buffer[1], buffer[2], buffer[3]]) as usize;
    if len == 0 || len > 4 * 1024 * 1024 {
        buffer.clear();
        return Some(r#"{"type":33,"status":"error","code":400,"msg":"invalid frame"}"#.to_string());
    }
    if buffer.len() < 4 + len {
        return None;
    }
    let body = buffer[4..4 + len].to_vec();
    buffer.drain(0..4 + len);
    String::from_utf8(body).ok()
}
