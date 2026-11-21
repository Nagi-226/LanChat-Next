#pragma once

#include "message_types.h"
#include "mini_json.hpp"

#include <cstdint>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace lanchat::server::protocol_json {

using lanchat::protocol::MsgType;
using vendor::json::Array;
using vendor::json::Object;
using vendor::json::Value;

inline Object parseObject(const std::string& json, bool& ok) {
    ok = false;
    try {
        auto value = vendor::json::parse(json);
        if (!value.is_object()) {
            return {};
        }
        ok = true;
        return value.obj;
    } catch (...) {
        return {};
    }
}

inline bool has(const Object& obj, const std::string& key) {
    return obj.find(key) != obj.end();
}

inline int intField(const Object& obj, const std::string& key, int fallback = 0) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return fallback;
    }
    return static_cast<int>(it->second.as_int());
}

inline std::int64_t int64Field(
    const Object& obj,
    const std::string& key,
    std::int64_t fallback = 0) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.is_number()) {
        return fallback;
    }
    return it->second.as_int();
}

// msg_id is declared as string in the JSON schemas, but legacy clients send a
// number. Accept both so schema-compliant and legacy clients keep working.
inline std::int64_t msgIdField(
    const Object& obj,
    const std::string& key = "msg_id",
    std::int64_t fallback = 0) {
    auto it = obj.find(key);
    if (it == obj.end()) {
        return fallback;
    }
    if (it->second.is_number()) {
        return it->second.as_int();
    }
    if (it->second.is_string()) {
        const std::string text = it->second.as_string();
        try {
            return static_cast<std::int64_t>(std::stoll(text));
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

inline std::string stringField(
    const Object& obj,
    const std::string& key,
    const std::string& fallback = "") {
    auto it = obj.find(key);
    if (it == obj.end() || !it->second.is_string()) {
        return fallback;
    }
    return it->second.as_string();
}

inline MsgType typeField(const Object& obj) {
    return lanchat::protocol::fromInt(intField(obj, lanchat::protocol::field::TYPE, -1));
}

inline std::string serialize(const Object& obj) {
    return vendor::json::serialize(Value(obj));
}

inline std::string ok(MsgType type, const std::string& msg = "ok") {
    Object obj;
    obj["type"] = Value(lanchat::protocol::toInt(type));
    obj["status"] = Value("ok");
    obj["msg"] = Value(msg);
    return serialize(obj);
}

inline std::string error(MsgType type, int code, const std::string& msg) {
    Object obj;
    obj["type"] = Value(lanchat::protocol::toInt(type));
    obj["status"] = Value("error");
    obj["code"] = Value(code);
    obj["msg"] = Value(msg);
    return serialize(obj);
}

} // namespace lanchat::server::protocol_json
