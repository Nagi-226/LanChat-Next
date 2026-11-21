#include "ui/main_window.h"
#include "ui/chat_view.h"
#include "ui/contacts_panel.h"
#include "ui/theme.h"
#include "app/devmode.h"
#include "model/store.h"
#include "net/session.h"
#include <QApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
#include <functional>
#include <memory>
#include "test_assert.h"

static bool waitFor(const std::function<bool()>& check)
{
    QElapsedTimer timer;timer.start();
    while(timer.elapsed()<5000){QCoreApplication::processEvents();if(check())return true;QTest::qWait(10);}
    return false;
}
static void deliver(QTcpSocket* socket,const QJsonObject& object)
{
    const QByteArray payload=QJsonDocument(object).toJson(QJsonDocument::Compact);
    QByteArray packet(4,0);qToBigEndian<quint32>(payload.size(),reinterpret_cast<uchar*>(packet.data()));
    socket->write(packet+payload);
}
static int bubbles(ChatView* chat)
{
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    return chat->findChildren<QLabel*>("bubbleSelf").size();
}
int main(int argc,char** argv)
{
    QApplication app(argc,argv);QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    app.setStyleSheet(theme::stylesheet());
    QTemporaryDir directory;assert(directory.isValid());
    app.setOrganizationName("LanChat-Qt-Test");app.setApplicationName("MessageEchoTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
    Session session;Store store;DevMode dev;MainWindow window(&session,&store,&dev);window.show();
    QTcpServer server;assert(server.listen(QHostAddress::LocalHost,0));
    QTcpSocket* peer=nullptr;QList<QJsonObject> outbound;bool badFrame=false;
    // Fake server decodes length+JSON independently and waits for explicit echo delivery.
    QObject::connect(&server,&QTcpServer::newConnection,&app,[&]{
        peer=server.nextPendingConnection();auto bytes=std::make_shared<QByteArray>();
        QObject::connect(peer,&QTcpSocket::readyRead,peer,[&,bytes]{
            bytes->append(peer->readAll());
            while(bytes->size()>=4){
                const quint32 size=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(bytes->constData()));
                if(size==0||size>65536){badFrame=true;return;}
                if(bytes->size()<static_cast<int>(size+4))return;
                const auto request=QJsonDocument::fromJson(bytes->mid(4,size)).object();bytes->remove(0,size+4);
                const int type=request.value("type").toInt();
                if(type==2)deliver(peer,{{"type",3},{"id",1000},{"nickname","Sender"},
                    {"users",QJsonArray{QJsonObject{{"id",4242},{"nickname","Board"}}}},
                    {"groups",QJsonArray{QJsonObject{{"groupId",9},{"name","Group"}}}}});
                else if(type==5||type==16)outbound.append(request);
            }
        });
    });
    session.connectToServer("127.0.0.1",server.serverPort());session.login("test-user","test-only");
    assert(waitFor([&]{return session.selfId()==1000&&!store.contacts().isEmpty();}));
    auto* contacts=window.findChild<ContactsPanel*>();auto* chat=window.findChild<ChatView*>();assert(contacts&&chat);
    auto* input=chat->findChild<QLineEdit*>();assert(input);
    QSignalSpy receipts(&session,&Session::messageReceived);
    auto send=[&](const QString& text){input->setText(text);QTest::keyClick(input,Qt::Key_Return);};
    auto echo=[&](int index){
        auto response=outbound[index];response["type"]=response.value("type").toInt()==5?6:17;
        response["msg_id"]=QString("server-%1").arg(index+1);
        response["timestamp"]=QDateTime::currentMSecsSinceEpoch();deliver(peer,response);
    };
    const QString body="您好";
    emit contacts->contactSelected(4242);send(body);
    assert(waitFor([&]{return outbound.size()==1;}));
    assert(outbound[0].value("type").toInt()==5&&outbound[0].value("toId").toInt()==4242);
    // A send followed by its echo must create one stored message and one bubble.
    echo(0);assert(waitFor([&]{return receipts.size()==1;}));
    assert(store.conversation(Store::directKey(4242)).size()==1&&bubbles(chat)==1);
    const int capture=app.arguments().indexOf("--capture");
    if(capture>0)assert(window.grab().save(app.arguments().value(capture+1)));
    send(body);assert(waitFor([&]{return outbound.size()==2;}));echo(1);
    assert(waitFor([&]{return receipts.size()==2;}));assert(store.conversation(Store::directKey(4242)).size()==2&&bubbles(chat)==2);
    emit contacts->groupSelected(9);send(body);assert(waitFor([&]{return outbound.size()==3;}));
    assert(outbound[2].value("type").toInt()==16&&outbound[2].value("groupId").toInt()==9);
    echo(2);assert(waitFor([&]{return receipts.size()==3;}));assert(store.conversation(Store::groupKey(9)).size()==1&&bubbles(chat)==1);
    send(body);assert(waitFor([&]{return outbound.size()==4;}));echo(3);
    assert(waitFor([&]{return receipts.size()==4;}));assert(store.conversation(Store::groupKey(9)).size()==2&&bubbles(chat)==2);
    emit contacts->contactSelected(4242);assert(bubbles(chat)==2);
    emit contacts->groupSelected(9);assert(bubbles(chat)==2);
    assert(store.search(body).size()==4&&!badFrame);
    session.logout();QObject::disconnect(peer,nullptr,nullptr,nullptr);peer->disconnectFromHost();server.close();
    std::printf("PASS message echo: direct/group send+echo once, repeated identical messages preserved, conversation reopening has no duplicates\n");
    return 0;
}
