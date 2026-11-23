#pragma once
#include <QString>
#include <QJsonObject>
namespace preferences {
bool shouldNotify(bool group,bool board,bool foregroundConversation);
QString scaledStyleSheet(int percent);
void applyDisplay();
bool startupEnabled();
bool setStartupEnabled(bool enabled);
QString diagnostics(const QJsonObject& bridge,const QString& connection,const QString& hostState,int uid);
}
