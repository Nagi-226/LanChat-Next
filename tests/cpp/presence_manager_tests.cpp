#include "lanchat/server/PresenceManager.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using lanchat::server::PresenceManager;

namespace {

void assertInitialStateOffline() {
    PresenceManager pm;
    assert(!pm.isOnline(42));
    assert(pm.status(42).empty() || pm.status(42) == "offline" ||
           pm.status(42) == "");
}

void assertSetOnlineDefaultStatus() {
    PresenceManager pm;
    pm.setOnline(1);
    assert(pm.isOnline(1));
    assert(pm.status(1) == "online");
}

void assertSetOnlineCustomStatus() {
    PresenceManager pm;
    pm.setOnline(1, "away");
    assert(pm.status(1) == "away");
}

void assertSetOnlineZeroUserId() {
    PresenceManager pm;
    pm.setOnline(0);
    assert(!pm.isOnline(0));
}

void assertSetOnlineNegativeUserId() {
    PresenceManager pm;
    pm.setOnline(-1);
    assert(!pm.isOnline(-1));
}

void assertSetOffline() {
    PresenceManager pm;
    pm.setOnline(1);
    assert(pm.isOnline(1));
    pm.setOffline(1);
    assert(!pm.isOnline(1));
}

void assertSetOfflineNeverOnline() {
    PresenceManager pm;
    pm.setOffline(999);
    assert(!pm.isOnline(999));
}

void assertOnlineUsersEmptyInitially() {
    PresenceManager pm;
    assert(pm.onlineUsers().empty());
}

void assertOnlineUsersMultiple() {
    PresenceManager pm;
    pm.setOnline(1);
    pm.setOnline(2);
    pm.setOnline(3);
    auto users = pm.onlineUsers();
    assert(users.size() == 3);
    assert(std::find(users.begin(), users.end(), 1) != users.end());
    assert(std::find(users.begin(), users.end(), 2) != users.end());
    assert(std::find(users.begin(), users.end(), 3) != users.end());
}

void assertOnlineUsersAfterOffline() {
    PresenceManager pm;
    pm.setOnline(1);
    pm.setOnline(2);
    pm.setOffline(1);
    auto users = pm.onlineUsers();
    assert(users.size() == 1);
    assert(users[0] == 2);
}

void assertStatusAfterOffline() {
    PresenceManager pm;
    pm.setOnline(1);
    pm.setOffline(1);
    assert(pm.status(1) == "offline");
}

void assertStatusUnknownUser() {
    PresenceManager pm;
    auto s = pm.status(999);
    assert(s.empty() || s == "offline" || s == "");
}

} // namespace

int main() {
    assertInitialStateOffline();
    assertSetOnlineDefaultStatus();
    assertSetOnlineCustomStatus();
    assertSetOnlineZeroUserId();
    assertSetOnlineNegativeUserId();
    assertSetOffline();
    assertSetOfflineNeverOnline();
    assertOnlineUsersEmptyInitially();
    assertOnlineUsersMultiple();
    assertOnlineUsersAfterOffline();
    assertStatusAfterOffline();
    assertStatusUnknownUser();
    std::cout << "PresenceManager tests passed\n";
    return 0;
}
