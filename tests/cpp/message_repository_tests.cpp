#include "lanchat/server/db/Database.h"
#include "lanchat/server/db/MessageRepository.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>

using lanchat::server::db::Database;
using lanchat::server::db::MessageRepository;

namespace {

std::string tempDbDir()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() / ("lanchat_messages_" + std::to_string(stamp))).string();
}

void assertEditDeleteReactionAndReadLifecycle()
{
    const auto dir = tempDbDir();
    Database db(dir);
    assert(db.open());
    MessageRepository messages(db);

    const auto saved = messages.savePrivate(1, 2, "before", "text", true);
    assert(messages.edit(saved.msg_id, 1, "after"));
    auto history = messages.privateHistory(1, 2);
    assert(history.size() == 1);
    assert(history[0].content == "after");
    assert(history[0].edited);
    assert(!messages.edit(saved.msg_id, 2, "not owner"));

    assert(messages.addReaction(saved.msg_id, 2, "👍"));
    history = messages.privateHistory(1, 2);
    assert(history[0].reactions.find("2:👍") != std::string::npos);

    assert(messages.markRead(saved.msg_id, 2));
    history = messages.privateHistory(1, 2);
    assert(history[0].read);

    assert(messages.softDelete(saved.msg_id, 1));
    history = messages.privateHistory(1, 2);
    assert(history[0].deleted);
    assert(history[0].content.empty());

    db.close();
    std::filesystem::remove_all(dir);
}

} // namespace

int main()
{
    assertEditDeleteReactionAndReadLifecycle();
    std::cout << "MessageRepository tests passed\n";
    return 0;
}
