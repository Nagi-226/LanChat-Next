#include "lanchat/server/ProtocolJson.h"

#include <cassert>
#include <iostream>

using lanchat::server::protocol_json::error;
using lanchat::server::protocol_json::has;
using lanchat::server::protocol_json::int64Field;
using lanchat::server::protocol_json::intField;
using lanchat::server::protocol_json::ok;
using lanchat::server::protocol_json::parseObject;
using lanchat::server::protocol_json::serialize;
using lanchat::server::protocol_json::stringField;
using lanchat::server::protocol_json::typeField;
using lanchat::protocol::MsgType;

namespace {

void assertParseValidObject() {
    bool ok = false;
    auto obj = parseObject(R"({"type":2,"id":100})", ok);
    assert(ok);
    assert(intField(obj, "type") == 2);
    assert(intField(obj, "id") == 100);
}

void assertParseInvalidJson() {
    bool ok = true;
    auto obj = parseObject("{bad", ok);
    assert(!ok);
}

void assertParseArrayReturnsInvalid() {
    bool ok = false;
    auto obj = parseObject("[1,2,3]", ok);
    assert(!ok);
}

void assertParseEmptyString() {
    bool ok = false;
    auto obj = parseObject("", ok);
    assert(!ok);
}

void assertHasExistingKey() {
    bool ok = false;
    auto obj = parseObject(R"({"name":"test"})", ok);
    assert(ok);
    assert(has(obj, "name"));
}

void assertHasMissingKey() {
    bool ok = false;
    auto obj = parseObject(R"({"name":"test"})", ok);
    assert(ok);
    assert(!has(obj, "missing"));
}

void assertIntFieldPresent() {
    bool ok = false;
    auto obj = parseObject(R"({"val":42})", ok);
    assert(ok);
    assert(intField(obj, "val") == 42);
}

void assertIntFieldMissingUsesFallback() {
    bool ok = false;
    auto obj = parseObject(R"({})", ok);
    assert(ok);
    assert(intField(obj, "val") == 0);
    assert(intField(obj, "val", -1) == -1);
}

void assertIntFieldStringReturnsFallback() {
    bool ok = false;
    auto obj = parseObject(R"({"val":"not_a_number"})", ok);
    assert(ok);
    assert(intField(obj, "val", 99) == 99);
}

void assertInt64Field() {
    bool ok = false;
    auto obj = parseObject(R"({"msg_id":9876543210})", ok);
    assert(ok);
    assert(int64Field(obj, "msg_id") == 9876543210LL);
    assert(int64Field(obj, "missing", -1) == -1);
}

void assertStringFieldPresent() {
    bool ok = false;
    auto obj = parseObject(R"({"name":"hello"})", ok);
    assert(ok);
    assert(stringField(obj, "name") == "hello");
}

void assertStringFieldMissingUsesFallback() {
    bool ok = false;
    auto obj = parseObject(R"({})", ok);
    assert(ok);
    assert(stringField(obj, "name") == "");
    assert(stringField(obj, "name", "default") == "default");
}

void assertTypeFieldValid() {
    bool ok = false;
    auto obj = parseObject(R"({"type":20})", ok);
    assert(ok);
    assert(typeField(obj) == MsgType::Heartbeat);
}

void assertTypeFieldMissing() {
    bool ok = false;
    auto obj = parseObject(R"({})", ok);
    assert(ok);
    assert(static_cast<int>(typeField(obj)) == -1);
}

#include "mini_json.hpp"

void assertSerializeRoundTrip() {
    lanchat::vendor::json::Object obj;
    obj["type"] = lanchat::vendor::json::Value(lanchat::protocol::toInt(MsgType::Heartbeat));
    obj["id"] = lanchat::vendor::json::Value(42);
    auto json = serialize(obj);

    bool ok = false;
    auto parsed = parseObject(json, ok);
    assert(ok);
    assert(intField(parsed, "type") == 20);
    assert(intField(parsed, "id") == 42);
}

void assertOkResponse() {
    auto json = ok(MsgType::HeartbeatAck);
    bool ok_flag = false;
    auto obj = parseObject(json, ok_flag);
    assert(ok_flag);
    assert(intField(obj, "type") == lanchat::protocol::toInt(MsgType::HeartbeatAck));
    assert(stringField(obj, "status") == "ok");
    assert(stringField(obj, "msg") == "ok");
}

void assertOkResponseWithCustomMessage() {
    auto json = ok(MsgType::SystemBroadcast, "all good");
    bool ok_flag = false;
    auto obj = parseObject(json, ok_flag);
    assert(ok_flag);
    assert(stringField(obj, "msg") == "all good");
}

void assertErrorResponse() {
    auto json = error(MsgType::LoginFailedReturn, 401, "invalid credentials");
    bool ok_flag = false;
    auto obj = parseObject(json, ok_flag);
    assert(ok_flag);
    assert(intField(obj, "type") == lanchat::protocol::toInt(MsgType::LoginFailedReturn));
    assert(stringField(obj, "status") == "error");
    assert(intField(obj, "code") == 401);
    assert(stringField(obj, "msg") == "invalid credentials");
}

} // namespace

int main() {
    assertParseValidObject();
    assertParseInvalidJson();
    assertParseArrayReturnsInvalid();
    assertParseEmptyString();
    assertHasExistingKey();
    assertHasMissingKey();
    assertIntFieldPresent();
    assertIntFieldMissingUsesFallback();
    assertIntFieldStringReturnsFallback();
    assertInt64Field();
    assertStringFieldPresent();
    assertStringFieldMissingUsesFallback();
    assertTypeFieldValid();
    assertTypeFieldMissing();
    assertSerializeRoundTrip();
    assertOkResponse();
    assertOkResponseWithCustomMessage();
    assertErrorResponse();
    std::cout << "ProtocolJson tests passed\n";
    return 0;
}
