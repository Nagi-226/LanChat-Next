// theme.h —— 暗色主题 QSS（token 化，单点改色）。
// 色板对齐现 React 版观感：bg #151A23 / panel #1B212C / accent #E8465A /
// text #E6E9F0 / muted #8B93A7 / online #35C46A。
// 说明：原计划 vendored QDarkStyleSheet(MIT)，但其 qss 为需 Python 处理变量的模板，
// 直接引入不可用；改为自研 QSS（同等暗色观感），无第三方代码拷贝。
#pragma once

#include <QString>

namespace theme {
QString stylesheet();
}