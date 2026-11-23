#include "app/preferences.h"
#include "ui/theme.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QRegularExpression>
#include <QJsonDocument>

namespace preferences {
bool shouldNotify(bool group,bool board,bool active){QSettings s;return !active&&!s.value("preferences/do_not_disturb",false).toBool()&&s.value(board?"preferences/notify_board":group?"preferences/notify_group":"preferences/notify_direct",true).toBool();}
QString scaledStyleSheet(int percent){auto sheet=theme::stylesheet();const auto scale=qBound(85,percent,150)/100.0;QRegularExpression size("font-size:\\s*([0-9.]+)px");auto matches=size.globalMatch(sheet);QList<QRegularExpressionMatch> list;while(matches.hasNext())list.append(matches.next());for(auto i=list.crbegin();i!=list.crend();++i)sheet.replace(i->capturedStart(1),i->capturedLength(1),QString::number(i->captured(1).toDouble()*scale,'f',1));return sheet;}
void applyDisplay(){qApp->setStyleSheet(scaledStyleSheet(QSettings().value("preferences/font_scale",100).toInt()));}
bool startupEnabled(){
#ifdef Q_OS_WIN
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);return run.value("LanChat-Next").toString()==('"'+QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+'"');
#else
    return false;
#endif
}
bool setStartupEnabled(bool enabled){
#ifdef Q_OS_WIN
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);if(enabled)run.setValue("LanChat-Next",'"'+QDir::toNativeSeparators(QCoreApplication::applicationFilePath())+'"');else run.remove("LanChat-Next");run.sync();return run.status()==QSettings::NoError;
#else
    return !enabled;
#endif
}
QString diagnostics(const QJsonObject& b,const QString& connection,const QString& hosted,int uid){
    // 逐个挑选运行状态字段；不序列化配置、聊天内容、账号密码、API key 或原始载荷。
    const auto ai=b.value("ai").toObject(),logs=b.value("logs").toObject(),display=b.value("display").toObject();
    QJsonObject info{{"client","LanChat-Next 2026.11.23.2"},{"console_protocol","VehicleStack bridge v1"},{"time",QDateTime::currentDateTime().toString(Qt::ISODate)},{"logged_in",uid>0},{"chat_connection",connection},{"hosting",hosted},{"board_connected",b.value("board_connected")},{"listening",b.value("listening")},{"terminal_ready",display.value("terminal_ready")},{"frames",b.value("frames")},{"decode_failures",b.value("decode_failures")},{"ai_configured",ai.value("configured")},{"ai_provider",ai.value("provider")},{"ai_model",ai.value("model")},{"ai_last_http",ai.value("last_http")},{"ai_error_kind",ai.value("error_kind")},{"ai_daily_calls",ai.value("daily_calls")},{"ai_cooldown_seconds",ai.value("cooldown_seconds")},{"usb_state",logs.value("state")},{"usb_write_errors",logs.value("write_errors")}};
    info.insert("console_version",b.value("console_version"));return QString::fromUtf8(QJsonDocument(info).toJson(QJsonDocument::Indented));
}
}
