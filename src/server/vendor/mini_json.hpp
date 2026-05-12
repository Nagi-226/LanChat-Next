// mini_json.hpp  Minimal JSON parser/serializer for LanChat-Next server
// Single-header, C++17, no external dependencies.
// Purpose: Replace nlohmann/json until it is vendored (v1.2.0).
//
// Supports: null, bool, int64_t, double, string, array, object
// Does NOT support: full Unicode escapes, number exponents, pretty-print
#pragma once

#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace lanchat::vendor::json {

struct Value;

using Array  = std::vector<Value>;
using Object = std::map<std::string, Value>;

struct Value {
    enum Kind { Null, Bool, Int, Float, String, Arr, Obj };

    Kind kind = Null;
    bool     b = false;
    int64_t  i = 0;
    double   d = 0.0;
    std::string s;
    Array    arr;
    Object   obj;

    Value() : kind(Null) {}
    Value(std::nullptr_t) : kind(Null) {}
    Value(bool v) : kind(Bool), b(v) {}
    Value(int v) : kind(Int), i(v) {}
    Value(int64_t v) : kind(Int), i(v) {}
    Value(double v) : kind(Float), d(v) {}
    Value(const char* v) : kind(String), s(v) {}
    Value(std::string v) : kind(String), s(std::move(v)) {}
    Value(Array v) : kind(Arr), arr(std::move(v)) {}
    Value(Object v) : kind(Obj), obj(std::move(v)) {}

    bool is_null()    const { return kind == Null; }
    bool is_bool()    const { return kind == Bool; }
    bool is_number()  const { return kind == Int || kind == Float; }
    bool is_string()  const { return kind == String; }
    bool is_array()   const { return kind == Arr; }
    bool is_object()  const { return kind == Obj; }

    int64_t as_int() const { return i; }
    std::string as_string() const { return s; }

    bool contains(const std::string& key) const {
        return kind == Obj && obj.find(key) != obj.end();
    }

    Value& operator[](const std::string& key) {
        return obj[key];
    }

    const Value& at(const std::string& key) const {
        static Value null_val;
        if (kind != Obj) return null_val;
        auto it = obj.find(key);
        return it != obj.end() ? it->second : null_val;
    }
};

namespace detail {

class Parser {
public:
    explicit Parser(const std::string& src) : s(src), pos(0) {}

    Value parse() {
        skip_ws();
        Value v = parse_value();
        skip_ws();
        if (pos != s.size()) throw std::runtime_error("trailing content");
        return v;
    }

private:
    const std::string& s;
    size_t pos;

    char peek() { return pos < s.size() ? s[pos] : '\0'; }
    char next() { return s[pos++]; }

    void skip_ws() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r')) ++pos;
    }

    Value parse_value() {
        char c = peek();
        if (c == 'n') return parse_literal("null", Value(nullptr));
        if (c == 't') return parse_literal("true", Value(true));
        if (c == 'f') return parse_literal("false", Value(false));
        if (c == '"') return parse_string();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        throw std::runtime_error(std::string("unexpected character: ") + c);
    }

    Value parse_literal(const char* expected, Value result) {
        for (const char* p = expected; *p; ++p) {
            if (next() != *p) throw std::runtime_error(std::string("expected ") + expected);
        }
        return result;
    }

    Value parse_string() {
        next(); // skip opening "
        std::string result;
        while (pos < s.size()) {
            char c = next();
            if (c == '"') return Value(result);
            if (c == '\\') {
                char esc = next();
                switch (esc) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    case 'r':  result += '\r'; break;
                    default:   result += esc;  break;
                }
            } else {
                result += c;
            }
        }
        throw std::runtime_error("unterminated string");
    }

    Value parse_number() {
        size_t start = pos;
        if (peek() == '-') next();
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        bool is_float = false;
        if (pos < s.size() && s[pos] == '.') {
            is_float = true;
            ++pos;
            while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
        }
        std::string num = s.substr(start, pos - start);
        if (is_float) {
            return Value(std::stod(num));
        }
        return Value(static_cast<int64_t>(std::stoll(num)));
    }

    Value parse_array() {
        next(); // skip '['
        Array arr;
        skip_ws();
        if (peek() == ']') { next(); return Value(arr); }
        while (true) {
            arr.push_back(parse_value());
            skip_ws();
            char c = next();
            if (c == ']') break;
            if (c != ',') throw std::runtime_error("expected , or ] in array");
            skip_ws();
        }
        return Value(arr);
    }

    Value parse_object() {
        next(); // skip '{'
        Object obj;
        skip_ws();
        if (peek() == '}') { next(); return Value(obj); }
        while (true) {
            skip_ws();
            Value key = parse_string();
            skip_ws();
            if (next() != ':') throw std::runtime_error("expected : in object");
            skip_ws();
            obj[key.as_string()] = parse_value();
            skip_ws();
            char c = next();
            if (c == '}') break;
            if (c != ',') throw std::runtime_error("expected , or } in object");
        }
        return Value(obj);
    }
};

} // namespace detail

inline Value parse(const std::string& src) {
    return detail::Parser(src).parse();
}

namespace detail {

inline void serialize_string(std::string& out, const std::string& s) {
    out += '"';
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    out += '"';
}

inline void serialize(std::string& out, const Value& v) {
    switch (v.kind) {
        case Value::Null:
            out += "null";
            break;
        case Value::Bool:
            out += v.b ? "true" : "false";
            break;
        case Value::Int:
            out += std::to_string(v.i);
            break;
        case Value::Float:
            out += std::to_string(v.d);
            break;
        case Value::String:
            serialize_string(out, v.s);
            break;
        case Value::Arr:
            out += '[';
            for (size_t i = 0; i < v.arr.size(); ++i) {
                if (i > 0) out += ',';
                serialize(out, v.arr[i]);
            }
            out += ']';
            break;
        case Value::Obj:
            out += '{';
            bool first = true;
            for (auto& [key, val] : v.obj) {
                if (!first) out += ',';
                first = false;
                serialize_string(out, key);
                out += ':';
                serialize(out, val);
            }
            out += '}';
            break;
    }
}

} // namespace detail

inline std::string serialize(const Value& v) {
    std::string out;
    detail::serialize(out, v);
    return out;
}

inline Value make_obj() { return Value(Object{}); }
inline Value make_arr() { return Value(Array{}); }

} // namespace lanchat::vendor::json
