use std::fs;
use std::path::PathBuf;

use tauri::{AppHandle, Manager};

fn storage_dir(app: &AppHandle) -> Result<PathBuf, String> {
    let dir = app
        .path()
        .app_data_dir()
        .map_err(|err| format!("app data dir unavailable: {err}"))?
        .join("secure-store");
    fs::create_dir_all(&dir).map_err(|err| format!("create secure store failed: {err}"))?;
    Ok(dir)
}

fn safe_file_name(key: &str) -> Result<String, String> {
    if key.is_empty() || key.len() > 128 {
        return Err("secure store key length invalid".to_string());
    }
    if !key
        .bytes()
        .all(|b| b.is_ascii_alphanumeric() || matches!(b, b':' | b'_' | b'-' | b'.'))
    {
        return Err("secure store key contains unsupported characters".to_string());
    }
    Ok(format!("{}.bin", key.replace(':', "__")))
}

fn path_for(app: &AppHandle, key: &str) -> Result<PathBuf, String> {
    Ok(storage_dir(app)?.join(safe_file_name(key)?))
}

pub fn set(app: AppHandle, key: String, value: String) -> Result<(), String> {
    let encrypted = platform_encrypt(value.as_bytes())?;
    fs::write(path_for(&app, &key)?, encrypted).map_err(|err| format!("secure store write failed: {err}"))
}

pub fn get(app: AppHandle, key: String) -> Result<Option<String>, String> {
    let path = path_for(&app, &key)?;
    if !path.exists() {
        return Ok(None);
    }
    let encrypted = fs::read(path).map_err(|err| format!("secure store read failed: {err}"))?;
    let decrypted = platform_decrypt(&encrypted)?;
    String::from_utf8(decrypted)
        .map(Some)
        .map_err(|err| format!("secure store value is not utf-8: {err}"))
}

pub fn delete(app: AppHandle, key: String) -> Result<(), String> {
    let path = path_for(&app, &key)?;
    if path.exists() {
        fs::remove_file(path).map_err(|err| format!("secure store delete failed: {err}"))?;
    }
    Ok(())
}

#[cfg(windows)]
fn platform_encrypt(input: &[u8]) -> Result<Vec<u8>, String> {
    use std::ptr::null_mut;
    use std::slice;
    use windows_sys::Win32::Foundation::LocalFree;
    use windows_sys::Win32::Security::Cryptography::{CryptProtectData, CRYPT_INTEGER_BLOB};

    let mut in_blob = CRYPT_INTEGER_BLOB {
        cbData: input.len() as u32,
        pbData: input.as_ptr() as *mut u8,
    };
    let mut out_blob = CRYPT_INTEGER_BLOB {
        cbData: 0,
        pbData: null_mut(),
    };

    let ok = unsafe {
        CryptProtectData(
            &mut in_blob,
            std::ptr::null(),
            std::ptr::null(),
            null_mut(),
            std::ptr::null(),
            0,
            &mut out_blob,
        )
    };
    if ok == 0 {
        return Err("Windows DPAPI encryption failed".to_string());
    }

    let bytes = unsafe { slice::from_raw_parts(out_blob.pbData, out_blob.cbData as usize).to_vec() };
    unsafe {
        LocalFree(out_blob.pbData.cast());
    }
    Ok(bytes)
}

#[cfg(windows)]
fn platform_decrypt(input: &[u8]) -> Result<Vec<u8>, String> {
    use std::ptr::null_mut;
    use std::slice;
    use windows_sys::Win32::Foundation::LocalFree;
    use windows_sys::Win32::Security::Cryptography::{CryptUnprotectData, CRYPT_INTEGER_BLOB};

    let mut in_blob = CRYPT_INTEGER_BLOB {
        cbData: input.len() as u32,
        pbData: input.as_ptr() as *mut u8,
    };
    let mut out_blob = CRYPT_INTEGER_BLOB {
        cbData: 0,
        pbData: null_mut(),
    };

    let ok = unsafe {
        CryptUnprotectData(
            &mut in_blob,
            null_mut(),
            std::ptr::null(),
            null_mut(),
            std::ptr::null(),
            0,
            &mut out_blob,
        )
    };
    if ok == 0 {
        return Err("Windows DPAPI decryption failed".to_string());
    }

    let bytes = unsafe { slice::from_raw_parts(out_blob.pbData, out_blob.cbData as usize).to_vec() };
    unsafe {
        LocalFree(out_blob.pbData.cast());
    }
    Ok(bytes)
}

#[cfg(not(windows))]
fn platform_encrypt(input: &[u8]) -> Result<Vec<u8>, String> {
    Ok(input.to_vec())
}

#[cfg(not(windows))]
fn platform_decrypt(input: &[u8]) -> Result<Vec<u8>, String> {
    Ok(input.to_vec())
}
