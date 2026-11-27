#include "ui/chat_view.h"
#include "ui/avatar.h"

#include <QDateTime>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMovie>
#include <QDialog>
#include <QProgressBar>
#include <QTimer>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QMessageBox>
#include <QSaveFile>
#include <QResizeEvent>

namespace {
// Nested layouts can estimate a limited-width label at its parent's wider size.
// Measure again at the actual painted width so the last lines are not clipped.
class MessageText : public QLabel {
public:
    MessageText(const QString& text,QWidget* parent):QLabel(text,parent) {
        setTextFormat(Qt::PlainText);setWordWrap(true);
        setTextInteractionFlags(Qt::TextSelectableByMouse);
        setAlignment(Qt::AlignLeft|Qt::AlignTop);
        setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    }
protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        if(event->size().width()!=event->oldSize().width())measureHeight();
    }
    void changeEvent(QEvent* event) override {
        QLabel::changeEvent(event);
        if(event->type()==QEvent::FontChange||event->type()==QEvent::StyleChange)measureHeight();
    }
private:
    void measureHeight() {
        if(measuring_)return;measuring_=true;
        // Clear the previous constraint first so widening can also reduce height.
        setMinimumHeight(0);setMaximumHeight(QWIDGETSIZE_MAX);
        setFixedHeight(QLabel::heightForWidth(qMax(1,width())));updateGeometry();
        measuring_=false;
    }
    bool measuring_=false;
};
}

ChatView::ChatView(QWidget* parent) : QWidget(parent)
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_StyledBackground, true);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // 会话头 56px
    auto* header = new QWidget(this);
    header->setFixedHeight(56);
    header->setAttribute(Qt::WA_StyledBackground, true);
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(24, 0, 24, 0);
    peer_ = new QLabel("选择一个联系人开始聊天", header);
    peer_->setTextFormat(Qt::PlainText);
    peer_->setObjectName("headerTitle");
    hl->addWidget(peer_);
    hl->addStretch(1);
    history_=new QPushButton("更早消息",header);history_->hide();hl->addWidget(history_);
    connect(history_,&QPushButton::clicked,this,&ChatView::historyRequested);
    leave_=new QPushButton("退出群",header);leave_->hide();hl->addWidget(leave_);
    connect(leave_,&QPushButton::clicked,this,&ChatView::leaveRequested);
    lay->addWidget(header);
    auto* div = new QFrame(this);
    div->setObjectName("divider");
    div->setFixedHeight(1);
    lay->addWidget(div);

    // 消息区（可滚动）
    scroll_ = new QScrollArea(this);
    scroll_->setObjectName("chatMessagesScroll");
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    msgBody_ = new QWidget;
    msgBody_->setAttribute(Qt::WA_StyledBackground, true);
    msgLay_ = new QVBoxLayout(msgBody_);
    msgLay_->setContentsMargins(0, 8, 0, 8);
    msgLay_->setSpacing(0);
    // 空状态
    empty_ = new QWidget(msgBody_);
    auto* el = new QVBoxLayout(empty_);
    el->setAlignment(Qt::AlignCenter);
    el->setSpacing(12);
    auto* icon = new QLabel(QStringLiteral("✉"), empty_);
    QFont inf = icon->font();
    inf.setPixelSize(48);
    icon->setFont(inf);
    icon->setAlignment(Qt::AlignCenter);
    auto* eff = new QGraphicsOpacityEffect(icon);
    eff->setOpacity(0.25);
    icon->setGraphicsEffect(eff);
    icon->setStyleSheet("color:#66707F;");
    auto* hint = new QLabel("还没有消息，打个招呼吧", empty_);
    hint->setObjectName("emptyHint");
    hint->setAlignment(Qt::AlignCenter);
    el->addWidget(icon);
    el->addWidget(hint);
    msgLay_->addWidget(empty_);
    msgLay_->addStretch(1);
    scroll_->setWidget(msgBody_);
    lay->addWidget(scroll_, 1);

    transfersBody_=new QWidget(this);transfersBody_->setMaximumHeight(160);auto* transfersLayout=new QVBoxLayout(transfersBody_);transfersLayout->setContentsMargins(24,4,24,4);transfersLayout->setSpacing(2);transfersBody_->hide();lay->addWidget(transfersBody_);
    // 输入区
    auto* inputWrap = new QWidget(this);
    inputWrap->setAttribute(Qt::WA_StyledBackground, true);
    auto* il = new QHBoxLayout(inputWrap);
    il->setContentsMargins(24, 12, 24, 16);
    il->setSpacing(8);
    input_ = new QLineEdit(inputWrap);
    input_->setPlaceholderText("输入文字 / Unicode 表情…（Enter 发送）");input_->installEventFilter(this);
    auto* attach=new QPushButton("附件",inputWrap);attach->setObjectName("attachFileButton");
    connect(attach,&QPushButton::clicked,this,[this]{auto paths=QFileDialog::getOpenFileNames(this,"发送图片或文件");if(!paths.isEmpty())emit filesRequested(paths);});il->addWidget(attach);
    auto* send = new QPushButton("发送", inputWrap);
    send->setObjectName("primary");
    il->addWidget(input_, 1);
    il->addWidget(send);
    lay->addWidget(inputWrap);

    connect(send, &QPushButton::clicked, this, [this] {
        const QString t = input_->text().trimmed();
        if (t.isEmpty()) return;
        input_->clear();
        emit sendRequested(t);
    });
    connect(input_, &QLineEdit::returnPressed, send, &QPushButton::click);
}

void ChatView::setSelfId(int id) { selfId_ = id; }

void ChatView::setPeerLabel(const QString& text) { peer_->setText(text); }

void ChatView::appendMessage(int fromId, const QString& senderName, const QString& text,
                             qint64 ts, bool grouped, int alertLevel, int avatarId)
{
    empty_->hide();
    const bool mine = (fromId == selfId_);
    auto* row = new QWidget(msgBody_);
    row->setAttribute(Qt::WA_StyledBackground, true);
    auto* h = new QHBoxLayout(row);
    h->setContentsMargins(24, grouped ? 2 : 12, 24, 0);
    h->setSpacing(8);
    if (mine) h->setDirection(QBoxLayout::RightToLeft);

    if (!grouped) h->addWidget(new Avatar(senderName, 32, row, avatarId));

    auto* col = new QVBoxLayout;
    col->setSpacing(4);
    if (!grouped) {
        auto* nameRow = new QHBoxLayout;
        nameRow->setSpacing(8);
        auto* nm = new QLabel(mine ? "我" : senderName, row);
        nm->setTextFormat(Qt::PlainText);
        nm->setObjectName("msgName");
        auto* tm = new QLabel(QDateTime::fromMSecsSinceEpoch(ts).toString("HH:mm"), row);
        tm->setObjectName("timestamp");
        nameRow->addWidget(nm);
        nameRow->addWidget(tm);
        nameRow->addStretch(1);
        col->addLayout(nameRow);
    }
    if (alertLevel > 0) {
        // 板端告警卡片：左 3px 严重度色条 + 级别徽章 + 正文，一眼区别于人聊的气泡
        auto* card = new QWidget(row);
        card->setObjectName("alertCard");
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setProperty("level", alertLevel);
        card->setMaximumWidth(520);
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(12, 8, 12, 8);
        cl->setSpacing(4);
        auto* head = new QHBoxLayout;
        head->setSpacing(8);
        auto* chip = new QLabel(alertLevel >= 3 ? "严重" : alertLevel == 2 ? "警告" : "提示", card);
        chip->setObjectName(alertLevel >= 3 ? "chipDanger" : alertLevel == 2 ? "chipWarn" : "chipInfo");
        head->addWidget(chip);
        auto* src = new QLabel("板端 · " + senderName, card);
        src->setObjectName("timestamp");
        head->addWidget(src);
        head->addStretch(1);
        cl->addLayout(head);
        auto* body = new MessageText(text, card);
        body->setObjectName("alertBody");body->setTextFormat(Qt::PlainText);
        body->setWordWrap(true);
        cl->addWidget(body);
        col->addWidget(card);
        h->addLayout(col, 1);
        if (mine) col->setAlignment(card, Qt::AlignRight);
    } else {
        auto* bubble = new MessageText(text, row);
        bubble->setObjectName(mine ? "bubbleSelf" : "bubblePeer");
        bubble->setTextFormat(Qt::PlainText);bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bubble->setWordWrap(true);
        bubble->setMaximumWidth(520);
        bubble->setAttribute(Qt::WA_StyledBackground, true);
        col->addWidget(bubble);
        h->addLayout(col, 1);
        // 让气泡靠发送者一侧：col 内 bubble 对齐
        // （RightToLeft 已处理整行方向；气泡在 col 内默认左对齐，self 需右对齐）
        if (mine) col->setAlignment(bubble, Qt::AlignRight);
    }

    // 插到 stretch 之前
    msgLay_->insertWidget(msgLay_->count() - 1, row);
    lastFrom_ = fromId;
    lastTs_ = ts;
    scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum());
    // Layout/scroll limits settle after the new wrapped label gets its real width.
    QTimer::singleShot(0,this,[this]{scroll_->verticalScrollBar()->setValue(scroll_->verticalScrollBar()->maximum());});
}

void ChatView::clear()
{
    QLayoutItem* child;
    while ((child = msgLay_->takeAt(0)) != nullptr) {
        if (child->widget() && child->widget() != empty_) child->widget()->deleteLater();
        else if (child->widget() == empty_) { /* keep */ }
        delete child;
    }
    msgLay_->addWidget(empty_);
    msgLay_->addStretch(1);
    empty_->show();
    lastFrom_ = -1;
    lastTs_ = 0;
}

void ChatView::updateEmptyState()
{
    empty_->setVisible(msgLay_->count() <= 2);
}
void ChatView::setGroup(bool group){leave_->setVisible(group);}
void ChatView::setHistoryAvailable(bool available){history_->setVisible(available);}
void ChatView::appendAttachment(int from,const QString& sender,const QJsonObject& a,qint64 ts,const QString& path){
    empty_->hide();auto* row=new QWidget(msgBody_);auto* line=new QHBoxLayout(row);line->setContentsMargins(24,12,24,0);
    if(from==selfId_)line->addStretch();auto* card=new QWidget(row);card->setObjectName("attachmentCard");card->setMaximumWidth(440);
    card->setProperty("attachment",a);card->setProperty("sender",sender+" · "+QDateTime::fromMSecsSinceEpoch(ts).toString("HH:mm"));
    new QVBoxLayout(card);populateAttachment(card,a,path);line->addWidget(card);if(from!=selfId_)line->addStretch();msgLay_->insertWidget(msgLay_->count()-1,row);
}
void ChatView::populateAttachment(QWidget* card,const QJsonObject& a,const QString& path){
    auto* layout=qobject_cast<QVBoxLayout*>(card->layout());while(auto* item=layout->takeAt(0)){if(item->widget())item->widget()->deleteLater();delete item;}
    auto* title=new QLabel(card->property("sender").toString(),card);title->setTextFormat(Qt::PlainText);title->setObjectName("msgName");layout->addWidget(title);
    auto* name=new QLabel(a["file_name"].toString(),card);name->setTextFormat(Qt::PlainText);name->setWordWrap(true);layout->addWidget(name);
    const auto kind=a.value("content_type").toString("file");const QString kindLabel=kind=="image"?"图片":kind=="sticker"?"GIF 动图":"文件";
    auto* size=new QLabel(QString("%1 MB · %2").arg(a["file_size"].toDouble()/1024/1024,0,'f',2).arg(kindLabel),card);size->setObjectName("timestamp");layout->addWidget(size);
    if(path.isEmpty()){auto* button=new QPushButton("下载 / 预览",card);button->setObjectName("downloadAttachmentButton");layout->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this,a]{emit attachmentRequested(a);});return;}
    QImageReader reader(path);auto dimensions=reader.size();bool viewable=dimensions.isValid()&&qint64(dimensions.width())*dimensions.height()<=16000000;
    if(viewable && a.value("content_type").toString()!="file"){
        auto* preview=new QLabel(card);preview->setObjectName("attachmentPreview");preview->setMaximumSize(320,240);preview->setScaledContents(false);
        if(reader.supportsAnimation()){auto* movie=new QMovie(path,QByteArray(),preview);movie->setCacheMode(QMovie::CacheNone);movie->setScaledSize(dimensions.scaled(320,240,Qt::KeepAspectRatio));preview->setMovie(movie);movie->start();}
        else {reader.setScaledSize(dimensions.scaled(320,240,Qt::KeepAspectRatio));preview->setPixmap(QPixmap::fromImage(reader.read()));}layout->addWidget(preview);
        auto* original=new QPushButton("查看原图",card);layout->addWidget(original);connect(original,&QPushButton::clicked,this,[this,path]{
            auto* dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("图片预览");auto* lay=new QVBoxLayout(dialog);auto* scroll=new QScrollArea(dialog);auto* img=new QLabel;
            QImageReader r(path);img->setPixmap(QPixmap::fromImage(r.read()));scroll->setWidget(img);lay->addWidget(scroll);dialog->resize(900,650);dialog->show();});
    }
    auto* save=new QPushButton("另存为…",card);layout->addWidget(save);connect(save,&QPushButton::clicked,this,[this,path,a]{
        auto target=QFileDialog::getSaveFileName(this,"保存附件",QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)+"/"+QFileInfo(a["file_name"].toString().replace('\\','/')).fileName());
        if(target.isEmpty()||QFileInfo(target)==QFileInfo(path))return;
        QFile source(path);if(!source.open(QIODevice::ReadOnly))return;QSaveFile dest(target);if(!dest.open(QIODevice::WriteOnly)){QMessageBox::information(this,"保存附件","无法写入目标文件");return;}
        bool ok=true;while(!source.atEnd()){auto data=source.read(64*1024);if(data.isEmpty()||dest.write(data)!=data.size()){ok=false;break;}}
        if(ok)ok=dest.commit();else dest.cancelWriting();if(!ok)QMessageBox::information(this,"保存附件","保存失败，请检查磁盘空间");
    });
}
void ChatView::attachmentReady(const QString& id,const QString& path){for(auto* card:msgBody_->findChildren<QWidget*>("attachmentCard")){auto a=card->property("attachment").toJsonObject();if(a["attachment_id"].toString()==id)populateAttachment(card,a,path);}}
void ChatView::setTransfers(const QList<TransferTask>& tasks){
    auto* layout=qobject_cast<QVBoxLayout*>(transfersBody_->layout());while(auto* item=layout->takeAt(0)){if(item->widget())item->widget()->deleteLater();delete item;}
    int count=0;for(const auto& t:tasks){if(t.state=="已完成")continue;if(++count>4)break;auto* row=new QWidget(transfersBody_);auto* line=new QHBoxLayout(row);line->setContentsMargins(0,0,0,0);
        auto* text=new QLabel((t.upload?"上传 ":"下载 ")+(t.upload?QFileInfo(t.path).fileName():t.attachment["file_name"].toString())+" · "+t.state,row);text->setTextFormat(Qt::PlainText);text->setMaximumWidth(250);line->addWidget(text);
        auto* bar=new QProgressBar(row);bar->setRange(0,1000);bar->setValue(t.size?int(t.offset*1000/t.size):0);bar->setFormat("%p%");line->addWidget(bar,1);
        auto* action=new QPushButton(t.active?"暂停":"继续",row);line->addWidget(action);connect(action,&QPushButton::clicked,this,[this,t]{emit transferAction(t.id,t.active?"pause":"resume");});
        auto* cancel=new QPushButton("取消",row);line->addWidget(cancel);connect(cancel,&QPushButton::clicked,this,[this,t]{emit transferAction(t.id,"cancel");});layout->addWidget(row);
    }transfersBody_->setVisible(count>0);
}
bool ChatView::eventFilter(QObject* object,QEvent* event){
    if(object==input_&&event->type()==QEvent::KeyPress){auto* key=static_cast<QKeyEvent*>(event);if(key->matches(QKeySequence::Paste)){
        const auto* mime=QApplication::clipboard()->mimeData();QStringList files;for(auto url:mime->urls())if(url.isLocalFile())files.append(url.toLocalFile());
        if(!files.isEmpty()){emit filesRequested(files);return true;}
        if(mime->hasImage()){auto image=qvariant_cast<QImage>(mime->imageData());if(!image.isNull()){
            auto directory=QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/clipboard/"+QUuid::createUuid().toString(QUuid::WithoutBraces);QDir().mkpath(directory);auto path=directory+"/剪贴板图片-"+QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss")+".png";
            if(image.save(path))emit filesRequested({path});return true;}}
    }}return QWidget::eventFilter(object,event);
}
void ChatView::dragEnterEvent(QDragEnterEvent* event){if(event->mimeData()->hasUrls())event->acceptProposedAction();}
void ChatView::dropEvent(QDropEvent* event){QStringList files;for(auto url:event->mimeData()->urls())if(url.isLocalFile())files.append(url.toLocalFile());if(!files.isEmpty()){emit filesRequested(files);event->acceptProposedAction();}}
