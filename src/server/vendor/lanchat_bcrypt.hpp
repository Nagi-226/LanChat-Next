#pragma once

extern "C" {
#include "openwall_bcrypt/ow-crypt.h"
}

#include <random>
#include <string>

namespace lanchat::vendor::bcrypt {

inline std::string random_bytes(size_t bytes = 16)
{
    static std::mt19937_64 rng(std::random_device{}());
    std::string bytes_out;
    bytes_out.resize(bytes);
    for (char& out : bytes_out) {
        out = static_cast<char>(rng() & 0xff);
    }
    return bytes_out;
}

inline std::string hash_password(const std::string& password, int cost = 12)
{
    char salt[32] = {};
    char output[64] = {};
    const std::string entropy = random_bytes(16);
    if (!crypt_gensalt_rn("$2b$", static_cast<unsigned long>(cost),
                          entropy.data(), static_cast<int>(entropy.size()),
                          salt, static_cast<int>(sizeof(salt)))) {
        return {};
    }
    if (!crypt_rn(password.c_str(), salt, output, static_cast<int>(sizeof(output)))) {
        return {};
    }
    return output;
}

inline bool verify_password(const std::string& password, const std::string& stored)
{
    if (stored.rfind("$2", 0) != 0) return false;
    char output[64] = {};
    if (!crypt_rn(password.c_str(), stored.c_str(), output, static_cast<int>(sizeof(output)))) {
        return false;
    }
    return stored == output;
}

} // namespace lanchat::vendor::bcrypt
