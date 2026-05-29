#include "lanchat/server/db/FriendRepository.h"

#include <sqlite3.h>

#include <ctime>

namespace lanchat::server::db {

namespace {

bool exec(sqlite3* db, const std::string& sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    sqlite3_free(err);
    return rc == SQLITE_OK;
}

void bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

} // namespace

FriendRepository::FriendRepository(Database& db, UserRepository& users)
    : db_(db), users_(users) {
    db_.migrate(1);
}

bool FriendRepository::sendRequest(int fromUid, int toUid, const std::string& requestMsg) {
    if (fromUid <= 0 || toUid <= 0 || fromUid == toUid) return false;
    const auto now = static_cast<int>(std::time(nullptr));

    sqlite3_stmt* existing = nullptr;
    const char* selectSql =
        "SELECT from_uid,to_uid,status FROM friendships WHERE "
        "(from_uid=? AND to_uid=?) OR (from_uid=? AND to_uid=?) LIMIT 1;";
    if (sqlite3_prepare_v2(db_.nativeHandle(), selectSql, -1, &existing, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(existing, 1, fromUid);
    sqlite3_bind_int(existing, 2, toUid);
    sqlite3_bind_int(existing, 3, toUid);
    sqlite3_bind_int(existing, 4, fromUid);
    if (sqlite3_step(existing) == SQLITE_ROW) {
        const int existingFrom = sqlite3_column_int(existing, 0);
        const int existingTo = sqlite3_column_int(existing, 1);
        const auto* statusText = sqlite3_column_text(existing, 2);
        const std::string status = statusText ? reinterpret_cast<const char*>(statusText) : "";
        sqlite3_finalize(existing);
        existing = nullptr;

        if (status == "accepted") {
            return false;
        }
        if (status == "rejected" && existingFrom == fromUid && existingTo == toUid) {
            return false;
        }
        if (status == "rejected") {
            // A rejection blocks the rejected requester, but it should not prevent
            // the other user from initiating a fresh request later.
        } else if (status != "pending" || existingFrom != fromUid || existingTo != toUid) {
            return false;
        }

        if (status == "rejected") {
            // Continue below and create a request in the opposite direction.
        } else {
            sqlite3_stmt* update = nullptr;
            const char* updateSql =
                "UPDATE friendships SET request_msg=?, updated_at=? "
                "WHERE from_uid=? AND to_uid=? AND status='pending';";
            if (sqlite3_prepare_v2(db_.nativeHandle(), updateSql, -1, &update, nullptr) != SQLITE_OK) return false;
            bindText(update, 1, requestMsg);
            sqlite3_bind_int(update, 2, now);
            sqlite3_bind_int(update, 3, fromUid);
            sqlite3_bind_int(update, 4, toUid);
            const bool ok = sqlite3_step(update) == SQLITE_DONE && sqlite3_changes(db_.nativeHandle()) > 0;
            sqlite3_finalize(update);
            return ok;
        }
    }
    if (existing) sqlite3_finalize(existing);

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO friendships(from_uid,to_uid,status,request_msg,created_at,updated_at) "
        "VALUES(?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, fromUid);
    sqlite3_bind_int(stmt, 2, toUid);
    bindText(stmt, 3, "pending");
    bindText(stmt, 4, requestMsg);
    sqlite3_bind_int(stmt, 5, now);
    sqlite3_bind_int(stmt, 6, now);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_.nativeHandle()) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

bool FriendRepository::acceptRequest(int fromUid, int toUid) {
    return setStatus(fromUid, toUid, "accepted");
}

bool FriendRepository::rejectRequest(int fromUid, int toUid) {
    return setStatus(fromUid, toUid, "rejected");
}

bool FriendRepository::removeFriendship(int uidA, int uidB) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "DELETE FROM friendships WHERE "
        "(from_uid=? AND to_uid=?) OR (from_uid=? AND to_uid=?);";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, uidA);
    sqlite3_bind_int(stmt, 2, uidB);
    sqlite3_bind_int(stmt, 3, uidB);
    sqlite3_bind_int(stmt, 4, uidA);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_.nativeHandle()) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

bool FriendRepository::areFriends(int uidA, int uidB) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT 1 FROM friendships WHERE status='accepted' AND "
        "((from_uid=? AND to_uid=?) OR (from_uid=? AND to_uid=?)) LIMIT 1;";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, uidA);
    sqlite3_bind_int(stmt, 2, uidB);
    sqlite3_bind_int(stmt, 3, uidB);
    sqlite3_bind_int(stmt, 4, uidA);
    const bool found = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return found;
}

std::vector<protocol::UserInfo> FriendRepository::getFriends(int uid) {
    std::vector<protocol::UserInfo> result;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT CASE WHEN from_uid=? THEN to_uid ELSE from_uid END AS friend_id "
        "FROM friendships WHERE status='accepted' AND (from_uid=? OR to_uid=?);";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, uid);
    sqlite3_bind_int(stmt, 2, uid);
    sqlite3_bind_int(stmt, 3, uid);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const int friendId = sqlite3_column_int(stmt, 0);
        auto user = users_.findById(friendId);
        if (user) result.push_back(*user);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<FriendRequestRecord> FriendRepository::pendingRequests(int uid) {
    std::vector<FriendRequestRecord> result;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT from_uid,to_uid,status,request_msg FROM friendships "
        "WHERE to_uid=? AND status='pending';";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_int(stmt, 1, uid);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FriendRequestRecord row;
        row.from_uid = sqlite3_column_int(stmt, 0);
        row.to_uid = sqlite3_column_int(stmt, 1);
        row.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const auto* msg = sqlite3_column_text(stmt, 3);
        row.request_msg = msg ? reinterpret_cast<const char*>(msg) : "";
        result.push_back(row);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool FriendRepository::setStatus(int fromUid, int toUid, const std::string& status) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE friendships SET status=?, updated_at=? "
        "WHERE from_uid=? AND to_uid=? AND status='pending';";
    if (sqlite3_prepare_v2(db_.nativeHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    bindText(stmt, 1, status);
    sqlite3_bind_int(stmt, 2, static_cast<int>(std::time(nullptr)));
    sqlite3_bind_int(stmt, 3, fromUid);
    sqlite3_bind_int(stmt, 4, toUid);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_.nativeHandle()) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

} // namespace lanchat::server::db
