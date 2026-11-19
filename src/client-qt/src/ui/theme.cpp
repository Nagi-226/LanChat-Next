#include "ui/theme.h"

namespace theme {

QString stylesheet()
{
    // token 集中在此，改色只改这里
    const QString bg = "#151A23";
    const QString panel = "#1B212C";
    const QString accent = "#E8465A";
    const QString text = "#E6E9F0";
    const QString muted = "#8B93A7";
    const QString online = "#35C46A";
    const QString border = "#2A3140";

    return QStringLiteral(
        "QWidget { background: %1; color: %2; font-family: 'Segoe UI','Microsoft YaHei',sans-serif; font-size: 14px; }"
        "QMainWindow, QDialog { background: %1; }"
        "QLineEdit, QTextEdit, QPlainTextEdit { background: %3; border: 1px solid %7; border-radius: 6px; padding: 6px 8px; color: %2; selection-background-color: %4; }"
        "QLineEdit:focus, QTextEdit:focus { border: 1px solid %4; }"
        "QPushButton { background: %3; border: 1px solid %7; border-radius: 6px; padding: 6px 14px; color: %2; }"
        "QPushButton:hover { background: %7; }"
        "QPushButton:pressed { background: %4; color: #ffffff; }"
        "QPushButton#primary { background: %4; border: none; color: #ffffff; font-weight: 600; }"
        "QPushButton#primary:hover { background: #f0566c; }"
        "QPushButton:flat { border: none; background: transparent; color: %5; }"
        "QListWidget, QListView, QTreeWidget, QTableView { background: %3; border: 1px solid %7; border-radius: 6px; outline: none; }"
        "QListWidget::item, QListView::item { padding: 8px; border-radius: 4px; }"
        "QListWidget::item:selected, QListView::item:selected { background: %4; color: #ffffff; }"
        "QListWidget::item:hover, QListView::item:hover { background: %7; }"
        "QScrollBar:vertical { background: %1; width: 10px; }"
        "QScrollBar::handle:vertical { background: %7; border-radius: 5px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QLabel#muted { color: %5; }"
        "QLabel#err { color: %4; }"
        "QLabel#online { color: %6; }"
        "QLabel#title { font-size: 16px; font-weight: 600; }"
        "QTabWidget::pane { border: 1px solid %7; border-radius: 6px; }"
        "QTabBar::tab { background: %3; padding: 6px 12px; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
        "QTabBar::tab:selected { background: %4; color: #ffffff; }"
        "QComboBox { background: %3; border: 1px solid %7; border-radius: 6px; padding: 4px 8px; }"
        "QComboBox QAbstractItemView { background: %3; selection-background-color: %4; }"
        "QToolTip { background: %3; color: %2; border: 1px solid %7; }"
    )
        .arg(bg, text, panel, accent, muted, online, border);
}

} // namespace theme