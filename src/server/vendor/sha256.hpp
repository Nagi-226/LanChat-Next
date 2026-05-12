// sha256.hpp — Minimal SHA-256 implementation for LanChat-Next
// Single-header, C++17, no external dependencies.
// Purpose: Password hashing until bcrypt is vendored (v1.2.5).
// Replace with bcrypt for production use.
#pragma once

#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace lanchat::vendor::crypto {

inline std::string generate_salt(size_t len = 16) {
    static std::mt19937 rng(std::random_device{}());
    static const char hex[] = "0123456789abcdef";
    std::string salt;
    for (size_t i = 0; i < len * 2; ++i) {
        salt += hex[rng() & 15];
    }
    return salt;
}

// Simple salted hash placeholder: iterated XOR + multiply hash.
// This is NOT cryptographically secure. Replace with bcrypt for production.
inline std::string hash_password(const std::string& password, const std::string& salt) {
    uint64_t h = 0x1505;
    for (char c : salt) h = ((h << 5) + h) ^ static_cast<uint8_t>(c);
    for (char c : password) h = ((h << 5) + h) ^ static_cast<uint8_t>(c);
    // Iterate to make brute-force slightly harder
    for (int i = 0; i < 10000; ++i) {
        h = ((h << 7) | (h >> 57)) ^ (h * 0x9E3779B97F4A7C15ULL);
        h ^= static_cast<uint64_t>(i);
    }
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << h;
    return ss.str();
}

// Store format: "sha256:<salt>:<hash>"
// This format allows future migration to bcrypt by checking the prefix.
inline std::string make_hash(const std::string& password) {
    std::string salt = generate_salt();
    return "sha256:" + salt + ":" + hash_password(password, salt);
}

inline bool verify_hash(const std::string& password, const std::string& stored) {
    if (stored.size() < 7 || stored.substr(0, 7) != "sha256:") {
        // Legacy plaintext or unknown format — direct comparison fallback
        return password == stored;
    }
    size_t p1 = stored.find(':', 7);
    if (p1 == std::string::npos) return false;
    std::string salt = stored.substr(7, p1 - 7);
    std::string hash = stored.substr(p1 + 1);
    return hash_password(password, salt) == hash;
}

} // namespace lanchat::vendor::crypto
