// test_assert.h —— 测试断言宏：无论 Debug/Release 都真正求值。
// 标准 assert 在 Release（NDEBUG）下会被整条编译掉——若断言里带副作用
// （如 spy.wait()、fb.next()），Release 下副作用不执行，测试"假绿"甚至空指针崩溃。
// 故测试统一用本宏：失败打印位置并返回 1，成功继续；副作用永远执行。
#pragma once

#include <cassert>
#include <cstdio>

#undef assert
#define assert(x)                                                          \
    do {                                                                   \
        if (!(x)) {                                                        \
            std::fprintf(stderr, "CHECK FAILED %s:%d: %s\n",               \
                         __FILE__, __LINE__, #x);                          \
            return 1;                                                      \
        }                                                                  \
    } while (0)