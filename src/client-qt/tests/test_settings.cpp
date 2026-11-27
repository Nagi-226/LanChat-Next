#include "app/preferences.h"
#include "app/credentials.h"
#include "app/board_connection.h"
#include "app/devmode.h"
#include "net/session.h"
#include "net/transfer_manager.h"
#include "model/store.h"
#include "ui/settings_panel.h"
#include "ui/board_connection_panel.h"
#include "ui/main_window.h"
#include "ui/theme.h"
#include "test_assert.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QSettings>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QSystemTrayIcon>
#include <QSignalSpy>
#include <QTest>
#include <QTcpServer>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QTabBar>
#include <functional>
static bool waitFor(const std::function<bool()>& test){QElapsedTimer time;time.start();while(time.elapsed()<5000){QCoreApplication::processEvents();if(test())return true;QTest::qWait(10);}return false;}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.setOrganizationName("LanChat-Settings-Test");app.setApplicationName("SettingsTest");QTemporaryDir temp;assert(temp.isValid());QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temp.path());app.setStyleSheet(theme::stylesheet());
    Session session;Store store;DevMode dev;MainWindow window(&session,&store,&dev);window.show();assert(!window.findChild<QWidget*>("settingsPage"));window.loadDemoData();
    auto* gear=window.findChild<QPushButton*>("settingsButton");if(!gear)for(auto* b:window.findChildren<QPushButton*>())if(b->objectName().contains("ettings")&&b->isVisible())gear=b;
    assert(gear);gear->click();assert(window.findChild<QWidget*>("accountSettingsPage"));
    auto* panel=window.findChild<SettingsPanel*>();auto* bridge=window.findChild<BoardConnection*>();auto* transfers=window.findChild<TransferManager*>();assert(panel&&bridge&&transfers);
    panel->setSection(1);panel->findChild<QCheckBox*>("notify_group")->setChecked(false);panel->findChild<QCheckBox*>("do_not_disturb")->setChecked(true);
    // 默认开机启动保持现状，测试不修改用户的 Windows Run 注册表。
    panel->findChild<QCheckBox*>("startup")->setChecked(preferences::startupEnabled());panel->findChild<QPushButton*>("saveNotifications")->click();
    assert(!preferences::shouldNotify(false,false,false));panel->findChild<QCheckBox*>("do_not_disturb")->setChecked(false);panel->findChild<QPushButton*>("saveNotifications")->click();
    assert(preferences::shouldNotify(false,false,false));assert(!preferences::shouldNotify(true,false,false));assert(!preferences::shouldNotify(false,false,true));assert(preferences::shouldNotify(true,true,false));
    panel->setSection(2);panel->findChild<QLineEdit*>("download_directory")->setText(temp.path()+"/downloads");panel->findChild<QCheckBox*>("auto_download_images")->setChecked(true);panel->findChild<QSpinBox*>("auto_download_mb")->setValue(2);panel->findChild<QPushButton*>("saveStorage")->click();assert(TransferManager::downloadDirectory()==temp.path()+"/downloads");assert(!transfers->autoDownload({{"file_name","a.exe"},{"file_size",12},{"content_type","file"}}));assert(!transfers->autoDownload({{"file_name","large.png"},{"file_size",3*1024*1024},{"content_type","image"}}));
    panel->setSection(3);panel->findChild<QSpinBox*>("aiDailyLimit")->setValue(7);panel->findChild<QSpinBox*>("aiTimeout")->setValue(30);panel->findChild<QSpinBox*>("aiMaxTokens")->setValue(888);panel->findChild<QSpinBox*>("aiConcurrency")->setValue(1);panel->findChild<QSpinBox*>("aiInterval")->setValue(10);panel->findChild<QPushButton*>("saveAiLimits")->click();assert(bridge->config().aiDailyLimit==7&&bridge->config().aiMaxTokens==888);BoardConnection restored(nullptr,temp.path()+"/restored");assert(restored.config().aiTimeoutSeconds==30&&restored.config().aiConcurrency==1&&restored.config().aiIntervalSeconds==10);
    const auto secret=QString("never-export-this-secret");const auto diag=preferences::diagnostics({{"api_key",secret},{"password",secret},{"logs",QJsonObject{{"text",secret}}},{"ai",QJsonObject{{"raw_response",secret},{"last_http",429},{"error_kind","quota"}}}},"connected","hosted",1);assert(!diag.contains(secret));assert(diag.contains("429")&&diag.contains("quota"));
    QTcpServer http;assert(http.listen(QHostAddress::LocalHost));int calls=0,code=200;QString method;QObject::connect(&http,&QTcpServer::newConnection,&app,[&]{auto* socket=http.nextPendingConnection();QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{const auto input=socket->readAll();if(!input.contains("\r\n\r\n"))return;++calls;method=QString::fromLatin1(input.left(input.indexOf('\r')));QByteArray body=code==200?QByteArray("{\"data\":[{\"id\":\"fixture\"}]}"):QByteArray("{\"error\":\"never-export-this-secret\"}");socket->write("HTTP/1.1 "+QByteArray::number(code)+" Test\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);socket->disconnectFromHost();});});
    auto c=bridge->config();c.aiKey=secret;c.aiProvider="openai";c.aiEndpoint=QString("http://127.0.0.1:%1/v1").arg(http.serverPort());c.aiModel="fixture";bridge->configure(c);
    auto* detect=panel->findChild<QPushButton*>("aiEndpointCheck");detect->click();assert(waitFor([&]{return calls==1&&detect->isEnabled();}));assert(method=="GET /v1/models HTTP/1.1");assert(panel->findChild<QLabel*>("settingsFeedback")->text().contains("找到"));code=429;detect->click();assert(waitFor([&]{return calls==2&&detect->isEnabled();}));assert(panel->findChild<QLabel*>("settingsFeedback")->text().contains("429")&&!panel->findChild<QLabel*>("settingsFeedback")->text().contains(secret));
    QSettings().setValue("board_ai/enabled",true);
    BoardConnection migrated(nullptr,temp.path()+"/migrated");
    assert(migrated.aiProfile(0).key==secret&&migrated.aiProfile(0,true).key==secret);
    assert(migrated.aiProfile(0,true).verifiedAt==0&&migrated.aiProfile(1).key.isEmpty());
    assert(migrated.aiProfile(1).endpoint=="https://agentrouter.org/v1"&&migrated.aiProfile(1).model=="gpt-6.2-sol");
    BoardConnectionPanel profiles(&migrated);
    auto* tabs=profiles.findChild<QTabBar*>("boardAiProfiles");
    auto* endpoint=profiles.findChild<QLineEdit*>("boardAiEndpoint");
    auto* model=profiles.findChild<QLineEdit*>("boardAiModel");
    auto* key=profiles.findChild<QLineEdit*>("boardAiKey");
    auto* save=profiles.findChild<QPushButton*>("boardAiSaveProfile");
    auto* probe=profiles.findChild<QPushButton*>("boardAiTestProfile");
    auto* restore=profiles.findChild<QPushButton*>("boardAiRestoreProfile");
    assert(tabs&&tabs->count()==3&&endpoint&&model&&key&&save&&probe&&restore);
    tabs->setCurrentIndex(1);assert(key->text().isEmpty());key->setText("gpt-fixture");save->click();
    tabs->setCurrentIndex(2);assert(endpoint->text().isEmpty()&&key->text().isEmpty());
    endpoint->setText("https://example.invalid/v1");model->setText("third-fixture");save->click();
    tabs->setCurrentIndex(0);
    assert(model->text()=="fixture"&&key->text().isEmpty()&&restore->isEnabled());
    assert(migrated.aiProfile(1).key=="gpt-fixture"&&migrated.aiProfile(2).model=="third-fixture");
    const QByteArray disk=QSettings().value("board_ai_profiles/1/draft").toMap().value("secret").toByteArray();
    assert(!disk.isEmpty()&&!disk.contains("gpt-fixture"));
    QTcpServer generation;assert(generation.listen(QHostAddress::LocalHost));
    int probes=0, probeCode=200;bool requestsCorrect=true;
    QObject::connect(&generation,&QTcpServer::newConnection,&app,[&]{
        auto* socket=generation.nextPendingConnection();
        QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{
            auto data=socket->property("request").toByteArray()+socket->readAll();
            const auto split=data.indexOf("\r\n\r\n");
            const auto head=data.left(split);
            int length=0;
            for(const auto& line:head.split('\n'))
                if(line.toLower().startsWith("content-length:"))length=line.mid(15).trimmed().toInt();
            if(split<0||data.size()<split+4+length){socket->setProperty("request",data);return;}
            requestsCorrect=requestsCorrect&&data.startsWith("POST /v1/chat/completions HTTP/1.1\r\n")
                &&!data.contains("gpt-fixture")&&!data.contains(secret.toUtf8());
            ++probes;
            const QByteArray body=probeCode==200?"{\"choices\":[{\"message\":{\"content\":\"OK\"}}]}":"{\"error\":{\"message\":\"fixture\"}}";
            socket->write("HTTP/1.1 "+QByteArray::number(probeCode)+" Test\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body);
            socket->disconnectFromHost();
        });
    });
    endpoint->setText(QString("http://127.0.0.1:%1/v1").arg(generation.serverPort()));
    probe->click();QTest::qWait(40);assert(probes==0&&migrated.config().aiKey==secret);
    key->setText("new-ds-fixture");probe->click();
    assert(waitFor([&]{return probes==1&&probe->isEnabled();}));
    assert(migrated.aiProfile(0,true).verifiedAt>0&&migrated.config().aiKey=="new-ds-fixture");
    probeCode=401;key->setText("wrong-ds-fixture");probe->click();
    assert(waitFor([&]{return probes==2&&probe->isEnabled();}));
    assert(migrated.aiProfile(0).key=="wrong-ds-fixture"&&migrated.aiProfile(0,true).key=="new-ds-fixture");
    assert(requestsCorrect&&migrated.config().aiKey=="new-ds-fixture");
    restore->click();BoardConnection profileReload(nullptr,temp.path()+"/profile-reload");
    assert(profileReload.aiProfile(0).key=="new-ds-fixture"&&profileReload.aiProfile(1).key=="gpt-fixture"&&profileReload.aiProfile(2).model=="third-fixture");
    panel->setSection(4);panel->findChild<QSpinBox*>("font_scale")->setValue(125);panel->findChild<QComboBox*>("board_default_page")->setCurrentIndex(3);panel->findChild<QCheckBox*>("log_follow")->setChecked(false);panel->findChild<QPushButton*>("saveDisplay")->click();assert(QSettings().value("preferences/font_scale").toInt()==125&&QSettings().value("preferences/board_default_page").toString()=="ppm");assert(app.styleSheet()==preferences::scaledStyleSheet(125));
    const auto args=app.arguments();if(args.contains("--capture-dir")){const auto dir=args.value(args.indexOf("--capture-dir")+1);QDir().mkpath(dir);const QStringList ids={"accountSettingsButton","notificationSettingsButton","storageSettingsButton","aiSettingsButton","displaySettingsButton","aboutSettingsButton"};for(int i=0;i<6;++i){window.findChild<QPushButton*>(ids[i])->click();QTest::qWait(100);assert(window.grab().save(dir+QString("/settings-%1.png").arg(i)));}}
    for(int percent:{100,125,150})for(int width:{950,1300}){QSettings().setValue("preferences/font_scale",percent);preferences::applyDisplay();window.resize(width,800);window.findChild<QPushButton*>("aiSettingsButton")->click();QTest::qWait(100);auto* hint=panel->findChild<QLabel*>("aiLimitHint");assert(hint->height()>=hint->heightForWidth(hint->width()));}
    if(QSystemTrayIcon::isSystemTrayAvailable()){QSettings().setValue("preferences/close_to_tray",true);window.close();assert(!window.isVisible());window.show();QSettings().setValue("preferences/close_to_tray",false);}
    credentials::save("isolated-user","isolated-password");panel->setSection(0);panel->findChild<QPushButton*>("forgetLogin")->click();assert(!credentials::load().remembered);panel->findChild<QPushButton*>("logoutAccount")->click();assert(window.findChild<QStackedWidget*>("mainPages")->currentIndex()==0);window.findChild<QPushButton*>("aiSettingsButton")->click();assert(window.findChild<QStackedWidget*>("mainPages")->currentIndex()==0);
    std::puts("PASS settings: login gate/logout, durable preferences, notifications, safe download controls, AI settings restore, GET-only diagnostics, redaction, font scaling, tray close");return 0;
}
