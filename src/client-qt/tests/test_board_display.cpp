#include "app/board_connection.h"
#include "app/credentials.h"
#include "net/session.h"
#include "ui/board_connection_panel.h"
#include "ui/theme.h"
#include <QApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
#include <functional>
#include <memory>
#include <cstdio>
#include <cstdlib>

#undef assert
#define assert(condition) do { if(!(condition)){std::fprintf(stderr,"CHECK FAILED %s:%d: %s\n",__FILE__,__LINE__,#condition);std::exit(1);} } while(false)

static bool waitFor(const std::function<bool()>& check,int limit=8000)
{
    QElapsedTimer timer;timer.start();
    while(timer.elapsed()<limit){QCoreApplication::processEvents();if(check())return true;QTest::qWait(10);}
    return false;
}
static void save(const QString& path,const QJsonObject& data)
{
    for(int retry=0;retry<20;++retry){
        QSaveFile file(path);
        if(file.open(QIODevice::WriteOnly)){file.write(QJsonDocument(data).toJson());if(file.commit())return;}
        QTest::qWait(10);
    }
    assert(false);
}
static void reply(QTcpSocket* socket,const QJsonObject& message)
{
    const auto data=QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame(4,0);qToBigEndian<quint32>(data.size(),reinterpret_cast<uchar*>(frame.data()));socket->write(frame+data);
}
int main(int argc,char** argv)
{
    QApplication app(argc,argv);
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    app.setStyleSheet(theme::stylesheet());
    const auto args=app.arguments();
    if(args.contains("--bridge")){
        const auto status=args.value(args.indexOf("--status-file")+1),stop=args.value(args.indexOf("--stop-file")+1);
        QTimer timer;
        QObject::connect(&timer,&QTimer::timeout,&app,[&]{
            if(QFile::exists(stop)){app.quit();return;}
            QJsonObject display{{"terminal_ready",true},{"navigation",true},{"view",QJsonObject{{"page",0},{"language","zh"}}}};
            QFile state(QFileInfo(status).absolutePath()+"/fake-display.json");
            if(state.open(QIODevice::ReadOnly))display=QJsonDocument::fromJson(state.readAll()).object();
            save(status,{{"updated_ms",QDateTime::currentMSecsSinceEpoch()},{"listening",true},{"board_connected",true},{"display",display}});
        });timer.start(100);return app.exec();
    }
    const bool live=args.contains("--live");
    QTemporaryDir temporary;assert(temporary.isValid());
    if(live){app.setOrganizationName("LanChat");app.setApplicationName("LanChat-Qt");}
    else{
        app.setOrganizationName("LanChat-Qt-Test");app.setApplicationName("BoardDisplayTest");
        QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary.path());
    }
    BoardConnection bridge(nullptr,live?QString():temporary.path()+"/bridge");
    Session session;
    QTcpServer server;QString sentText;int sentTo=0;
    if(!live){
        assert(server.listen(QHostAddress::LocalHost,0));
        QObject::connect(&server,&QTcpServer::newConnection,&app,[&]{
            auto* socket=server.nextPendingConnection();auto bytes=std::make_shared<QByteArray>();
            QObject::connect(socket,&QTcpSocket::readyRead,socket,[&,socket,bytes]{
                bytes->append(socket->readAll());
                while(bytes->size()>=4){
                    const auto n=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes->constData()));
                    assert(n<65536);if(bytes->size()<static_cast<int>(n+4))break;
                    const auto message=QJsonDocument::fromJson(bytes->mid(4,n)).object();bytes->remove(0,n+4);
                    if(message.value("type").toInt()==2)reply(socket,{{"type",3},{"id",1000},{"nickname","Sender"}});
                    if(message.value("type").toInt()==5){
                        sentText=message.value("msg").toString();sentTo=message.value("toId").toInt();
                        reply(socket,{{"type",6},{"fromId",1000},{"toId",4242},{"msg",sentText},{"msg_id","server-101"}});
                    }
                }
            });
        });
        auto config=bridge.config();config.consolePath=app.applicationFilePath();config.accountId=4242;config.password="test-only";config.serverPort=server.serverPort();bridge.configure(config);
        session.connectToServer("127.0.0.1",server.serverPort());session.login("test-only","test-only");
    }else{
        const auto saved=credentials::load();
        if(!saved.remembered||saved.password.isEmpty())qFatal("No saved login available for local board validation");
        assert(bridge.config().accountId>0&&!bridge.config().password.isEmpty());
        session.connectToServer(bridge.config().serverHost,bridge.config().serverPort);session.login(saved.username,saved.password);
    }
    bridge.start();
    assert(waitFor([&]{return session.selfId()>0&&bridge.snapshot().value("display").toObject().value("terminal_ready").toBool();},25000));
    BoardConnectionPanel dialog(&bridge,nullptr,&session);dialog.resize(850,820);dialog.show();
    auto* input=dialog.findChild<QPlainTextEdit*>("boardMessageText");
    auto* send=dialog.findChild<QPushButton*>("boardSendMessage");
    auto* delivery=dialog.findChild<QLabel*>("boardMessageDelivery");assert(input&&send&&delivery);
    input->setPlainText(QString(129,QChar(0x8bbe)));assert(!send->isEnabled());
    const QString text="设备已连接，收到中文消息。请查看详情并检查板端。\n保留字符：| ; \\\n第二行显示正常。";
    input->setPlainText(text);assert(send->isEnabled());QTest::mouseClick(send,Qt::LeftButton);assert(!send->isEnabled());
    if(!live){
        assert(waitFor([&]{return sentText==text;}));assert(sentTo==4242);
        // The response uses a server-assigned ID, independently of the client's outbound ID.
        save(bridge.profileDirectory()+"/fake-display.json",{{"terminal_ready",true},{"navigation",true},{"deliveries",QJsonArray{QJsonObject{{"id","server-101"},{"state","accepted"}}}}});
        assert(waitFor([&]{return delivery->text().contains("等待屏幕显示");}));
        save(bridge.profileDirectory()+"/fake-display.json",{{"terminal_ready",true},{"navigation",true},{"deliveries",QJsonArray{QJsonObject{{"id","server-101"},{"state","rendered"}}}}});
    }
    assert(waitFor([&]{return delivery->text().startsWith("板端已显示");},35000));assert(send->isEnabled());
    if(live){
        auto navigate=[&](const QString& action,const std::function<bool(const QJsonObject&)>& check){
            QString id;assert(bridge.navigate(action,&id));
            assert(waitFor([&]{const auto view=bridge.snapshot().value("display").toObject().value("view").toObject();return view.value("control").toString()==id&&check(view);}));
        };
        navigate("detail",[](const QJsonObject& v){return v.value("page").toInt()==1&&v.value("detail").toBool();});
        navigate("scroll_down",[](const QJsonObject& v){return v.value("detail").toBool();});
        QTest::qWait(600);
        navigate("read",[](const QJsonObject& v){return v.value("unread").toInt()==0;});
        navigate("ppm",[](const QJsonObject& v){return v.value("page").toInt()==2;});
        navigate("link",[](const QJsonObject& v){return v.value("page").toInt()==3;});
        navigate("device",[](const QJsonObject& v){return v.value("page").toInt()==4;});
        navigate("lang_en",[](const QJsonObject& v){return v.value("language").toString()=="en";});
        navigate("lang_zh",[](const QJsonObject& v){return v.value("language").toString()=="zh";});
        navigate("home",[](const QJsonObject& v){return v.value("page").toInt()==0;});
        const int output=args.indexOf("--result");
        if(output>0){save(args.value(output+1),{{"delivery",delivery->text()},{"snapshot",bridge.snapshot()}});}
        std::printf("PASS live Qt panel -> LanChat -> Console -> ESP32-S3: Chinese rendered, details, read, project pages, language switch\n");
    }else{
        assert(bridge.navigate("device"));QFile request(bridge.profileDirectory()+"/navigation.request");assert(request.open(QIODevice::ReadOnly));
        assert(QJsonDocument::fromJson(request.readAll()).object().value("action").toString()=="device");
        assert(!bridge.navigate("reboot"));
        std::printf("PASS board display panel: UTF-8 bound, real Session send, server-ID mapping, accepted/rendered feedback, navigation request\n");
    }
    const int capture=args.indexOf("--capture");
    if(capture>0)assert(dialog.grab().save(args.value(capture+1)));
    session.logout();bridge.stop();return 0;
}
