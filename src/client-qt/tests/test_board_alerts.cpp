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
#include <QLineEdit>
#include <QDir>

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
    QSettings().setValue("board_bridge/account_id",77);
    Session session;
    Store store;
    DevMode dev;
    MainWindow window(&session, &store, &dev);
    window.show();
    auto* pages=window.findChild<QStackedWidget*>("mainPages");
    auto* settingsButton=window.findChild<QPushButton*>("settingsButton");
    assert(pages&&settingsButton&&!settingsButton->isVisible()&&!settingsButton->isEnabled());
    QWidget* loginPage=pages->currentWidget();
    const auto args=app.arguments();QString captureFolder;if(args.contains("--capture-dir")){captureFolder=args.value(args.indexOf("--capture-dir")+1);QDir().mkpath(captureFolder);QTest::qWait(50);assert(window.grab().save(captureFolder+"/login.png"));}
    window.showBoardConnection();
    assert(pages->currentWidget()==loginPage&&!window.findChild<BoardConnectionPanel*>()&&!window.findChild<QPushButton*>("serverSettingsButton"));
    emit session.ready(99,"Tester");QCoreApplication::processEvents();
    assert(settingsButton->isVisible()&&settingsButton->isEnabled());
    assert(settingsButton->mapTo(&window,QPoint()).x()<50&&settingsButton->mapTo(&window,QPoint()).y()>window.height()-100);
    QTest::mouseClick(settingsButton,Qt::LeftButton);
    auto* settingsPage=window.findChild<QWidget*>("settingsPage");
    auto* contents=window.findChild<QStackedWidget*>("settingsContents");
    auto* boardButton=window.findChild<QPushButton*>("boardConnectionButton");
    auto* serverButton=window.findChild<QPushButton*>("serverSettingsButton");
    assert(settingsPage&&contents&&boardButton&&serverButton&&pages->currentWidget()==settingsPage);
    assert(serverButton->parentWidget()!=window.findChild<QWidget*>("windowTitleBar"));
    if(!captureFolder.isEmpty()){QTest::qWait(50);assert(window.grab().save(captureFolder+"/settings.png"));}
    QTest::mouseClick(boardButton,Qt::LeftButton);
    auto* boardPage=window.findChild<BoardConnectionPanel*>();
    assert(boardPage&&contents->currentWidget()==boardPage&&!boardPage->isWindow()&&boardPage->window()==&window);
    assert(window.findChildren<QDialog*>().isEmpty());
    auto* aiKey=boardPage->findChild<QLineEdit*>("boardAiKey");
    auto* aiChat=boardPage->findChild<QPushButton*>("boardAiChat");
    assert(aiKey&&aiKey->echoMode()==QLineEdit::Password&&aiKey->text().isEmpty()&&aiChat);
    QSignalSpy aiNavigation(boardPage,&BoardConnectionPanel::aiChatRequested);
    QTest::mouseClick(aiChat,Qt::LeftButton);assert(aiNavigation.count()==1&&aiNavigation.at(0).at(0).toInt()==77&&pages->currentWidget()==pages->widget(2));
    QTest::mouseClick(settingsButton,Qt::LeftButton);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    auto* draft=boardPage->findChild<QPlainTextEdit*>("boardMessageText");
    auto* returnButton=boardPage->findChild<QPushButton*>("boardReturnToLanChat");
    auto* scroll=boardPage->findChild<QScrollArea*>();
    assert(draft&&returnButton&&scroll);
    draft->setPlainText("板端未发送草稿");
    QCoreApplication::processEvents();scroll->verticalScrollBar()->setValue(120);
    const int savedScroll=scroll->verticalScrollBar()->value();
    QTest::mouseClick(returnButton,Qt::LeftButton);assert(pages->currentWidget()==pages->widget(2));
    QTest::mouseClick(settingsButton,Qt::LeftButton);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    QTest::mouseClick(boardButton,Qt::LeftButton);
    assert(window.findChildren<BoardConnectionPanel*>().size()==1);
    assert(draft->toPlainText()=="板端未发送草稿"&&scroll->verticalScrollBar()->value()==savedScroll);
    QTest::mouseClick(serverButton,Qt::LeftButton);assert(contents->currentWidget()!=boardPage);
    QTest::mouseClick(boardButton,Qt::LeftButton);assert(contents->currentWidget()==boardPage&&draft->toPlainText()=="板端未发送草稿");
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
    assert(store.conversation(Store::directKey(88)).size()==1&&contents->currentWidget()==boardPage&&pages->currentWidget()==settingsPage);
    for(auto* topLevel:QApplication::topLevelWidgets())if(topLevel->isVisible())assert(topLevel==&window);
    const int capture = app.arguments().indexOf("--capture-board");
    if (capture > 0 && capture + 1 < app.arguments().size()) {
        window.showBoardConnection();
        QCoreApplication::processEvents();
        assert(window.grab().save(app.arguments().at(capture + 1)));
    }
    session.logout();assert(pages->currentWidget()==loginPage&&!settingsButton->isVisible()&&!settingsButton->isEnabled());
    window.showBoardConnection();boardButton->click();serverButton->click();assert(pages->currentWidget()==loginPage);
    auto* address=loginPage->findChild<QLineEdit*>("loginServerAddress");auto* connectButton=loginPage->findChild<QPushButton*>("loginConnectButton");assert(address&&connectButton);
    QSignalSpy connection(&window,&MainWindow::connectionRequested);
    address->setText("user:password@host:12346");QTest::mouseClick(connectButton,Qt::LeftButton);assert(connection.isEmpty());
    address->setText("100.64.0.8:12346");QTest::mouseClick(connectButton,Qt::LeftButton);assert(connection.size()==1&&connection.first().at(0).toString()=="100.64.0.8"&&!connection.first().at(2).toBool());
    assert(QSettings().value("board_bridge/account_id").toInt()==0&&pages->currentWidget()==loginPage);
    std::printf("BOARD ALERTS PASS: login-only settings, compact connection bootstrap, embedded board/AI/draft/scroll, original conversation and receive while in settings\n");
    return 0;
}
