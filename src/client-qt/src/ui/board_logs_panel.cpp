#include "board_logs_panel.h"
#include "app/board_connection.h"
#include "net/session.h"
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QScrollArea>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QKeyEvent>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextBlock>
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlockFormat>
#include <QTextEdit>
#include <QAbstractTextDocumentLayout>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr int severityProperty = QTextFormat::UserProperty + 1;
// 左侧细线随日志滚动；只绘制可见行，长日志换行后仍是一整条告警。
class LogView : public QTextEdit {
public:
    explicit LogView(QWidget* parent):QTextEdit(parent){setAcceptRichText(false);}
protected:
    void paintEvent(QPaintEvent* event) override {
        QTextEdit::paintEvent(event);QPainter painter(viewport());
        for(auto block=cursorForPosition(QPoint(0,0)).block();block.isValid();block=block.next()){
            const auto rect=document()->documentLayout()->blockBoundingRect(block).translated(0,-verticalScrollBar()->value());if(rect.top()>viewport()->height())break;
            const auto severity=block.blockFormat().property(severityProperty).toString();
            if(severity=="E"||severity=="W"){
                const QRectF band(0,rect.top()-6,viewport()->width(),rect.height()+12);
                painter.fillRect(band,severity=="E"?QColor(180,78,98,28):QColor(170,139,61,24));
                painter.fillRect(QRectF(0,band.top(),3,band.height()),severity=="E"?QColor("#d45763"):QColor("#9f7b37"));
            }
        }
    }
};
}

BoardLogsPanel::BoardLogsPanel(BoardConnection* bridge,Session* session,QWidget* parent):QWidget(parent),bridge_(bridge),session_(session){
    setObjectName("boardLogsPanel");auto* outer=new QVBoxLayout(this);outer->setContentsMargins(0,8,0,0);
    outer->setSpacing(12);outer->setAlignment(Qt::AlignTop);
    setStyleSheet(R"(
        QFrame#boardLogSourceCard, QFrame#boardLogCard, QFrame#boardQueryCard { background:#1b212c; border:1px solid #2e3544; border-radius:10px; }
        QLabel { background:transparent; border:0; color:#dfe4ed; }
        QLabel#logCardTitle { font-size:17px; font-weight:600; }
        QLabel#boardLogLastTime, QLabel#boardLogCounts, QLabel#logHint { color:#8b93a7; font-size:11px; }
        QFrame#logSeparator { background:#2e3544; border:0; min-height:1px; max-height:1px; }
        QLineEdit, QComboBox { background:#151a23; color:#e6e9f0; border:1px solid #2e3544; border-radius:7px; padding:9px 10px; min-height:17px; }
        QLineEdit:focus, QComboBox:focus { border-color:#8099bb; }
        QPushButton { background:#242b38; color:#e6e9f0; border:1px solid #2e3544; border-radius:7px; padding:8px 10px; font-size:12px; }
        QPushButton:hover { background:#2b3444; border-color:#64728a; }
        QPushButton:pressed { background:#303b4d; }
        QPushButton#boardLogStart { background:#283c32; color:#b0e7c4; border-color:#375845; }
        QPushButton#boardLogStart:hover { background:#314b3d; }
        QPushButton#boardLogPause[paused="true"] { background:#433725; color:#e8bf76; border-color:#75603b; }
        QTextEdit#boardLogText { background:#121720; color:#dde2ed; border:0; border-radius:0; padding:0; font-family:"Consolas","Microsoft YaHei"; font-size:12px; selection-background-color:#35445b; }
        QPlainTextEdit#boardLogReplies { background:#171d27; border:1px solid #2e3544; padding:10px; font-size:12px; }
        QCheckBox { background:transparent; color:#8b93a7; font-size:11px; }
    )");
    auto* sourceCard=new QFrame(this);sourceCard->setObjectName("boardLogSourceCard");auto* sourceLayout=new QVBoxLayout(sourceCard);sourceLayout->setContentsMargins(14,12,14,12);
    auto* sourceRow=new QHBoxLayout;source_=new QLabel(this);source_->setWordWrap(true);source_->setObjectName("boardLogSource");sourceRow->addWidget(source_,1);
    port_=new QComboBox(this);port_->setEditable(true);port_->addItem(bridge_->config().logPort);port_->setObjectName("boardLogPort");port_->setMaximumWidth(115);sourceRow->addWidget(port_);
    auto* start=new QPushButton("开始采集",this);start->setObjectName("boardLogStart");auto* stop=new QPushButton("停止采集",this);stop->setObjectName("boardLogStop");sourceRow->addWidget(start);sourceRow->addWidget(stop);sourceLayout->addLayout(sourceRow);
    auto* note=new QLabel("板端 USB 原始输出 · 115200 · 采集前的输出无法补取\n烧录或使用其他串口监视器前，请停止采集以释放端口。",this);note->setObjectName("logHint");note->setWordWrap(true);sourceLayout->addWidget(note);outer->addWidget(sourceCard);
    auto* columns=new QHBoxLayout;columns->setSpacing(14);
    auto* logCard=new QFrame(this);logCard->setObjectName("boardLogCard");auto* queryCard=new QFrame(this);queryCard->setObjectName("boardQueryCard");
    auto* left=new QVBoxLayout(logCard);auto* right=new QVBoxLayout(queryCard);left->setContentsMargins(0,0,0,0);left->setSpacing(0);right->setContentsMargins(14,14,14,14);right->setSpacing(10);right->setAlignment(Qt::AlignTop);
    auto* header=new QHBoxLayout;header->setContentsMargins(16,15,16,15);header->setSpacing(12);auto* title=new QLabel("实时日志",this);title->setObjectName("logCardTitle");lastTime_=new QLabel(this);lastTime_->setObjectName("boardLogLastTime");follow_=new QCheckBox("跟随最新",this);follow_->setObjectName("boardLogFollow");follow_->setChecked(true);follow_->setToolTip("新日志自动显示在底部；向上查看历史会暂停跟随，回到底部或重新勾选可恢复。");header->addWidget(title,1);header->addWidget(lastTime_);header->addWidget(follow_);left->addLayout(header);
    auto separator=[this](QVBoxLayout* layout){auto* line=new QFrame(this);line->setObjectName("logSeparator");line->setFixedHeight(1);layout->addWidget(line);};separator(left);
    auto* filters=new QGridLayout;level_=new QComboBox(this);level_->setObjectName("boardLogLevel");level_->addItem("全部级别","all");level_->addItem("警告与错误","warning");level_->addItem("仅错误","E");
    module_=new QComboBox(this);module_->setObjectName("boardLogModule");module_->addItem("全部模块","all");for(const auto& module:QStringList{"terminal","terminal_query","wifi","terminal_tcp","producer","camera"})module_->addItem(module,module);
    search_=new QLineEdit(this);search_->setObjectName("boardLogSearch");search_->setPlaceholderText("搜索当前日志");filters->setContentsMargins(14,12,14,0);filters->setSpacing(8);filters->addWidget(level_,0,0);filters->addWidget(module_,0,1);filters->addWidget(search_,1,0,1,2);left->addLayout(filters);
    auto* actions=new QHBoxLayout;actions->setContentsMargins(14,10,14,12);actions->setSpacing(6);pause_=new QPushButton("暂停显示",this);pause_->setObjectName("boardLogPause");auto* clear=new QPushButton("清空视图",this);clear->setObjectName("boardLogClear");auto* copy=new QPushButton("复制筛选结果",this);auto* exportLog=new QPushButton("导出日志",this);exportLog->setObjectName("boardLogExport");actions->addWidget(pause_);actions->addWidget(clear);actions->addWidget(copy);actions->addWidget(exportLog);actions->addStretch();left->addLayout(actions);
    separator(left);logs_=new LogView(this);logs_->setObjectName("boardLogText");logs_->setReadOnly(true);logs_->document()->setMaximumBlockCount(2000);logs_->document()->setDocumentMargin(16);logs_->setFixedHeight(350);logs_->setFont(QFont("Consolas",10));logs_->setToolTip("时间为主机接收时间。复制与导出包含完整日期、来源端口和脱敏正文。");left->addWidget(logs_);separator(left);
    auto* footer=new QHBoxLayout;footer->setContentsMargins(14,12,14,10);counts_=new QLabel(this);counts_->setWordWrap(true);counts_->setObjectName("boardLogCounts");raw_=new QCheckBox("显示原始报文 hex",this);raw_->setObjectName("boardLogRaw");footer->addWidget(counts_,1);footer->addWidget(raw_);left->addLayout(footer);
    auto* retention=new QLabel("缓存：2,000 行 / 4 MiB · 历史滚动最多 20 MiB\n暂停只冻结显示；清空视图保留日志缓存。",this);retention->setObjectName("logHint");retention->setWordWrap(true);retention->setContentsMargins(14,0,14,14);left->addWidget(retention);
    auto* queryTitle=new QLabel("聊天日志查询",this);queryTitle->setObjectName("logCardTitle");right->addWidget(queryTitle);auto* queryHint=new QLabel("专用板端账号 · 回复同步到原会话",this);queryHint->setObjectName("logHint");queryHint->setWordWrap(true);right->addWidget(queryHint);separator(right);
    auto* shortcuts=new QGridLayout;const QStringList commands={"/日志","/日志 错误","/日志 链路","/日志 帮助"};for(int i=0;i<commands.size();++i){auto* button=new QPushButton(commands[i],this);shortcuts->addWidget(button,i/2,i%2);connect(button,&QPushButton::clicked,this,[this,text=commands[i]]{command_->setText(text);query(text);});}right->addLayout(shortcuts);
    replies_=new QPlainTextEdit(this);replies_->setObjectName("boardLogReplies");replies_->setReadOnly(true);replies_->setMaximumBlockCount(160);replies_->setFixedHeight(350);replies_->setPlaceholderText("登录后向专用板端账号查询。其他 PC 可私聊板端账号或在已配置群中输入 /日志。");right->addWidget(replies_);
    command_=new QLineEdit("/日志",this);command_->setObjectName("boardLogCommand");auto* send=new QPushButton("发送查询",this);auto* form=new QHBoxLayout;form->addWidget(command_,1);form->addWidget(send);right->addLayout(form);feedback_=new QLabel(this);feedback_->setWordWrap(true);feedback_->setObjectName("boardLogFeedback");right->addWidget(feedback_);
    columns->addWidget(logCard,2,Qt::AlignTop);columns->addWidget(queryCard,1,Qt::AlignTop);outer->addLayout(columns);
    connect(start,&QPushButton::clicked,this,[this]{feedback_->setText(bridge_->setLogCapture(true,port_->currentText())?"正在启用采集，等待串口状态…":"无法提交采集请求");});
    connect(stop,&QPushButton::clicked,this,[this]{feedback_->setText(bridge_->setLogCapture(false,port_->currentText())?"正在停止采集…":"桥未启动，采集已停止");});
    connect(pause_,&QPushButton::clicked,this,[this]{paused_=!paused_;pause_->setText(paused_?"恢复显示":"暂停显示");pause_->setProperty("paused",paused_);pause_->setStyleSheet(paused_?"background:#433725;color:#e8bf76;border-color:#75603b;":QString());if(!paused_){setFollowing(true);rows_=bridge_->logSnapshot().value("rows").toArray();}refresh();render();});
    connect(clear,&QPushButton::clicked,this,[this]{cleared_=bridge_->logSnapshot().value("status").toObject().value("sequence").toInteger();render();feedback_->setText("已清空本地视图；聊天查询的缓存保留。");});
    connect(copy,&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(filteredText());feedback_->setText("已复制当前筛选的脱敏日志。");});
    connect(exportLog,&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getSaveFileName(this,"导出脱敏日志","LanChat-板端日志.txt","文本文件 (*.txt)");if(path.isEmpty())return;QSaveFile file(path);const auto bytes=filteredText().toUtf8();const bool ok=file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size()&&file.commit();feedback_->setText(ok?"已导出 UTF-8 脱敏日志。":"日志导出失败。");});
    connect(level_,&QComboBox::currentIndexChanged,this,[this]{render();});connect(module_,&QComboBox::currentIndexChanged,this,[this]{render();});connect(search_,&QLineEdit::textChanged,this,[this]{render();});connect(raw_,&QCheckBox::toggled,this,[this]{render();});
    connect(follow_,&QCheckBox::toggled,this,[this](bool checked){setFollowing(checked);});
    connect(logs_->verticalScrollBar(),&QScrollBar::actionTriggered,this,[this]{if(!rendering_)setFollowing(logs_->verticalScrollBar()->sliderPosition()==logs_->verticalScrollBar()->maximum());});
    connect(logs_->verticalScrollBar(),&QScrollBar::sliderMoved,this,[this](int position){if(!rendering_)setFollowing(position==logs_->verticalScrollBar()->maximum());});
    connect(logs_->verticalScrollBar(),&QScrollBar::sliderReleased,this,[this]{setFollowing(logs_->verticalScrollBar()->value()==logs_->verticalScrollBar()->maximum());});
    connect(logs_->verticalScrollBar(),&QScrollBar::rangeChanged,this,[this]{if(!rendering_)scrollToLatest();});
    logs_->installEventFilter(this);logs_->viewport()->installEventFilter(this);
    connect(send,&QPushButton::clicked,this,[this]{query(command_->text());});connect(command_,&QLineEdit::returnPressed,this,[this]{query(command_->text());});
    if(session_)connect(session_,&Session::messageReceived,this,[this](const QJsonObject& message){const auto text=message.value("msg").toString();const int board=bridge_->config().accountId,self=session_->selfId();if(message.value("type").toInt()!=6)return;
        if(message.value("fromId").toInt()==self&&message.value("toId").toInt()==board&&text.startsWith("/日志"))replies_->appendPlainText("你："+text);
        else if(message.value("fromId").toInt()==board&&message.value("toId").toInt()==self&&text.startsWith("[S3 · 日志"))replies_->appendPlainText(text+"\n");});
    connect(bridge_,&BoardConnection::logsChanged,this,[this]{refresh();});connect(bridge_,&BoardConnection::changed,this,[this]{refresh();});auto* timer=new QTimer(this);timer->setInterval(1000);connect(timer,&QTimer::timeout,this,[this]{refresh();});timer->start();refresh();
}
void BoardLogsPanel::setFollowing(bool following){following_=following;const QSignalBlocker blocker(follow_);follow_->setChecked(following);if(following&&!paused_)scrollToLatest();}
void BoardLogsPanel::scrollToLatest(){
    if(scrollScheduled_||!following_||paused_)return;scrollScheduled_=true;
    // QTextEdit 在布局和显示事件后才有最终滚动范围，不能只在 insertText 后取 maximum。
    QTimer::singleShot(0,this,[this]{scrollScheduled_=false;if(!following_||paused_||!logs_->isVisible())return;
        const QScopedValueRollback<bool> rendering(rendering_,true);logs_->document()->documentLayout()->documentSize();
        logs_->verticalScrollBar()->setValue(logs_->verticalScrollBar()->maximum());
        // 外层板端页面也要露出日志区底部，避免最新行已滚到末尾却被页面裁掉。
        for(auto* parent=logs_->parentWidget();parent;parent=parent->parentWidget())if(auto* area=qobject_cast<QScrollArea*>(parent)){
            const int bottom=logs_->viewport()->mapTo(area->widget(),QPoint(0,logs_->viewport()->height())).y()+8;
            if(bottom>area->verticalScrollBar()->value()+area->viewport()->height())area->verticalScrollBar()->setValue(bottom-area->viewport()->height());break;
        }
    });
}
bool BoardLogsPanel::eventFilter(QObject* watched,QEvent* event){
    if(watched==logs_->viewport()&&(event->type()==QEvent::Show||event->type()==QEvent::Resize))scrollToLatest();
    if(watched==logs_&&event->type()==QEvent::KeyPress){const auto key=static_cast<QKeyEvent*>(event)->key();
        if(key==Qt::Key_Up||key==Qt::Key_Down||key==Qt::Key_PageUp||key==Qt::Key_PageDown||key==Qt::Key_Home||key==Qt::Key_End){
            setFollowing(false);QTimer::singleShot(0,this,[this]{setFollowing(logs_->verticalScrollBar()->value()==logs_->verticalScrollBar()->maximum());});
        }
    }
    return QWidget::eventFilter(watched,event);
}
void BoardLogsPanel::query(const QString& command){const auto text=command.normalized(QString::NormalizationForm_KC).trimmed();if(!QStringList{"/日志","/日志 错误","/日志 链路","/日志 帮助"}.contains(text)){feedback_->setText("请使用四种日志查询命令之一。");return;}if(!session_||session_->selfId()<1||bridge_->config().accountId<1){feedback_->setText("请先登录并连接板端桥。");return;}session_->sendPrivate(bridge_->config().accountId,text);feedback_->setText("查询已提交，等待原会话回复。");}
void BoardLogsPanel::refresh(){const auto snapshot=bridge_->logSnapshot(),status=snapshot.value("status").toObject();const QString state=status.value("state").toString();const bool fresh=bridge_->running()&&QDateTime::currentMSecsSinceEpoch()-status.value("updated_ms").toInteger()<2500;
    const QString label=!fresh?"采集未运行 / 历史缓存":state=="running"?"USB 日志采集中":state=="busy"?"串口被占用，等待释放":state=="unavailable"?"USB 未接入或端口不可用":"采集已停止";
    source_->setText("● "+label+(status.value("port").toString().isEmpty()?QString():" · "+status.value("port").toString()));
    source_->setStyleSheet(QString("color:%1;font-weight:600;").arg(fresh&&state=="running"?"#a8dfba":"#dfb16c"));
    lastTime_->setText(status.value("last_ms").toInteger()>0?"采集末条 "+QDateTime::fromMSecsSinceEpoch(status.value("last_ms").toInteger()).toString("HH:mm:ss"):"尚无日志");
    for(const auto& value:status.value("ports").toArray()){const auto name=value.toObject().value("name").toString();if(port_->findText(name)<0)port_->addItem(name);}
    const auto sequence=status.value("sequence").toInteger();if(!paused_&&(sequence!=displayed_||rows_.isEmpty())){rows_=snapshot.value("rows").toArray();displayed_=sequence;render();}
    updateCounts();
}
QJsonArray BoardLogsPanel::filteredRows()const{QJsonArray result;const auto level=level_->currentData().toString(),module=module_->currentData().toString(),search=search_->text().trimmed();
    for(const auto& value:rows_){const auto row=value.toObject();const auto tag=row.value("level").toString(),name=row.value("module").toString(),text=row.value("text").toString();if(row.value("seq").toInteger()<=cleared_||(!raw_->isChecked()&&row.value("wire").toBool())||(level=="E"&&tag!="E")||(level=="warning"&&tag!="E"&&tag!="W")||(module!="all"&&module!=name)||(!search.isEmpty()&&!(name+" "+text).contains(search,Qt::CaseInsensitive)))continue;
        result.append(row);}
    return result;
}
QString BoardLogsPanel::filteredText()const{QStringList lines;for(const auto& value:filteredRows()){const auto row=value.toObject();
        lines.append(QString("%1 %2 %3 %4: %5").arg(QDateTime::fromMSecsSinceEpoch(row.value("received_ms").toInteger()).toString("MM-dd HH:mm:ss.zzz"),row.value("port").toString(),row.value("level").toString(),row.value("module").toString(),row.value("text").toString()));}
    return "来源：板端 USB / 主机采集；当前筛选的已脱敏日志（接收时间）\n"+lines.join('\n');
}
void BoardLogsPanel::updateCounts(){const auto status=bridge_->logSnapshot().value("status").toObject();QString text=QString("显示 %1 / 已采集 %2 行").arg(filteredRows().size()).arg(status.value("count").toInt());
    if(paused_)text+=QString(" · 待显示 %1").arg(qMax<qint64>(0,status.value("sequence").toInteger()-displayed_));
    QStringList diagnostics;for(const auto& item:QList<QPair<QString,QString>>{{"evicted","淘汰"},{"truncated","截断"},{"write_errors","写盘错误"},{"history_lost","历史缺失"}})if(status.value(item.first).toInteger()>0)diagnostics << QString("%1 %2").arg(item.second).arg(status.value(item.first).toInteger());
    if(!diagnostics.isEmpty())text+='\n'+diagnostics.join(" · ");counts_->setText(text);
}
void BoardLogsPanel::render(){const QScopedValueRollback<bool> rendering(rendering_,true);const int scroll=logs_->verticalScrollBar()->value();logs_->setUpdatesEnabled(false);logs_->clear();QTextCursor cursor(logs_->document());cursor.setBlockFormat(QTextBlockFormat());
    const auto rows=filteredRows();int index=0;
    for(const auto& value:rows){const auto row=value.toObject();const auto level=row.value("level").toString();const QColor color=level=="E"?QColor("#f296a0"):level=="W"?QColor("#e8bf76"):QColor("#dde2ed");
        if(index++)cursor.insertBlock();QTextBlockFormat paragraph;paragraph.setTopMargin(7);paragraph.setBottomMargin(7);paragraph.setLineHeight(130,QTextBlockFormat::ProportionalHeight);paragraph.setProperty(severityProperty,level);cursor.setBlockFormat(paragraph);
        QTextCharFormat style;style.setFontFamilies({"Consolas","Microsoft YaHei"});style.setFontPointSize(10);style.setForeground(QColor("#8b93a7"));
        cursor.insertText(QDateTime::fromMSecsSinceEpoch(row.value("received_ms").toInteger()).toString("HH:mm:ss")+"  ",style);
        style.setForeground(level=="E"||level=="W"?color:QColor("#8fa5c2"));style.setFontWeight(QFont::Bold);cursor.insertText(level.leftJustified(3,' ')+" ",style);
        style.setFontWeight(QFont::Normal);style.setForeground(QColor("#91a8c9"));cursor.insertText(row.value("module").toString()+": ",style);
        style.setForeground(color);cursor.insertText(row.value("text").toString(),style);
    }
    if(rows.isEmpty()){QTextCharFormat style;style.setForeground(QColor("#8b93a7"));cursor.insertText("当前筛选下没有日志。\n可以更换筛选，或等待新的板端输出。",style);}
    logs_->setUpdatesEnabled(true);updateCounts();
    logs_->verticalScrollBar()->setValue(following_&&!paused_?logs_->verticalScrollBar()->maximum():scroll);
    scrollToLatest();
}
