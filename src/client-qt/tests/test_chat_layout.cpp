#include "ui/chat_view.h"
#include "ui/theme.h"
#include "test_assert.h"
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFontDatabase>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>

int main(int argc,char** argv) {
    QApplication app(argc,argv);QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");app.setStyleSheet(theme::stylesheet());
    ChatView chat;chat.setSelfId(1);chat.resize(820,620);chat.show();
    QString text="[S3 · AI · #27]\n结论：板子运行和链路正常，但感知结果未计算。\n\n";
    for(int i=0;i<10;++i)text+=QString("第 %1 段：S3 与 Console 已连接；内部堆 61.6 KiB，PSRAM 7.75 MiB。未计算不能作为正常结论。\n\n").arg(i+1);
    text+="消息末尾：完整内容必须显示。";
    const auto now=QDateTime::currentMSecsSinceEpoch();
    chat.appendMessage(2,"Board",text,now,false,1);
    chat.appendMessage(3,"Peer",text,now,false);
    chat.appendMessage(1,"Me",text,now,true);
    auto* scroll=chat.findChild<QScrollArea*>();assert(scroll);
    for(int width:{820,420,1100,540}) {
        chat.resize(width,620);QTest::qWait(80);
        for(const QString name:{QString("alertBody"),QString("bubblePeer"),QString("bubbleSelf")}) {
            auto* label=chat.findChild<QLabel*>(name);assert(label&&label->text().endsWith("完整内容必须显示。"));
            const int needed=label->heightForWidth(label->width());
            std::fprintf(stderr,"%s width=%d height=%d needed=%d\n",qPrintable(name),label->width(),label->height(),needed);
            assert(label->height()>=needed);
            assert(label->parentWidget()->rect().contains(label->geometry()));
        }
        assert(scroll->horizontalScrollBar()->maximum()==0);
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
        assert(scroll->verticalScrollBar()->value()==scroll->verticalScrollBar()->maximum());
    }
    const auto args=app.arguments();if(args.contains("--capture-dir")) {
        const QString folder=args.value(args.indexOf("--capture-dir")+1);QDir().mkpath(folder);
        chat.clear();chat.resize(820,900);chat.appendMessage(2,"RoboMind-S3",text,now,false,1);QTest::qWait(100);
        scroll->verticalScrollBar()->setValue(0);assert(chat.grab().save(folder+"/long-message-top.png"));
        scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());assert(chat.grab().save(folder+"/long-message-bottom.png"));
    }
    std::puts("PASS long messages: alert/peer/self full wrapped height, resize and scroll to final line");return 0;
}
