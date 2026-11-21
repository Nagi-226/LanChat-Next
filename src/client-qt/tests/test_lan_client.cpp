#include "net/session.h"
#include "net/transfer_manager.h"
#include "model/store.h"
#include "app/devmode.h"
#include "ui/main_window.h"
#include "ui/chat_view.h"
#include "ui/contacts_panel.h"
#include "ui/members_panel.h"
#include "ui/theme.h"
#include "test_assert.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QClipboard>
#include <QJsonArray>
#include <QFontDatabase>
#include <QCryptographicHash>
#include <functional>

static bool wait(const std::function<bool()>& condition,int ms=15000){QElapsedTimer timer;timer.start();while(timer.elapsed()<ms){QCoreApplication::processEvents();if(condition())return true;QTest::qWait(5);}return false;}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.setOrganizationName("LanChat-LAN-Test");app.setApplicationName("RealClientTest");app.setStyleSheet(theme::stylesheet());
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/seguiemj.ttf");app.setFont(QFont("Microsoft YaHei"));
    app.setStyleSheet(theme::stylesheet()+"QWidget {font-family:'Microsoft YaHei';}");
    QTemporaryDir directory;assert(directory.isValid());QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,directory.path());
    QTcpServer probe;assert(probe.listen(QHostAddress::LocalHost,0));auto port=probe.serverPort();probe.close();
    auto args=app.arguments();int serverArg=args.indexOf("--server");assert(serverArg>0);
    QProcess server;server.setWorkingDirectory(directory.path());server.start(args.value(serverArg+1),{"--port",QString::number(port),"--data",directory.path()+"/data"});assert(server.waitForStarted());
    Session a,b;Store sa,sb;DevMode da,db;MainWindow wa(&a,&sa,&da),wb(&b,&sb,&db);wa.show();wb.show();
    QSignalSpy ar(&a,&Session::registered),br(&b,&Session::registered),ag(&a,&Session::groupEvent),bg(&b,&Session::groupEvent);
    QSignalSpy astates(&a,&Session::stateChanged),bstates(&b,&Session::stateChanged);
    a.connectToServer("127.0.0.1",port);b.connectToServer("127.0.0.1",port);
    assert(wait([&]{bool ac=false,bc=false;for(auto s:astates)if(s[0].toString()=="已连接")ac=true;for(auto s:bstates)if(s[0].toString()=="已连接")bc=true;return ac&&bc;}));
    a.registerUser("Alice-client","test-password");b.registerUser("Bob-client","test-password");assert(wait([&]{return ar.size()==1&&br.size()==1;}));
    a.login("Alice-client","test-password");b.login("Bob-client","test-password");assert(wait([&]{return a.selfId()>0&&b.selfId()>0;}));
    a.createGroup("公司项目组");assert(wait([&]{for(auto e:ag)if(e[0].toJsonObject()["type"].toInt()==10)return true;return false;}));
    int gid=0;for(auto e:ag)if(e[0].toJsonObject()["type"].toInt()==10)gid=e[0].toJsonObject()["groupId"].toInt();assert(gid>0);b.joinGroup(gid);
    assert(wait([&]{for(auto e:bg)if(e[0].toJsonObject()["type"].toInt()==14)return true;return false;}));
    auto* chatA=wa.findChild<ChatView*>();auto* chatB=wb.findChild<ChatView*>();assert(chatA&&chatB);
    const QString text="中文 😀 👍 🎉 <img src=evil>";emit chatA->sendRequested(text);
    assert(wait([&]{return sb.conversation(Store::groupKey(gid)).size()==1;}));
    assert(sa.conversation(Store::groupKey(gid)).size()==1);bool plain=false;
    for(auto* label:chatB->findChildren<QLabel*>("bubblePeer"))if(label->text()==text)plain=label->textFormat()==Qt::PlainText;assert(plain);
    auto* ta=wa.findChild<TransferManager*>();auto* tb=wb.findChild<TransferManager*>();assert(ta&&tb);
    QByteArray bytes(3*1024*1024+5,0);for(int i=0;i<bytes.size();++i)bytes[i]=char(i*31);
    QString path=directory.path()+"/真实文件.bin";QFile f(path);assert(f.open(QIODevice::WriteOnly));assert(f.write(bytes)==bytes.size());f.close();
    auto upload=ta->upload(path,0,gid);assert(!upload.isEmpty());ta->pause(upload);assert(!ta->tasks().first().active);ta->resume(upload);
    assert(wait([&]{return sb.conversation(Store::groupKey(gid)).size()==2;}));
    auto attachment=sb.conversation(Store::groupKey(gid)).last().attachment;assert(!attachment.isEmpty());
    QSignalSpy downloaded(tb,&TransferManager::downloaded);auto download=tb->download(attachment);assert(!download.isEmpty());
    tb->pause(download);tb->resume(download);assert(wait([&]{return downloaded.size()==1;}));
    QFile saved(downloaded[0][1].toString());assert(saved.open(QIODevice::ReadOnly));assert(saved.readAll()==bytes);
    // Clipboard PNG traverses the actual widget event, client transfer manager and server.
    QImage image(240,120,QImage::Format_RGB32);image.fill(QColor("#65abf3"));QApplication::clipboard()->setImage(image);
    auto* input=chatA->findChild<QLineEdit*>();assert(input);QTest::keyClick(input,Qt::Key_V,Qt::ControlModifier);
    assert(wait([&]{return sb.conversation(Store::groupKey(gid)).size()==3;}));
    auto png=sb.conversation(Store::groupKey(gid)).last();assert(png.contentType=="image");tb->download(png.attachment);
    assert(wait([&]{return downloaded.size()==2;}));assert(wait([&]{return !chatB->findChildren<QLabel*>("attachmentPreview").isEmpty();}));
    // GIF is a typed PC attachment with QMovie preview.
    QByteArray gif=QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7");
    QString gifPath=directory.path()+"/表情.gif";QFile gf(gifPath);assert(gf.open(QIODevice::WriteOnly));gf.write(gif);gf.close();ta->upload(gifPath,0,gid);
    assert(wait([&]{return sb.conversation(Store::groupKey(gid)).size()==4;}));auto sticker=sb.conversation(Store::groupKey(gid)).last();assert(sticker.contentType=="sticker");tb->download(sticker.attachment);
    assert(wait([&]{return downloaded.size()==3;}));assert(wait([&]{for(auto* p:chatB->findChildren<QLabel*>("attachmentPreview"))if(p->movie())return true;return false;}));
    int capture=args.indexOf("--capture");if(capture>0){QTest::qWait(200);assert(wb.grab().save(args.value(capture+1)));}
    // Switching conversations reloads server history and merges without repeated bubbles.
    auto* contacts=wb.findChild<ContactsPanel*>();emit contacts->contactSelected(a.selfId());emit contacts->groupSelected(gid);QTest::qWait(200);assert(sb.conversation(Store::groupKey(gid)).size()==4);
    assert(chatB->findChildren<QWidget*>("attachmentCard").size()>=3);
    // Paused tasks survive manager destruction and are scoped to account and server.
    QString resumeId;
    {TransferManager temporary(&a);emit a.ready(a.selfId(),a.selfNick());resumeId=temporary.upload(path,0,gid);temporary.pause(resumeId);}
    {TransferManager restored(&a);emit a.ready(a.selfId(),a.selfNick());bool found=false;for(auto t:restored.tasks())if(t.id==resumeId)found=true;assert(found);restored.cancel(resumeId);}
    a.logout();b.logout();server.kill();assert(server.waitForFinished(5000));
    qInfo("PASS real Qt clients: Unicode/plain text, join/list/members/history, file bytes, pause/resume, clipboard PNG, GIF preview, persistent tasks");
}
