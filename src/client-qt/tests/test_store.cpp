// test_store.cpp —— Store 模型 + 本地搜索单测。
#include "model/store.h"
#include "test_assert.h"

#include <QCoreApplication>
#include <iostream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Store s;
    s.setSelfId(1);

    // upsert 去重：同 id 两次只留一条，昵称更新
    s.upsertContact(1, "alice", false);
    s.upsertContact(1, "alice2", true);
    s.upsertContact(2, "bob", false);
    assert(s.contacts().size() == 2);

    // setOnline 更新
    s.setOnline(2, true);
    bool bobOnline = false;
    for (const auto& c : s.contacts()) if (c.id == 2) bobOnline = c.online;
    assert(bobOnline);

    // addMessage 归到正确会话
    Message m1; m1.fromId = 2; m1.toId = 1; m1.text = "Hello World"; m1.kind = "direct";
    Message m2; m2.fromId = 1; m2.toId = 2; m2.text = "reply here"; m2.kind = "direct";
    Message g1; g1.fromId = 3; g1.groupId = 9; g1.text = "group talk"; g1.kind = "group";
    s.addMessage(m1); s.addMessage(m2); s.addMessage(g1);
    assert(s.conversation(Store::directKey(2)).size() == 2);
    assert(s.conversation(Store::groupKey(9)).size() == 1);
    assert(s.conversation(Store::directKey(99)).isEmpty());

    // 搜索大小写不敏感 + 不命中无关词
    assert(s.search("hello").size() == 1);
    assert(s.search("HELLO").size() == 1);
    assert(s.search("group").size() == 1);
    assert(s.search("zzz_not_present").isEmpty());
    assert(s.search("   ").isEmpty());

    std::cout << "store tests passed\n";
    return 0;
}