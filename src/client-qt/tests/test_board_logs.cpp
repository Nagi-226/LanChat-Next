#include "app/board_connection.h"
#include "ui/board_connection_panel.h"
#include "ui/theme.h"
#include "net/session.h"
#include <QTcpServer>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QDir>
#include <QFontDatabase>
#include <QTextEdit>
#include <QFrame>
#include <QScrollBar>
#include <QScrollArea>
#include <QTextCursor>
#include <QWheelEvent>
#include <functional>
#include <cstdio>
#include <cstdlib>
static std::function<void()> cleanup;
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);if(cleanup)cleanup();std::exit(1);} }while(false)
static bool waitFor(const std::function<bool()>& check,int ms=4000){QElapsedTimer timer;timer.start();while(timer.elapsed()<ms){QCoreApplication::processEvents();if(check())return true;QTest::qWait(20);}return false;}
static void save(const QString& folder,QJsonArray rows){QDir().mkpath(folder+"/logs");QSaveFile file(folder+"/logs/logs.snapshot.json");CHECK(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"status",QJsonObject{{"state","running"},{"count",rows.size()},{"sequence",rows.last().toObject().value("seq")},{"updated_ms",QDateTime::currentMSecsSinceEpoch()},{"last_ms",QDateTime::currentMSecsSinceEpoch()}}},{"rows",rows}}).toJson());CHECK(file.commit());}
int main(int argc,char** argv){QApplication app(argc,argv);QTemporaryDir folder;CHECK(folder.isValid());app.setOrganizationName("LanChatIsolatedLogsTest");app.setApplicationName("Logs");QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,folder.path());app.setStyleSheet(theme::stylesheet());
    // Windows 的 offscreen 插件不枚举系统字体；原生窗口无此限制。
    if(qEnvironmentVariable("QT_QPA_PLATFORM")=="offscreen"){QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");QFontDatabase::addApplicationFont("C:/Windows/Fonts/consola.ttf");}
    const auto args=app.arguments();auto value=[&](const QString& key){return args.value(args.indexOf(key)+1);};
    if(args.contains("--live")){
        BoardConnection bridge(nullptr,args.contains("--profile")?value("--profile"):folder.path());auto config=bridge.config();config.consolePath=value("--console");config.serverPort=value("--server-port").toUShort();config.accountId=value("--board-id").toInt();config.password="isolated-test-password";config.logEnabled=true;config.logPort=value("--port");
        QTcpServer probe;CHECK(probe.listen(QHostAddress::LocalHost,0));config.listenPort=probe.serverPort();probe.close();bridge.configure(config);bridge.start();
        cleanup=[&]{bridge.stop();};
        CHECK(waitFor([&]{for(const auto& value:bridge.logSnapshot().value("rows").toArray())if(!value.toObject().value("wire").toBool())return true;return false;},30000));
        Session session;session.connectToServer("127.0.0.1",config.serverPort);session.login("LogOperator","isolated-test-password");CHECK(waitFor([&]{return session.selfId()>0;}));
        BoardConnectionPanel panel(&bridge,nullptr,&session);panel.resize(960,840);panel.show();panel.findChild<QTabWidget*>("boardConnectionTabs")->setCurrentIndex(1);
        auto* text=panel.findChild<QTextEdit*>("boardLogText");CHECK(text->toPlainText().contains("producer:")||text->toPlainText().contains("terminal"));
        auto* command=panel.findChild<QLineEdit*>("boardLogCommand");command->setText("/日志");QTest::keyClick(command,Qt::Key_Return);
        auto* replies=panel.findChild<QPlainTextEdit*>("boardLogReplies");CHECK(waitFor([&]{return replies->toPlainText().contains("[S3 · 日志");},8000));CHECK(replies->toPlainText().count("你：/日志")==1);CHECK(replies->toPlainText().count("[S3 · 日志")==1);
        if(args.contains("--capture"))CHECK(panel.grab().save(value("--capture")));
        QTest::mouseClick(panel.findChild<QPushButton*>("boardLogStop"),Qt::LeftButton);CHECK(waitFor([&]{return bridge.logSnapshot().value("status").toObject().value("state").toString()=="stopped";}));
        session.logout();bridge.stop();cleanup={};std::puts("PASS live Qt log tab: actual USB -> Console -> file -> panel; real Session query/echo/reply once; stop releases source");return 0;
    }
    QJsonArray rows;auto row=[&](int seq,const QString& level,const QString& module,const QString& text,bool wire=false){return QJsonObject{{"seq",seq},{"received_ms",QDateTime::currentMSecsSinceEpoch()},{"level",level},{"module",module},{"text",text},{"wire",wire}};};
    rows.append(row(1,"I","producer","frames=100"));rows.append(row(2,"E","camera","中文错误 "+QString(1800,QChar('x'))));rows.append(row(3,"RAW","wire","hex=ffee",true));save(folder.path(),rows);
    BoardConnection bridge(nullptr,folder.path());BoardConnectionPanel panel(&bridge);panel.resize(960,840);panel.show();auto* tabs=panel.findChild<QTabWidget*>("boardConnectionTabs");CHECK(tabs&&tabs->count()==2);tabs->setCurrentIndex(1);
    auto* text=panel.findChild<QTextEdit*>("boardLogText");auto* level=panel.findChild<QComboBox*>("boardLogLevel");auto* search=panel.findChild<QLineEdit*>("boardLogSearch");auto* raw=panel.findChild<QCheckBox*>("boardLogRaw");CHECK(text&&level&&search&&raw);CHECK(text->toPlainText().contains("中文错误"));CHECK(!text->toPlainText().contains("hex="));
    level->setCurrentIndex(2);CHECK(text->toPlainText().contains("中文错误")&&!text->toPlainText().contains("frames="));level->setCurrentIndex(0);search->setText("camera");CHECK(!text->toPlainText().contains("frames="));search->clear();raw->setChecked(true);CHECK(text->toPlainText().contains("hex="));raw->setChecked(false);
    auto* pause=panel.findChild<QPushButton*>("boardLogPause");QTest::mouseClick(pause,Qt::LeftButton);rows.append(row(4,"I","wifi","latest-wifi"));save(folder.path(),rows);CHECK(waitFor([&]{return bridge.logSnapshot().value("status").toObject().value("sequence").toInt()==4;}));CHECK(!text->toPlainText().contains("latest-wifi"));QTest::mouseClick(pause,Qt::LeftButton);CHECK(text->toPlainText().contains("latest-wifi"));
    QTest::mouseClick(panel.findChild<QPushButton*>("boardLogClear"),Qt::LeftButton);CHECK(!text->toPlainText().contains("中文错误"));CHECK(bridge.logSnapshot().value("rows").toArray().size()==4);
    auto* follow=panel.findChild<QCheckBox*>("boardLogFollow");CHECK(follow&&follow->isChecked());
    auto* scrollbar=text->verticalScrollBar();auto* pageScroll=panel.findChild<QScrollArea*>();CHECK(pageScroll);
    auto latestVisible=[&]{QTextCursor end(text->document());end.movePosition(QTextCursor::End);const auto rect=text->cursorRect(end);return scrollbar->maximum()>0&&scrollbar->value()==scrollbar->maximum()&&text->viewport()->rect().contains(rect.center())&&pageScroll->viewport()->rect().contains(text->viewport()->mapTo(pageScroll->viewport(),rect.center()));};
    for(int seq=5;seq<=64;++seq)rows.append(row(seq,"I","producer",QString("stream-row-%1").arg(seq)));save(folder.path(),rows);
    CHECK(waitFor([&]{return text->toPlainText().contains("stream-row-64")&&latestVisible();}));
    auto wheel=[&](int delta){const auto pos=text->viewport()->rect().center();QWheelEvent event(QPointF(pos),QPointF(text->viewport()->mapToGlobal(pos)),QPoint(),QPoint(0,delta),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(text->viewport(),&event);};
    wheel(120);CHECK(waitFor([&]{return !follow->isChecked();}));const int historyScroll=scrollbar->value();
    rows.append(row(65,"I","wifi","follow-after-history"));save(folder.path(),rows);CHECK(waitFor([&]{return text->toPlainText().contains("follow-after-history");}));CHECK(scrollbar->value()==historyScroll);
    wheel(-12000);CHECK(waitFor([&]{return follow->isChecked()&&latestVisible();}));
    QTest::mouseClick(follow,Qt::LeftButton);scrollbar->setValue(0);CHECK(!follow->isChecked());rows.append(row(66,"I","wifi","follow-disabled"));save(folder.path(),rows);CHECK(waitFor([&]{return text->toPlainText().contains("follow-disabled");}));CHECK(scrollbar->value()==0&&!follow->isChecked());
    QTest::mouseClick(follow,Qt::LeftButton);CHECK(waitFor(latestVisible));
    QTest::mouseClick(pause,Qt::LeftButton);rows.append(row(67,"I","wifi","paused-follow"));save(folder.path(),rows);CHECK(waitFor([&]{return bridge.logSnapshot().value("status").toObject().value("sequence").toInt()==67;}));CHECK(!text->toPlainText().contains("paused-follow"));QTest::mouseClick(pause,Qt::LeftButton);CHECK(waitFor([&]{return text->toPlainText().contains("paused-follow")&&latestVisible();}));
    tabs->setCurrentIndex(0);rows.append(row(68,"I","wifi","hidden-tab-latest"));save(folder.path(),rows);CHECK(waitFor([&]{return text->toPlainText().contains("hidden-tab-latest");}));tabs->setCurrentIndex(1);CHECK(waitFor(latestVisible));
    while(rows.size()>4)rows.removeLast();save(folder.path(),rows);CHECK(waitFor([&]{return !text->toPlainText().contains("stream-row-64");}));
    if(args.contains("--capture")){
        const QList<QStringList> preview={{"W","wifi","WiFi disconnected, reason=200"},{"W","terminal_tcp","disconnected; retaining inbox, retrying"},{"I","wifi","WiFi retry 1/5"},{"I","terminal_tcp","connected 192.0.2.10:9000"},{"E","camera","frame capture failed; retry scheduled"},{"I","terminal_query","kind=perception bytes=235 stack_free=2352"},{"I","producer","frames=200 ok=199 run_fail=1 no_target=199"},{"I","terminal","RoboMind-S3 terminal boot version=1"},{"I","terminal","camera init=ESP_OK"},{"I","wifi","WiFi connecting to SSID: [masked]"},{"I","wifi","WiFi connected OK"}};
        int number=5;for(const auto& example:preview){auto sample=row(number++,example[0],example[1],example[2]);sample["received_ms"]=QDateTime::currentMSecsSinceEpoch()-(16-number)*1000;rows.append(sample);}save(folder.path(),rows);CHECK(waitFor([&]{return text->toPlainText().contains("WiFi connected OK");}));
        panel.findChild<QLabel*>("boardLogSource")->setText("样式预览 · 以下为示例日志");
        CHECK(panel.grab().save(value("--capture")));if(args.contains("--card-capture"))CHECK(panel.findChild<QFrame*>("boardLogCard")->grab().save(value("--card-capture")));
    }
    std::puts("PASS log panel: embedded tabs, UTF-8, filters, pause/cache/clear; follows visible latest row, preserves history, resumes at bottom or by checkbox, hidden tab recovery");
}
