#include "ui/board_connection_panel.h"
#include "app/board_connection.h"
#include "net/session.h"
#include <QPlainTextEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QStandardItemModel>
#include <QGridLayout>
#include <QTabWidget>
#include <QTabBar>
#include <QUrl>
#include "ui/board_logs_panel.h"

BoardConnectionPanel::BoardConnectionPanel(BoardConnection* bridge, QWidget* parent, Session* session)
    : QWidget(parent), bridge_(bridge), session_(session)
{
    setObjectName("boardConnectionPanel");
    setAttribute(Qt::WA_StyledBackground,true);
    auto* centered = new QHBoxLayout(this);
    centered->setContentsMargins(0,0,0,0);
    auto* column = new QWidget(this);
    column->setMaximumWidth(960);
    column->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    centered->addStretch();
    centered->addWidget(column,1);
    centered->addStretch();
    auto* outer = new QVBoxLayout(column);
    auto* scroll = new QScrollArea(column);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    scroll->setWidget(content);
    outer->addWidget(scroll,1);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(14);
    auto* title = new QLabel("板端连接", this);
    title->setStyleSheet("font-size:22px;font-weight:600;color:#E7EAF0;background:transparent;");
    layout->addWidget(title);
    auto* description = new QLabel("板端状态与告警上行 · 本机消息下行显示\n电脑可用 5GHz，板子用同一路由器的 2.4GHz。\n私聊板端账号：感知 / 链路 / 设备 / 帮助；群聊使用 /感知 等命令。回复显示在聊天会话中。", this);
    description->setWordWrap(true);
    description->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(description);
    auto* tabs=new QTabWidget(content);tabs->setObjectName("boardConnectionTabs");
    auto* connectionPage=new QWidget(tabs);tabs->addTab(connectionPage,"连接与屏幕");
    tabs->addTab(new BoardLogsPanel(bridge_,session_,tabs),"运行日志");layout->addWidget(tabs);
    auto* connectionLayout=new QVBoxLayout(connectionPage);connectionLayout->setContentsMargins(0,12,0,0);
    layout=connectionLayout;
    status_ = new QLabel(this);
    status_->setObjectName("boardConnectionStatus");
    status_->setStyleSheet("background:#1D212A;padding:14px;border-radius:8px;font-weight:600;");
    layout->addWidget(status_);
    stats_ = new QLabel(this);
    stats_->setObjectName("boardConnectionStats");
    stats_->setWordWrap(true);
    stats_->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(stats_);
    auto* form = new QFormLayout;
    form->setVerticalSpacing(12);
    auto spin = [this](int maximum, int value, const QString& name) {
        auto* input = new QSpinBox(this);
        input->setRange(0, maximum);
        input->setValue(value);
        input->setObjectName(name);
        input->setMinimumHeight(36);
        return input;
    };
    const auto& config = bridge_->config();
    port_ = spin(65535, config.listenPort, "boardListenPort");
    port_->setMinimum(1);
    target_ = spin(2147483647, config.target, "boardTargetId");
    group_ = spin(2147483647, config.groupId, "boardGroupId");
    target_->setSpecialValueText("关闭私聊转发");
    group_->setSpecialValueText("关闭群转发");
    form->addRow("板端上报端口", port_);
    form->addRow("私聊接收账号 ID", target_);
    form->addRow("告警群 ID", group_);
    account_ = new QLabel(this);
    account_->setWordWrap(true);
    form->addRow("DeepSeek 板端账号", account_);
    auto* gptId = spin(2147483647, config.gptAccountId, "boardGptAccountId");
    gptId->setSpecialValueText("不连接 GPT 账号");
    auto* gptPassword = new QLineEdit(this);
    gptPassword->setObjectName("boardGptAccountPassword");
    gptPassword->setEchoMode(QLineEdit::Password);
    gptPassword->setPlaceholderText(config.gptPassword.isEmpty()?"填写 LanChat 账号密码（与 API Key 分开）":"已加密保存，留空保留");
    auto* gptState = new QLabel(this);
    gptState->setWordWrap(true);
    form->addRow("GPT 板端账号 ID",gptId);
    form->addRow("GPT 板端账号密码",gptPassword);
    form->addRow("GPT 单聊状态",gptState);
    const auto refreshGpt = [this,gptState] {
        const auto& current = bridge_->config();
        const auto data = bridge_->snapshot();
        const auto ai = data.value("gpt_ai").toObject();
        const auto online = data.value("gpt_lanchat_state").toString();
        if (!current.gptAccountId) gptState->setText("尚未绑定 GPT 的 LanChat 账号。");
        else if (online.isEmpty()) gptState->setText("等待 GPT 桥启动；请验证 GPT 配置并保存连接。");
        else gptState->setText(online + (ai.isEmpty()?QString(): " · " + ai.value("model").toString()));
    };
    connect(bridge_,&BoardConnection::changed,gptState,refreshGpt);
    refreshGpt();
    automatic_ = new QCheckBox("启动 LanChat 时自动连接板端桥", this);
    automatic_->setObjectName("boardAutoStart");
    automatic_->setChecked(config.autoStart);
    form->addRow(automatic_);
    layout->addLayout(form);
    auto* hint = new QLabel("两个目标均为 0 时只监测连接；同时配置时群和私聊各发一份。\n登录后，未配置的私聊目标会自动填为当前账号。", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#8A94A4;background:transparent;");
    layout->addWidget(hint);
    consolePath_ = new QLineEdit(config.consolePath, this);
    consolePath_->setReadOnly(true);
    auto* browse = new QPushButton("选择 Console", this);
    auto* executableRow = new QHBoxLayout;
    executableRow->addWidget(consolePath_, 1);
    executableRow->addWidget(browse);
    layout->addLayout(executableRow);
    auto* aiTitle = new QLabel("AI 状态问答",this);
    aiTitle->setStyleSheet("font-size:16px;font-weight:600;color:#E7EDF7;background:transparent;");
    layout->addWidget(aiTitle);
    aiEnabled_=new QCheckBox("启用文字自由问答",this);aiEnabled_->setObjectName("boardAiEnabled");aiEnabled_->setChecked(config.aiEnabled);
    layout->addWidget(aiEnabled_);
    auto* aiEditor=new QWidget(this);
    auto* aiLayout=new QVBoxLayout(aiEditor);
    aiLayout->setContentsMargins(0,0,0,0);
    auto* aiSlots=new QTabBar(aiEditor);
    aiSlots->setObjectName("boardAiProfiles");
    for(int i=0;i<BoardConnection::AiProfileCount;++i)aiSlots->addTab(BoardConnection::aiProfileName(i));
    aiLayout->addWidget(aiSlots);
    auto* aiForm=new QFormLayout;aiForm->setVerticalSpacing(10);
    aiProvider_=new QComboBox(this);aiProvider_->setObjectName("boardAiProvider");
    aiProvider_->addItem("Anthropic 兼容（含 DeepSeek）","anthropic");aiProvider_->addItem("OpenAI 兼容","openai");
    aiProvider_->setCurrentIndex(config.aiProvider=="openai"?1:0);
    aiEndpoint_=new QLineEdit(config.aiEndpoint,this);aiEndpoint_->setObjectName("boardAiEndpoint");aiEndpoint_->setPlaceholderText("输入服务商提供的接口地址");
    aiModel_=new QLineEdit(config.aiModel,this);aiModel_->setObjectName("boardAiModel");aiModel_->setPlaceholderText("服务商提供的模型名称");
    aiReasoningEffort_=new QComboBox(this);aiReasoningEffort_->setObjectName("boardAiReasoningEffort");
    aiReasoningEffort_->addItem("按模型默认","");aiReasoningEffort_->addItem("关闭思考（none）","none");
    aiReasoningEffort_->addItem("低（low）","low");aiReasoningEffort_->addItem("中（medium）","medium");
    aiReasoningEffort_->addItem("高（high）","high");aiReasoningEffort_->addItem("最高（max）","max");
    aiReasoningEffort_->setCurrentIndex(qMax(0,aiReasoningEffort_->findData(config.aiReasoningEffort)));
    aiReasoningEffort_->setEnabled(config.aiProvider=="openai");
    connect(aiProvider_,&QComboBox::currentIndexChanged,this,[this]{aiReasoningEffort_->setEnabled(aiProvider_->currentData().toString()=="openai");});
    aiReasoningEffort_->setToolTip("仅用于支持 reasoning_effort 的 OpenAI 兼容模型。商汤 deepseek-flash 选 none 可减少等待，避免思考占满输出上限。");
    aiKey_=new QLineEdit(this);aiKey_->setObjectName("boardAiKey");aiKey_->setEchoMode(QLineEdit::Password);
    aiKey_->setPlaceholderText(config.aiKey.isEmpty()?"输入 API 密钥":"密钥已加密保存，留空保留");
    aiForm->addRow("接口协议",aiProvider_);aiForm->addRow("接口地址",aiEndpoint_);aiForm->addRow("模型",aiModel_);aiForm->addRow("思考强度",aiReasoningEffort_);aiForm->addRow("API 密钥",aiKey_);aiLayout->addLayout(aiForm);
    auto* saveAi=new QPushButton("保存配置",aiEditor);saveAi->setObjectName("boardAiSaveProfile");
    auto* testAi=new QPushButton(aiEditor);testAi->setObjectName("boardAiTestProfile");
    auto* restoreAi=new QPushButton("恢复上次通过的配置",aiEditor);restoreAi->setObjectName("boardAiRestoreProfile");
    auto* profileActions=new QHBoxLayout;
    profileActions->addWidget(saveAi);profileActions->addWidget(testAi);profileActions->addWidget(restoreAi);profileActions->addStretch();
    aiLayout->addLayout(profileActions);
    auto* aiFeedback=new QLabel(aiEditor);aiFeedback->setObjectName("boardAiProfileFeedback");aiFeedback->setWordWrap(true);
    aiLayout->addWidget(aiFeedback);layout->addWidget(aiEditor);
    auto showProfile=[this,aiSlots,restoreAi,testAi,aiFeedback](int slot) {
        const auto profile=bridge_->aiProfile(slot);
        const auto verified=bridge_->aiProfile(slot,true);
        aiProvider_->setCurrentIndex(qMax(0,aiProvider_->findData(profile.provider)));
        aiEndpoint_->setText(profile.endpoint);aiModel_->setText(profile.model);
        aiReasoningEffort_->setCurrentIndex(qMax(0,aiReasoningEffort_->findData(profile.reasoningEffort)));
        aiKey_->clear();
        aiKey_->setPlaceholderText(profile.key.isEmpty()?"输入 API 密钥":"此配置的密钥已加密保存");
        const bool canRestore=!verified.endpoint.isEmpty()&&!verified.key.isEmpty();
        restoreAi->setEnabled(canRestore);
        restoreAi->setText(canRestore&&verified.verifiedAt<=0?"恢复迁移时的配置":"恢复上次通过的配置");
        testAi->setText(slot==0?"验证并应用":"验证配置");
        const QString owner=slot==0?"当前桥账号：RoboMind-S3_DS":slot==1?"独立私聊账号：RoboMind-S3_GPT":"预留配置";
        aiFeedback->setText(verified.verifiedAt>0?owner+" · 验证通过："+QDateTime::fromMSecsSinceEpoch(verified.verifiedAt).toString("yyyy-MM-dd HH:mm"):canRestore?owner+" · 已迁移当前配置":owner);
    };
    connect(aiSlots,&QTabBar::currentChanged,this,showProfile);
    showProfile(0);
    const auto readProfile=[this,aiSlots] {
        const auto existing=bridge_->aiProfile(aiSlots->currentIndex());
        const auto provider=aiProvider_->currentData().toString();
        const auto endpoint=aiEndpoint_->text().trimmed();
        const auto key=aiKey_->text().trimmed();
        return BoardConnection::AiProfile{provider,endpoint,aiModel_->text().trimmed(),
            key.isEmpty()&&existing.endpoint==endpoint&&existing.provider==provider?existing.key:key,
            provider=="openai"?aiReasoningEffort_->currentData().toString():QString()};
    };
    connect(saveAi,&QPushButton::clicked,this,[this,aiSlots,readProfile,showProfile,aiFeedback] {
        const int slot=aiSlots->currentIndex();
        if(!bridge_->saveAiProfile(slot,readProfile())){aiFeedback->setText("无法保存此配置。");return;}
        showProfile(slot);aiFeedback->setText("配置已保存；当前运行的模型没有改动。");
    });
    connect(restoreAi,&QPushButton::clicked,this,[this,aiSlots,showProfile,aiFeedback] {
        const int slot=aiSlots->currentIndex();
        const auto profile=bridge_->aiProfile(slot,true);
        if(profile.endpoint.isEmpty()||profile.key.isEmpty()||!bridge_->saveAiProfile(slot,profile)){aiFeedback->setText("无法恢复此配置。");return;}
        showProfile(slot);aiFeedback->setText("已恢复表单；当前桥的运行配置没有改动。");
    });
    auto* probe=new QNetworkAccessManager(aiEditor);
    connect(testAi,&QPushButton::clicked,this,[this,aiSlots,aiEditor,probe,aiFeedback,readProfile,showProfile] {
        const int slot=aiSlots->currentIndex();
        auto profile=readProfile();
        const QUrl url(profile.endpoint);
        const bool local=url.host()=="127.0.0.1"||url.host()=="localhost"||url.host()=="::1";
        if(!url.isValid()||url.host().isEmpty()||!url.userInfo().isEmpty()||url.hasFragment()||url.hasQuery()
           ||(url.scheme()!="https"&&!(local&&url.scheme()=="http"))||profile.model.isEmpty()||profile.key.isEmpty()){
            aiFeedback->setText("请输入有效的接口地址、模型和此地址的 API 密钥。地址改变后需重新填写密钥。");return;
        }
        if(!bridge_->saveAiProfile(slot,profile)){aiFeedback->setText("无法加密保存配置。");return;}
        const bool anthropic=profile.provider=="anthropic";
        const QString suffix=anthropic?"/messages":"/chat/completions";
        QString resolved=profile.endpoint;
        while(resolved.endsWith('/'))resolved.chop(1);
        if(!resolved.endsWith(suffix))resolved+=(resolved.endsWith("/v1")?suffix:"/v1"+suffix);
        QNetworkRequest request{QUrl(resolved)};
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
        request.setRawHeader("Authorization","Bearer "+profile.key.toUtf8());
        if(anthropic)request.setRawHeader("anthropic-version","2023-06-01");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
        request.setTransferTimeout(25000);
        QJsonObject payload{{"model",profile.model},{"max_tokens",qMin(bridge_->config().aiMaxTokens,512)},
            {"stream",false},{"messages",QJsonArray{QJsonObject{{"role","user"},{"content","Reply with OK."}}}}};
        if(!anthropic&&!profile.reasoningEffort.isEmpty())payload.insert("reasoning_effort",profile.reasoningEffort);
        auto* reply=probe->post(request,QJsonDocument(payload).toJson(QJsonDocument::Compact));
        auto* deadline=new QTimer(reply);deadline->setSingleShot(true);
        connect(deadline,&QTimer::timeout,reply,&QNetworkReply::abort);deadline->start(25000);
        reply->setReadBufferSize(65537);
        connect(reply,&QNetworkReply::readyRead,reply,[reply]{if(reply->bytesAvailable()>65536)reply->abort();});
        aiEditor->setEnabled(false);aiFeedback->setText("正在请求一条测试回复…");
        connect(reply,&QNetworkReply::finished,this,[this,reply,slot,profile,anthropic,aiEditor,aiFeedback,showProfile]() mutable {
            const auto raw=reply->readAll();
            const auto body=QJsonDocument::fromJson(raw.left(65536)).object();
            const auto items=body.value(anthropic?"content":"choices").toArray();
            QString answer;
            for(const auto& item:items)answer+=anthropic?item.toObject().value("text").toString():
                item.toObject().value("message").toObject().value("content").toString();
            const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool ok=reply->error()==QNetworkReply::NoError&&status>=200&&status<300&&raw.size()<=65536&&!answer.trimmed().isEmpty();
            reply->deleteLater();
            aiEditor->setEnabled(true);
            showProfile(slot);
            if(!ok){aiFeedback->setText(status?QString("验证未通过（HTTP %1），已保留原来的可用配置。").arg(status):"验证未通过，已保留原来的可用配置。");return;}
            profile.verifiedAt=QDateTime::currentMSecsSinceEpoch();
            if(!bridge_->saveAiProfile(slot,profile,true)){aiFeedback->setText("接口已响应，但无法保存验证结果。");return;}
            if(slot==0){bridge_->applyPrimaryAiProfile(profile);aiEnabled_->setChecked(true);}
            else if(slot==1)bridge_->applyGptAiProfile();
            showProfile(slot);
            aiFeedback->setText(slot==0?"验证通过，DeepSeek 账号已应用此配置。":
                slot==1&&bridge_->config().gptAccountId>0?"验证通过，已保存 GPT 单聊配置。":"验证通过，配置已保存。");
        });
    });
    auto* aiHint=new QLabel("私聊专用板端账号可直接提问，例如「现在没检测到人是什么原因？」；群聊用 /AI 问题。\n每次刷新感知、链路和设备状态，不上传图像或完整日志。模型上下文最多三轮，仅留内存，10 分钟无活动清空；聊天记录按 LanChat 保存。模型服务可能计费。",this);
    aiHint->setWordWrap(true);aiHint->setStyleSheet("color:#8A94A4;background:transparent;");layout->addWidget(aiHint);
    aiState_=new QLabel(this);aiState_->setObjectName("boardAiState");aiState_->setWordWrap(true);layout->addWidget(aiState_);
    auto* aiChat=new QPushButton("在聊天中询问板端状态",this);aiChat->setObjectName("boardAiChat");layout->addWidget(aiChat);
    connect(aiChat,&QPushButton::clicked,this,[this]{if(bridge_->config().accountId>0)emit aiChatRequested(bridge_->config().accountId);});
    auto* messageTitle = new QLabel("发送到板端屏幕", this);
    layout->addWidget(messageTitle);
    message_ = new QPlainTextEdit(this);
    message_->setObjectName("boardMessageText");
    message_->setPlaceholderText("输入消息。正文最多 384 字节，约 128 个汉字。板端可查看详情。");
    message_->setMaximumHeight(84);
    layout->addWidget(message_);
    auto* messageRow = new QHBoxLayout;
    send_ = new QPushButton("发送到板端", this);
    send_->setObjectName("boardSendMessage");
    delivery_ = new QLabel(this);
    delivery_->setObjectName("boardMessageDelivery");
    delivery_->setWordWrap(true);
    messageRow->addWidget(send_); messageRow->addWidget(delivery_, 1);
    layout->addLayout(messageRow);
    controls_=new QWidget(this);
    auto* controlsLayout=new QVBoxLayout(controls_);
    controlsLayout->setContentsMargins(0,0,0,0);
    auto* pageRow=new QHBoxLayout;
    auto* page=new QComboBox(controls_);page->setObjectName("boardScreenPage");
    const QStringList pageNames={"总览","收件箱","PPM 感知","VVOS 链路","设备状态"};
    const QStringList pageActions={"home","inbox","ppm","link","device"};
    for(int i=0;i<pageNames.size();++i)page->addItem(pageNames[i],pageActions[i]);
    auto* language=new QComboBox(controls_);language->setObjectName("boardScreenLanguage");
    language->addItem("中文","lang_zh");language->addItem("English","lang_en");
    pageRow->addWidget(new QLabel("板端页面",controls_));pageRow->addWidget(page,1);pageRow->addWidget(language);
    controlsLayout->addLayout(pageRow);
    connect(page,&QComboBox::activated,this,[this,page](int index){bridge_->navigate(page->itemData(index).toString());});
    connect(language,&QComboBox::activated,this,[this,language](int index){bridge_->navigate(language->itemData(index).toString());});
    auto* navigation=new QGridLayout;
    const QStringList labels={"最新消息详情","上一条","下一条","正文向上","正文向下","本地标为已读"};
    const QStringList navActions={"detail","previous","next","scroll_up","scroll_down","read"};
    for(int i=0;i<labels.size();++i){
        auto* button=new QPushButton(labels[i],controls_);button->setObjectName("boardNav_"+navActions[i]);
        navigation->addWidget(button,i/3,i%3);
        connect(button,&QPushButton::clicked,this,[this,action=navActions[i]]{bridge_->navigate(action);});
    }
    controlsLayout->addLayout(navigation);layout->addWidget(controls_);
    view_=new QLabel("等待板端页面状态",this);view_->setObjectName("boardScreenState");view_->setWordWrap(true);
    layout->addWidget(view_);
    layout->addWidget(new QLabel("在上方「板端页面」切换五个界面；板端暂不支持触摸翻页。",this));
    connect(send_, &QPushButton::clicked, this, [this] {
        const QString text = message_->toPlainText().trimmed();
        if (!session_ || session_->selfId() < 1 || text.isEmpty() || text.toUtf8().size() > 384) return;
        pendingText_ = text; pendingId_.clear(); pendingSince_ = QDateTime::currentMSecsSinceEpoch();
        session_->sendPrivate(bridge_->config().accountId, text);
        delivery_->setText("正在提交到 LanChat…"); send_->setEnabled(false);
    });
    connect(message_, &QPlainTextEdit::textChanged, this, &BoardConnectionPanel::refresh);
    auto* whaleTitle=new QLabel("板端角色 · 沉浸模式",this);
    whaleTitle->setStyleSheet("font-size:16px;font-weight:600;color:#E7EDF7;background:transparent;");layout->addWidget(whaleTitle);
    auto* whaleHint=new QLabel("鲸鱼娘使用深蓝主题，GPT 白龙娘使用银白主题。文字回答同步到板端字幕和扬声器。\n模型渠道在「AI 状态问答」设置。",this);whaleHint->setWordWrap(true);layout->addWidget(whaleHint);
    auto* voice=new QComboBox(this);voice->setObjectName("boardWhaleVoice");voice->addItem("自动选择（优先角色声线）","auto");voice->addItem("Windows 中文语音","windows");voice->addItem("角色声线（本地 VITS）","local_vits");
    voice->setCurrentIndex(qMax(0,voice->findData(bridge_->config().voiceProvider)));layout->addWidget(voice);
    voice->setToolTip("切换语音设置会重新连接板端桥。新版角色声线需安装在桥接 PC：鲸鱼娘为派蒙风格，GPT 白龙娘为优菈风格。");
    const auto updateVoice=[this,voice]{if(auto* model=qobject_cast<QStandardItemModel*>(voice->model()))model->item(2)->setEnabled(bridge_->localVoiceAvailable());};
    connect(bridge_,&BoardConnection::changed,voice,updateVoice);updateVoice();
    connect(voice,QOverload<int>::of(&QComboBox::activated),this,[this,voice]{if(!bridge_->setVoiceProvider(voice->currentData().toString()))voice->setCurrentIndex(qMax(0,voice->findData(bridge_->config().voiceProvider)));});
    auto* whaleButtons=new QGridLayout;
    int whaleIndex=0;
    for(const auto& item:QList<QPair<QString,QString>>{{"鲸鱼娘 · 深蓝","immersion"},{"GPT 白龙娘 · 银白","immersion_dragon"},{"返回五页终端","terminal"},{"扬声器短音测试","audio_test"},{"停止说话","stop_speech"}}){
        auto* button=new QPushButton(item.first,this);button->setObjectName("boardWhale_"+item.second);whaleButtons->addWidget(button,whaleIndex/2,whaleIndex%2);++whaleIndex;
        connect(button,&QPushButton::clicked,this,[this,action=item.second]{if(!bridge_->navigate(action))stats_->setText("板端未就绪或不支持沉浸模式，请检查连接和固件。");});
        const auto enableWhale=[this,button,action=item.second]{const auto d=bridge_->snapshot().value("display").toObject();button->setEnabled(d.value("terminal_ready").toBool()&&d.value("navigation").toBool()&&d.value("immersion_supported").toBool()&&(action!="immersion_dragon"||d.value("dragon_supported").toBool())&&(action!="audio_test"||d.value("pcm_supported").toBool()));};
        connect(bridge_,&BoardConnection::changed,button,enableWhale);enableWhale();
    }
    layout->addLayout(whaleButtons);
    auto* volume=new QComboBox(this);volume->setObjectName("boardWhaleVolume");for(int value:{10,20,30,40,50})volume->addItem(QString("音量 %1%").arg(value),value);volume->setCurrentIndex(1);layout->addWidget(volume);
    connect(volume,QOverload<int>::of(&QComboBox::activated),this,[this,volume]{bridge_->navigate("volume_"+volume->currentData().toString());});
    const auto updateVolume=[this,volume]{const auto d=bridge_->snapshot().value("display").toObject();volume->setEnabled(d.value("terminal_ready").toBool()&&d.value("pcm_supported").toBool());const auto index=volume->findData(d.value("view").toObject().value("volume").toInt(20));if(index>=0)volume->setCurrentIndex(index);};
    connect(bridge_,&BoardConnection::changed,volume,updateVolume);updateVolume();
    auto* speechStatus=new QLabel(this);speechStatus->setObjectName("boardWhaleSpeechStatus");speechStatus->setWordWrap(true);layout->addWidget(speechStatus);
    const auto updateSpeech=[this,speechStatus]{
        const auto s=bridge_->snapshot().value("speech").toObject();
        const auto view=bridge_->snapshot().value("display").toObject().value("view").toObject();
        const QString name=!view.value("immersion").toBool()?"五页终端":
            view.value("persona").toString()=="dragon"?"GPT 白龙娘":"鲸鱼娘";
        speechStatus->setText(QString("板端界面：%1 · 语音状态：%2 · 已完成 %3 段\n声线：%4\n%5")
            .arg(name,s.value("stage").toString("未连接")).arg(s.value("completed_segments").toInt())
            .arg(s.value("voice").toString("待连接")).arg(s.value("last_error").toString()));
    };
    connect(bridge_,&BoardConnection::changed,speechStatus,updateSpeech);updateSpeech();
    if (session_) connect(session_, &Session::messageReceived, this, [this](const QJsonObject& message) {
        if (pendingText_.isEmpty() || message.value("fromId").toInt() != session_->selfId()
            || message.value("toId").toInt() != bridge_->config().accountId || message.value("msg").toString() != pendingText_) return;
        pendingId_ = message.value("msg_id").toVariant().toString(); refresh();
    });
    connect(browse, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getOpenFileName(this, "选择 Console 桥", consolePath_->text(), "程序 (*.exe)");
        if (!path.isEmpty()) consolePath_->setText(path);
    });
    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(24,0,24,0);
    auto* start = new QPushButton("保存并连接", this);
    start->setObjectName("primary");
    auto* stop = new QPushButton("断开连接", this);
    auto* close = new QPushButton("返回 LanChat", this);
    close->setObjectName("boardReturnToLanChat");
    actions->addWidget(start);
    actions->addWidget(stop);
    actions->addStretch();
    actions->addWidget(close);
    outer->addLayout(actions);
    connect(start, &QPushButton::clicked, this, [this,gptId,gptPassword,gptState] {
        auto config = bridge_->config();
        config.listenPort = static_cast<quint16>(port_->value());
        config.target = target_->value();
        config.groupId = group_->value();
        config.autoStart = automatic_->isChecked();
        config.consolePath = consolePath_->text();
        config.aiEnabled=aiEnabled_->isChecked();
        const int gptIdValue=gptId->value();
        if (gptIdValue>0 && gptIdValue==config.accountId) {
            gptState->setText("GPT 必须使用另一个 LanChat 账号 ID。");
            gptId->setFocus();
            return;
        }
        if (gptIdValue!=config.gptAccountId) config.gptPassword.clear();
        config.gptAccountId=gptIdValue;
        if (!gptIdValue) config.gptPassword.clear();
        else if (!gptPassword->text().isEmpty()) config.gptPassword=gptPassword->text();
        if (gptIdValue>0 && config.gptPassword.isEmpty()) {
            gptState->setText("请填写 GPT 的 LanChat 账号密码。");
            gptPassword->setFocus();
            return;
        }
        bridge_->configure(config);
        bridge_->start();
        gptPassword->clear();
        gptPassword->setPlaceholderText(config.gptPassword.isEmpty()?"填写 LanChat 账号密码（与 API Key 分开）":"已加密保存，留空保留");
    });
    connect(stop, &QPushButton::clicked, bridge_, &BoardConnection::stop);
    connect(close, &QPushButton::clicked, this, &BoardConnectionPanel::returnRequested);
    connect(bridge_, &BoardConnection::changed, this, &BoardConnectionPanel::refresh);
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &BoardConnectionPanel::refresh);
    timer->start(500);
    refresh();
}

void BoardConnectionPanel::refresh()
{
    status_->setText(bridge_->state());
    const auto data = bridge_->snapshot();
    const auto ai=data.value("ai").toObject();
    aiState_->setText(ai.isEmpty()?"等待 AI 桥状态":!ai.value("enabled").toBool()?"AI 未启用":!ai.value("configured").toBool()?"AI 配置不完整":QString("AI 已就绪 · %1 · %2 个问题处理中 · 已回答 %3 次 / 失败 %4 次")
        .arg(ai.value("model").toString()).arg(ai.value("pending").toInt()).arg(ai.value("replied").toInteger()).arg(ai.value("failed").toInteger()));
    findChild<QPushButton*>("boardAiChat")->setEnabled(bridge_->config().accountId>0);
    QStringList addresses;
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()
            && !address.toString().startsWith("169.254.")) addresses << address.toString();
    }
    const qint64 last = data.value("last_frame_ms").toInteger();
    stats_->setText(QString("本机地址：%1\n板端地址：%2    已收 %3 帧    解码失败 %4\n最近上报：%5    LanChat：%6")
        .arg(addresses.join(" / "), data.value("board_peer").toString("未连接"))
        .arg(data.value("frames").toInteger()).arg(data.value("decode_failures").toInteger())
        .arg(last > 0 ? QDateTime::fromMSecsSinceEpoch(last).toString("HH:mm:ss") : "尚无有效帧",
             data.value("lanchat_state").toString("等待桥接状态")));
    account_->setText(bridge_->config().accountId > 0
        ? QString("%1（密码由 Windows 加密保存）").arg(bridge_->config().accountId)
        : "首次连接时自动创建，与聊天账号独立");
    const auto display = data.value("display").toObject();
    const bool ready = display.value("terminal_ready").toBool();
    controls_->setEnabled(ready&&display.value("navigation").toBool());
    const auto view=display.value("view").toObject();
    const QStringList pages={"总览","收件箱","PPM 感知","VVOS 链路","设备状态"};
    const int pageIndex=view.value("page").toInt(-1);
    view_->setText(pageIndex>=0&&pageIndex<pages.size()?QString("当前显示：%1%2 · %3 条消息 / %4 未读 · %5")
        .arg(pages[pageIndex],view.value("detail").toBool()?" / 详情":"").arg(view.value("count").toInt()).arg(view.value("unread").toInt()).arg(view.value("language").toString()):"等待板端页面状态");
    if(pageIndex>=0&&pageIndex<pages.size())controls_->findChild<QComboBox*>("boardScreenPage")->setCurrentIndex(pageIndex);
    controls_->findChild<QComboBox*>("boardScreenLanguage")->setCurrentIndex(view.value("language").toString()=="en"?1:0);
    const QString text = message_->toPlainText().trimmed();
    if (!pendingText_.isEmpty()) {
        for (const auto& item : display.value("deliveries").toArray()) {
            const auto result = item.toObject(); if (result.value("id").toString() != pendingId_) continue;
            const QString state = result.value("state").toString();
            if (state == "rendered") { delivery_->setText("板端已显示 · 消息 " + pendingId_); pendingText_.clear(); }
            else if (state == "accepted") delivery_->setText("板端已收到，等待屏幕显示…");
            else if (state == "rejected" || state == "expired") { delivery_->setText(result.value("reason").toString()); pendingText_.clear(); }
            else delivery_->setText("LanChat 已接收，等待板端…");
            break;
        }
        if (!pendingText_.isEmpty() && QDateTime::currentMSecsSinceEpoch() - pendingSince_ > 35000) {
            delivery_->setText("显示回执超时，连接恢复后可重发"); pendingText_.clear();
        }
    } else if (pendingSince_ == 0) {
        delivery_->setText(!session_ || session_->selfId() < 1 ? "先登录 LanChat" : !ready ? "等待板端终端固件连接" : "板端可接收消息");
    }
    send_->setEnabled(session_ && session_->selfId() > 0 && ready && pendingText_.isEmpty() && !text.isEmpty() && text.toUtf8().size() <= 384);
}
