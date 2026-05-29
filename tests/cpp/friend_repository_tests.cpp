#include "lanchat/server/db/Database.h"
#include "lanchat/server/db/FriendRepository.h"
#include "lanchat/server/db/UserRepository.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>

using lanchat::server::db::Database;
using lanchat::server::db::FriendRepository;
using lanchat::server::db::UserRepository;

namespace {

std::string tempDbDir()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() / ("lanchat_friends_" + std::to_string(stamp))).string();
}

void assertFriendLifecycle()
{
    const auto dir = tempDbDir();
    Database db(dir);
    assert(db.open());
    UserRepository users(db);
    const int alice = users.create("secret", "alice", 1);
    const int bob = users.create("secret", "bob", 2);
    FriendRepository friends(db, users);

    assert(friends.sendRequest(alice, bob, "hi"));
    assert(!friends.areFriends(alice, bob));
    auto pending = friends.pendingRequests(bob);
    assert(pending.size() == 1);
    assert(pending[0].from_uid == alice);

    assert(friends.acceptRequest(alice, bob));
    assert(friends.areFriends(alice, bob));
    auto bobFriends = friends.getFriends(bob);
    assert(bobFriends.size() == 1);
    assert(bobFriends[0].id == alice);

    assert(friends.removeFriendship(alice, bob));
    assert(!friends.areFriends(alice, bob));

    db.close();
    std::filesystem::remove_all(dir);
}

void assertRejectedRequestBlocksImmediateRetry()
{
    const auto dir = tempDbDir();
    Database db(dir);
    assert(db.open());
    UserRepository users(db);
    const int alice = users.create("secret", "alice", 1);
    const int bob = users.create("secret", "bob", 2);
    FriendRepository friends(db, users);

    assert(friends.sendRequest(alice, bob, "hi"));
    assert(friends.rejectRequest(alice, bob));
    assert(friends.pendingRequests(bob).empty());
    assert(!friends.areFriends(alice, bob));
    assert(!friends.sendRequest(alice, bob, "retry too soon"));

    db.close();
    std::filesystem::remove_all(dir);
}

} // namespace

int main()
{
    assertFriendLifecycle();
    assertRejectedRequestBlocksImmediateRetry();
    std::cout << "FriendRepository tests passed\n";
    return 0;
}
