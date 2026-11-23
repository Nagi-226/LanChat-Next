#include "ui/settings_panel.h"
#include "app/preferences.h"
#include "app/credentials.h"
#include "app/board_connection.h"
#include "net/session.h"
#include "net/transfer_manager.h"
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QUrl>
#include <QResizeEvent>
#include <QScopedValueRollback>

namespace {
class SettingsLabel : public QLabel {
public:
    SettingsLabel(const QString& text,QWidget* parent=nullptr):QLabel(text,parent){setWordWrap(true);setTextFormat(Qt::PlainText);setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);}
    void measure(){if(measuring_||width()<1)return;QScopedValueRollback<bool> guard(measuring_,true);setMinimumHeight(0);setMaximumHeight(QWIDGETSIZE_MAX);setFixedHeight(QLabel::heightForWidth(width()));}
protected:
    void resizeEvent(QResizeEvent* event)override{QLabel::resizeEvent(event);measure();}
    void changeEvent(QEvent* event)override{QLabel::changeEvent(event);if(event->type()==QEvent::FontChange||event->type()==QEvent::StyleChange)measure();}
private:
    bool measuring_=false;
};
class SettingsScroll : public QScrollArea {
public:
    using QScrollArea::QScrollArea;
    QFrame* card=nullptr;
protected:
    void resizeEvent(QResizeEvent* event)override{QScrollArea::resizeEvent(event);if(card)card->setFixedWidth(qMax(240,qMin(760,viewport()->width()-56)));}
};
QCheckBox* check(QFormLayout* form,const QString& name,const QString& text){auto* field=new QCheckBox(text);field->setObjectName(name);form->addRow(field);return field;}
QSpinBox* spin(QFormLayout* form,const QString& name,const QString& text,int low,int high,const QString& suffix={}){auto* field=new QSpinBox;field->setObjectName(name);field->setRange(low,high);field->setSuffix(suffix);form->addRow(text,field);return field;}
QLabel* label(QFormLayout* form,const QString& name){auto* field=new SettingsLabel("");field->setObjectName(name);form->addRow(field);return field;}
QPushButton* button(QFormLayout* form,const QString& name,const QString& text){auto* field=new QPushButton(text);field->setObjectName(name);field->setCursor(Qt::PointingHandCursor);form->addRow(field);return field;}
}
QFormLayout* SettingsPanel::page(const QString& name,const QString& title,const QString& description){
    auto* area=new SettingsScroll(pages_);area->setWidgetResizable(true);area->setFrameShape(QFrame::NoFrame);area->setObjectName(name);
    auto* body=new QWidget;auto* outer=new QVBoxLayout(body);outer->setContentsMargins(28,24,28,28);outer->setSpacing(18);
    auto* heading=new QLabel(title,body);heading->setStyleSheet("font-size:23px;font-weight:600;background:transparent;");outer->addWidget(heading);
    auto* hint=new SettingsLabel(description,body);hint->setObjectName("settingsHint");outer->addWidget(hint);
    auto* card=new QFrame(body);card->setObjectName("settingsCard");card->setMaximumWidth(760);auto* form=new QFormLayout(card);form->setContentsMargins(24,24,24,24);form->setVerticalSpacing(16);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    area->card=card;outer->addWidget(card,0,Qt::AlignHCenter);outer->addStretch();area->setWidget(body);pages_->addWidget(area);return form;
}
SettingsPanel::SettingsPanel(Session* session,BoardConnection* bridge,TransferManager* transfers,QWidget* parent):QWidget(parent),session_(session),bridge_(bridge),transfers_(transfers){
    setObjectName("extendedSettingsPanel");auto* outer=new QVBoxLayout(this);outer->setContentsMargins(0,0,0,12);pages_=new QStackedWidget(this);outer->addWidget(pages_,1);feedback_=new QLabel(this);feedback_->setObjectName("settingsFeedback");feedback_->setWordWrap(true);feedback_->setContentsMargins(28,4,28,0);outer->addWidget(feedback_);
    auto* form=page("accountSettingsPage","账号与连接","查看当前登录与主机状态。账号和聊天历史由当前消息服务器管理。");
    label(form,"currentAccount");label(form,"hostedStatus");label(form,"rememberedStatus");
    connect(button(form,"connectionCheck","检测消息服务器连接"),&QPushButton::clicked,this,&SettingsPanel::testConnection);
    connect(button(form,"connectionReconnect","重新连接当前服务器"),&QPushButton::clicked,this,&SettingsPanel::reconnectRequested);
    connect(button(form,"forgetLogin","清除已记住的账号与密码"),&QPushButton::clicked,this,[this]{credentials::clear();emit preferencesChanged();refresh();feedback_->setText("已清除本机记住的账号与密码；当前登录仍有效。");});
    connect(button(form,"tailscaleGuide","查看 LAN / Tailscale 连接说明"),&QPushButton::clicked,this,[this]{feedback_->setText("同一局域网：填写消息主机的局域网地址与端口。异地：双方安装并登录 Tailscale，由主机所有者授予访问权限，再连接主机的 100.x 地址。只在本机检测 TCP 连接无法证明第二台 PC 的异地链路已通过。");});
    connect(button(form,"logoutAccount","退出当前账号"),&QPushButton::clicked,this,&SettingsPanel::logoutRequested);

    form=page("notificationSettingsPage","通知与窗口","通知只针对新收到的消息；正在查看的会话不弹重复提醒。关闭到托盘可保持消息主机和板端桥运行。");
    check(form,"notify_direct","私聊消息通知");check(form,"notify_group","群聊消息通知");check(form,"notify_board","板端告警与回复通知");check(form,"sound","收到通知时播放系统提示音");check(form,"do_not_disturb","免打扰（关闭通知与提示音）");check(form,"close_to_tray","关闭窗口时收起到托盘");check(form,"startup","登录 Windows 后启动 LanChat");
    connect(button(form,"notificationTest","发送一条本机测试通知"),&QPushButton::clicked,this,&SettingsPanel::testNotificationRequested);
    connect(button(form,"saveNotifications","保存通知与窗口设置"),&QPushButton::clicked,this,[this]{save(1);});

    form=page("storageSettingsPage","附件与存储","完成的附件同时保留在下载目录。缓存按当前服务器与账号隔离；清理缓存不会删除下载文件或聊天历史。");
    auto* download=new QLineEdit;download->setObjectName("download_directory");form->addRow("默认下载目录",download);
    connect(button(form,"browseDownload","选择下载目录"),&QPushButton::clicked,this,[this,download]{const auto path=QFileDialog::getExistingDirectory(this,"选择默认下载目录",download->text());if(!path.isEmpty())download->setText(path);});
    connect(button(form,"openDownload","打开下载目录"),&QPushButton::clicked,this,[this]{const auto path=TransferManager::downloadDirectory();QDir().mkpath(path);QDesktopServices::openUrl(QUrl::fromLocalFile(path));});
    check(form,"auto_download_images","自动下载新收到的图片 / GIF");spin(form,"auto_download_mb","自动下载大小上限",1,100," MB");spin(form,"cache_mb","缓存上限",64,4096," MB");label(form,"cacheUsage");
    connect(button(form,"clearAttachmentCache","清理当前账号的附件缓存"),&QPushButton::clicked,this,[this]{if(QMessageBox::question(this,"清理缓存","将已完成的附件缓存移入回收站？下载文件与未完成传输会保留。",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;const int n=transfers_->clearCache();feedback_->setText(QString("已清理 %1 个缓存文件，未完成传输保留。").arg(n));refresh();});
    connect(button(form,"saveStorage","保存附件与存储设置"),&QPushButton::clicked,this,[this]{save(2);});

    form=page("aiSettingsPage","AI 与会话","当前模型由本机板端桥调用。次数与 token 为本机记录，实际余额和费用请查看服务商平台。");
    label(form,"aiUsage");label(form,"aiError");spin(form,"aiMaxTokens","单次输出 token 上限",128,4096);spin(form,"aiTimeout","请求超时",5,120," 秒");auto* limit=spin(form,"aiDailyLimit","每日调用次数上限",0,100000);limit->setSpecialValueText("不限制");spin(form,"aiInterval","全局调用间隔",0,120," 秒");spin(form,"aiConcurrency","最大并发问题数",1,4);
    label(form,"aiLimitHint")->setText("失败请求计入次数，重启保留今日统计；429 按 Retry-After 暂停调用，不自动重试。完整文字回复仍受 4096 字节限制。关键词和日志查询不调用模型。");
    connect(button(form,"aiEndpointCheck","检测模型接口（读取模型列表）"),&QPushButton::clicked,this,&SettingsPanel::testAi);
    connect(button(form,"aiProviderSettings","修改模型渠道与密钥"),&QPushButton::clicked,this,&SettingsPanel::boardRequested);
    connect(button(form,"aiClearContext","清空我与板端的 AI 私聊上下文"),&QPushButton::clicked,this,[this]{if(session_->selfId()<1||bridge_->config().accountId<1){feedback_->setText("请先登录并启动板端桥。");return;}session_->sendPrivate(bridge_->config().accountId,"/AI 清空");feedback_->setText("清空请求已提交，结果会回到私聊；LanChat 聊天记录保留。");});
    connect(button(form,"saveAiLimits","保存 AI 限额并应用"),&QPushButton::clicked,this,[this]{save(3);});

    form=page("displaySettingsPage","显示与设备","字体缩放即时生效。板端默认页面在支持页面控制的固件连接后应用。");
    spin(form,"font_scale","界面字体缩放",85,150," %");auto* pageChoice=new QComboBox;pageChoice->setObjectName("board_default_page");const QStringList names={"保持板端当前页面","总览","收件箱","PPM 感知","VVOS 链路","设备状态"},values={"","home","inbox","ppm","link","device"};for(int i=0;i<names.size();++i)pageChoice->addItem(names[i],values[i]);form->addRow("连接后默认页面",pageChoice);
    check(form,"bridge_reconnect","板端桥意外退出后自动重连");check(form,"log_follow","日志自动跟随最新输出");auto* level=new QComboBox;level->setObjectName("log_level");level->addItem("全部","all");level->addItem("警告与错误","warning");level->addItem("仅错误","E");form->addRow("日志默认筛选",level);check(form,"log_capture","采集板端 USB 日志");
    label(form,"displayTodo")->setText("鲸鱼娘启动展示 / 沉浸模式 / 动态语音回答已记入后续待办，当前仍使用五页项目终端。");
    connect(button(form,"saveDisplay","保存显示与设备设置"),&QPushButton::clicked,this,[this]{save(4);});

    form=page("aboutSettingsPage","关于与诊断","诊断只包含运行状态，不包含密码、API 密钥、聊天正文或板端原始报文。");
    label(form,"versionInfo")->setText("LanChat-Next · 2026.11.23.2\n配套 Console · VehicleStack bridge v1\n板端 RoboMind-S3 · ESP-IDF 6.0.1（已安装固件基线，运行状态需连接后确认）");label(form,"diagnosticSummary");
    connect(button(form,"copyDiagnostics","复制脱敏连接诊断"),&QPushButton::clicked,this,[this]{QApplication::clipboard()->setText(diagnosticText());feedback_->setText("已复制脱敏诊断。");});
    connect(button(form,"exportDiagnostics","导出脱敏诊断文件"),&QPushButton::clicked,this,[this]{const auto path=QFileDialog::getSaveFileName(this,"导出脱敏诊断","LanChat-连接诊断.json","JSON (*.json)");if(path.isEmpty())return;QSaveFile file(path);const auto data=diagnosticText().toUtf8();feedback_->setText(file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit()?"已导出脱敏诊断。":"导出失败。");});
    connect(button(form,"aboutLogs","查看 / 导出板端日志"),&QPushButton::clicked,this,&SettingsPanel::boardRequested);
    connect(button(form,"updateSource","打开官方 GitHub 发布页"),&QPushButton::clicked,this,[]{QDesktopServices::openUrl(QUrl("https://github.com/Nagi-226/LanChat-Next/releases"));});
    connect(bridge_,&BoardConnection::changed,this,[this]{findChild<QLabel*>("versionInfo")->setText("LanChat-Next · 2026.11.23.2\nConsole · "+bridge_->snapshot().value("console_version").toString("尚未运行或旧版本未上报")+"\n板端终端基线 · ESP-IDF 6.0.1，实时状态需连接后确认");});
    auto* timer=new QTimer(this);timer->setInterval(1000);connect(timer,&QTimer::timeout,this,&SettingsPanel::refresh);timer->start();connect(bridge_,&BoardConnection::changed,this,&SettingsPanel::refresh);connect(transfers_,&TransferManager::changed,this,&SettingsPanel::refresh);
    connect(&probe_,&QTcpSocket::connected,this,[this]{feedback_->setText("消息服务器 TCP 连接成功；当前登录状态请看账号信息。");probe_.disconnectFromHost();});connect(&probe_,&QTcpSocket::errorOccurred,this,[this]{feedback_->setText("消息服务器连接失败："+probe_.errorString());});setSection(0);
}
void SettingsPanel::setSection(int section){if(section<0||section>5)return;pages_->setCurrentIndex(section);feedback_->clear();QSettings s;
    for(const auto& key:QStringList{"notify_direct","notify_group","notify_board","sound","do_not_disturb","close_to_tray","auto_download_images","bridge_reconnect","log_follow"})findChild<QCheckBox*>(key)->setChecked(s.value("preferences/"+key,key.startsWith("notify_")||key=="bridge_reconnect"||key=="log_follow").toBool());
    findChild<QCheckBox*>("startup")->setChecked(preferences::startupEnabled());findChild<QLineEdit*>("download_directory")->setText(TransferManager::downloadDirectory());findChild<QSpinBox*>("auto_download_mb")->setValue(s.value("preferences/auto_download_mb",10).toInt());findChild<QSpinBox*>("cache_mb")->setValue(s.value("preferences/cache_mb",512).toInt());findChild<QSpinBox*>("font_scale")->setValue(s.value("preferences/font_scale",100).toInt());
    for(const auto& key:QStringList{"board_default_page","log_level"}){auto* combo=findChild<QComboBox*>(key);combo->setCurrentIndex(qMax(0,combo->findData(s.value("preferences/"+key,key=="log_level"?"all":""))));}
    const auto& c=bridge_->config();findChild<QCheckBox*>("log_capture")->setChecked(c.logEnabled);findChild<QSpinBox*>("aiMaxTokens")->setValue(c.aiMaxTokens);findChild<QSpinBox*>("aiTimeout")->setValue(c.aiTimeoutSeconds);findChild<QSpinBox*>("aiDailyLimit")->setValue(c.aiDailyLimit);findChild<QSpinBox*>("aiInterval")->setValue(c.aiIntervalSeconds);findChild<QSpinBox*>("aiConcurrency")->setValue(c.aiConcurrency);refresh();
}
void SettingsPanel::save(int section){QSettings s;QStringList checks,numbers;
    if(section==1){checks={"notify_direct","notify_group","notify_board","sound","do_not_disturb","close_to_tray"};const bool startup=findChild<QCheckBox*>("startup")->isChecked();if(startup!=preferences::startupEnabled()&&!preferences::setStartupEnabled(startup)){feedback_->setText("Windows 开机启动设置写入失败，其他设置尚未保存。");return;}}
    if(section==2){const auto path=findChild<QLineEdit*>("download_directory")->text().trimmed();if(!QDir::isAbsolutePath(path)||!QDir().mkpath(path)||!QFileInfo(path).isWritable()){feedback_->setText("请选择可写入的绝对下载目录。");return;}s.setValue("preferences/download_directory",QDir::cleanPath(path));checks={"auto_download_images"};numbers={"auto_download_mb","cache_mb"};}
    if(section==4){checks={"bridge_reconnect","log_follow"};numbers={"font_scale"};for(const auto& key:QStringList{"board_default_page","log_level"})s.setValue("preferences/"+key,findChild<QComboBox*>(key)->currentData());}
    for(const auto& key:checks)s.setValue("preferences/"+key,findChild<QCheckBox*>(key)->isChecked());for(const auto& key:numbers)s.setValue("preferences/"+key,findChild<QSpinBox*>(key)->value());s.sync();
    if(section==3){auto c=bridge_->config();c.aiMaxTokens=findChild<QSpinBox*>("aiMaxTokens")->value();c.aiTimeoutSeconds=findChild<QSpinBox*>("aiTimeout")->value();c.aiDailyLimit=findChild<QSpinBox*>("aiDailyLimit")->value();c.aiIntervalSeconds=findChild<QSpinBox*>("aiInterval")->value();c.aiConcurrency=findChild<QSpinBox*>("aiConcurrency")->value();const bool running=bridge_->running();bridge_->configure(c);if(running)bridge_->start();}
    if(section==4){bridge_->applyPreferences();const auto& c=bridge_->config();if(c.logEnabled!=findChild<QCheckBox*>("log_capture")->isChecked())bridge_->setLogCapture(findChild<QCheckBox*>("log_capture")->isChecked(),c.logPort);preferences::applyDisplay();}
    emit preferencesChanged();feedback_->setText("设置已保存并应用。");refresh();
}
void SettingsPanel::refresh(){findChild<QLabel*>("currentAccount")->setText(QString("当前账号：%1 · #%2\n消息主机：%3").arg(session_->selfNick()).arg(session_->selfId()).arg(session_->endpoint()));findChild<QLabel*>("hostedStatus")->setText(hostState_);findChild<QLabel*>("rememberedStatus")->setText(credentials::load().remembered?"本机已记住登录信息（Windows 加密保存）":"本机未记住密码");
    int pending=0;for(const auto& t:transfers_->tasks())if(t.state!="已完成")++pending;findChild<QLabel*>("cacheUsage")->setText(QString("当前账号缓存：%1 MB\n未完成传输：%2 项（清理时保留分块）").arg(transfers_->cacheBytes()/1048576.0,0,'f',1).arg(pending));
    const auto ai=bridge_->snapshot().value("ai").toObject();findChild<QLabel*>("aiUsage")->setText(ai.isEmpty()?"板端桥未运行，无法读取实时 AI 统计。":QString("模型：%1\n今日尝试 %2 / %3 次 · 输入 %4 / 输出 %5 token\n本次进程成功 %6 / 失败 %7 · 待回答 %8").arg(ai.value("model").toString()).arg(ai.value("daily_calls").toInteger()).arg(ai.value("daily_limit").toInt()==0?QString("不限"):QString::number(ai.value("daily_limit").toInt())).arg(ai.value("input_tokens").toInteger()).arg(ai.value("output_tokens").toInteger()).arg(ai.value("replied").toInteger()).arg(ai.value("failed").toInteger()).arg(ai.value("pending").toInt()));findChild<QLabel*>("aiError")->setText(QString("%1%2").arg(ai.value("last_error").toString()).arg(ai.value("cooldown_seconds").toInteger()>0?QString("\n冷却剩余 %1 秒").arg(ai.value("cooldown_seconds").toInteger()):QString()));findChild<QLabel*>("diagnosticSummary")->setText(QString("板端：%1 · 模型 HTTP：%2\nUSB 日志：%3").arg(bridge_->snapshot().value("board_connected").toBool()?"已连接":"离线 / 未运行").arg(ai.value("last_http").toInt()).arg(bridge_->logSnapshot().value("status").toObject().value("state").toString("未运行")));
    for(auto* field:findChildren<QLabel*>())if(auto* measured=dynamic_cast<SettingsLabel*>(field))measured->measure();
}
QString SettingsPanel::diagnosticText()const{return preferences::diagnostics(bridge_->snapshot(),session_->state()+" · "+session_->endpoint(),hostState_,session_->selfId());}
void SettingsPanel::testConnection(){probe_.abort();QSettings s;const auto host=s.value("network/hosted",true).toBool()?QString("127.0.0.1"):s.value("network/host").toString();probe_.connectToHost(host,quint16(s.value("network/port",12346).toInt()));feedback_->setText("正在检测 TCP 连接…");QTimer::singleShot(5000,this,[this]{if(probe_.state()==QAbstractSocket::ConnectingState){probe_.abort();feedback_->setText("连接检测超时，请检查主机地址、网络与防火墙。");}});}
void SettingsPanel::testAi(){const auto c=bridge_->config();QUrl url(c.aiEndpoint);const auto host=url.host().toLower();if(c.aiKey.isEmpty()||url.userInfo().size()||url.hasQuery()||url.hasFragment()||(url.scheme()!="https"&&!(url.scheme()=="http"&&QStringList{"127.0.0.1","localhost","::1"}.contains(host)))){feedback_->setText("请先配置有效的 HTTPS 模型接口与密钥。");return;}
    QString path=url.path();while(path.endsWith('/'))path.chop(1);for(const auto& suffix:QStringList{"/chat/completions","/messages"})if(path.endsWith(suffix))path.chop(suffix.size());url.setPath(path+(path.endsWith("/v1")?"/models":"/v1/models"));QNetworkRequest request(url);request.setRawHeader("Authorization","Bearer "+c.aiKey.toUtf8());if(c.aiProvider=="anthropic"){request.setRawHeader("x-api-key",c.aiKey.toUtf8());request.setRawHeader("anthropic-version","2023-06-01");}request.setTransferTimeout(10000);request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    auto* action=findChild<QPushButton*>("aiEndpointCheck");action->setEnabled(false);feedback_->setText("读取模型列表，不发送聊天或板端状态，也不生成回答…");auto* reply=network_.get(request);reply->setReadBufferSize(262145);connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>262144)reply->abort();});QTimer::singleShot(10000,reply,&QNetworkReply::abort);
    connect(reply,&QNetworkReply::finished,this,[this,reply,action,c]{const int code=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();const auto bytes=reply->readAll();if(code==200&&reply->error()==QNetworkReply::NoError&&bytes.size()<=262144){bool found=false;for(const auto& value:QJsonDocument::fromJson(bytes).object().value("data").toArray())if(value.toObject().value("id").toString()==c.aiModel)found=true;feedback_->setText(found?"模型列表读取成功，已找到配置的模型。文字调用额度仍以服务商平台为准。":"接口可达，但模型列表未找到配置的 ID；部分渠道不提供完整列表。");}else feedback_->setText(code==429?"模型列表也返回 HTTP 429，请检查服务商限流、余额与账户权限。":code==401||code==403?QString("模型列表 HTTP %1，请检查密钥与权限。").arg(code):code==404||code==405?"服务商不支持模型列表检测，不能据此判断文字接口不可用。":QString("模型列表检测失败（HTTP %1），请检查连接与地址。").arg(code));action->setEnabled(true);reply->deleteLater();});}
