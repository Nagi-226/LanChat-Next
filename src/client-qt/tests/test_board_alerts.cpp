#include "ui/main_window.h"
#include "ui/contacts_panel.h"
#include "ui/chat_view.h"
#include "ui/board_connection_panel.h"
#include "ui/theme.h"
#include "app/devmode.h"
#include "net/session.h"
#include "model/store.h"
#include "test_assert.h"
#include <QApplication>
#include <QJsonArray>
#include <QLabel>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QDialog>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QScrollArea>
#include <QScrollBar>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    app.setStyleSheet(theme::stylesheet());
    QTemporaryDir dir;
    app.setOrganizationName("LanChat-Qt-Test");
    app.setApplicationName("BoardAlertsTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    QSettings().setValue("board_account_id", 77);
    Session session;
    Store store;
    DevMode dev;
    MainWindow window(&session, &store, &dev);
    window.show();
    auto* pages=window.findChild<QStackedWidget*>("mainPages");
    auto* boardButton=window.findChild<QPushButton*>("boardConnectionButton");
    assert(pages&&boardButton);
    QWidget* loginPage=pages->currentWidget();
    QTest::mouseClick(boardButton,Qt::LeftButton);
    auto* boardPage=window.findChild<BoardConnectionPanel*>();
    assert(boardPage&&pages->currentWidget()==boardPage&&!boardPage->isWindow()&&boardPage->window()==&window);
    assert(window.findChildren<QDialog*>().isEmpty());
    auto* draft=boardPage->findChild<QPlainTextEdit*>("boardMessageText");
    auto* returnButton=boardPage->findChild<QPushButton*>("boardReturnToLanChat");
    auto* scroll=boardPage->findChild<QScrollArea*>();
    assert(draft&&returnButton&&scroll);
    draft->setPlainText("板端未发送草稿");
    QCoreApplication::processEvents();scroll->verticalScrollBar()->setValue(120);
    const int savedScroll=scroll->verticalScrollBar()->value();
    QTest::mouseClick(returnButton,Qt::LeftButton);assert(pages->currentWidget()==loginPage);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    assert(window.findChildren<BoardConnectionPanel*>().size()==1);
    assert(draft->toPlainText()=="板端未发送草稿"&&scroll->verticalScrollBar()->value()==savedScroll);
    emit session.ready(99, "Tester");
    assert(pages->currentWidget()==boardPage);
    QTest::mouseClick(returnButton,Qt::LeftButton);assert(pages->currentWidget()==pages->widget(2));
    emit session.usersReceived({{"users", QJsonArray{QJsonObject{{"id", 77}, {"nickname", "Board"}}}},
                               {"groups", QJsonArray{QJsonObject{{"groupId", 9}, {"name", "Alerts"}}}}});
    auto* panel = window.findChild<ContactsPanel*>();
    assert(panel);
    QSignalSpy selected(panel, &ContactsPanel::alertSelected);
    const QString body = "PPM 疲劳告警：drowsy_level=2";
    emit session.messageReceived({{"fromId", 77}, {"groupId", 9}, {"msg", body}, {"type", 17}});
    QCoreApplication::processEvents();
    assert(store.conversation(Store::groupKey(9)).size() == 1);
    QWidget* alert = nullptr;
    for (auto* widget : panel->findChildren<QWidget*>())
        if (widget->property("rowIsAlert").toBool()) alert = widget;
    assert(alert && alert->property("rowIsGroup").toBool());
    QTest::mouseClick(alert, Qt::LeftButton);
    assert(selected.size() == 1);
    assert(selected.first().at(0).toInt() == 9 && selected.first().at(1).toBool());
    bool groupOpened = false;
    for (auto* label : window.findChild<ChatView*>()->findChildren<QLabel*>())
        if (label->text() == "Alerts · 群 ID 9") groupOpened = true;
    assert(groupOpened);
    emit session.messageReceived({{"fromId", 77}, {"toId", 99}, {"msg", body}, {"type", 6}});
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    assert(store.conversation(Store::directKey(77)).size() == 1);
    int alerts = 0;
    for (auto* widget : panel->findChildren<QWidget*>())
        if (widget->property("rowIsAlert").toBool()) ++alerts;
    assert(alerts == 2);
    emit session.messageReceived({{"fromId", 88}, {"groupId", 9}, {"msg", body}, {"type", 17}});
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    int afterOtherSender = 0;
    for (auto* widget : panel->findChildren<QWidget*>())
        if (widget->property("rowIsAlert").toBool()) ++afterOtherSender;
    assert(afterOtherSender == 2);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    emit session.messageReceived({{"fromId",88},{"toId",99},{"msg","页面切换期间继续收消息"},{"type",6}});
    assert(store.conversation(Store::directKey(88)).size()==1&&pages->currentWidget()==boardPage);
    for(auto* topLevel:QApplication::topLevelWidgets())if(topLevel->isVisible())assert(topLevel==&window);
    const int capture = app.arguments().indexOf("--capture-board");
    if (capture > 0 && capture + 1 < app.arguments().size()) {
        window.showBoardConnection();
        QCoreApplication::processEvents();
        assert(window.grab().save(app.arguments().at(capture + 1)));
    }
    std::printf("BOARD ALERTS PASS: group/direct cards, embedded page, no second window, singleton/draft/scroll, login/chat return, receive while on board page\n");
    return 0;
}
