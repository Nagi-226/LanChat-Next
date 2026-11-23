#pragma once
#include <QWidget>
#include <QNetworkAccessManager>
#include <QTcpSocket>
class Session;
class BoardConnection;
class TransferManager;
class QStackedWidget;
class QFormLayout;
class QLabel;
class SettingsPanel : public QWidget {
    Q_OBJECT
public:
    SettingsPanel(Session* session,BoardConnection* bridge,TransferManager* transfers,QWidget* parent=nullptr);
    void setSection(int section);
    void setHostState(const QString& state){hostState_=state;refresh();}
signals:
    void preferencesChanged();
    void logoutRequested();
    void reconnectRequested();
    void boardRequested();
    void testNotificationRequested();
private:
    QFormLayout* page(const QString& name,const QString& title,const QString& description);
    void save(int section);
    void refresh();
    void testAi();
    void testConnection();
    QString diagnosticText() const;
    Session* session_;
    BoardConnection* bridge_;
    TransferManager* transfers_;
    QStackedWidget* pages_;
    QNetworkAccessManager network_;
    QTcpSocket probe_;
    QString hostState_="消息主机状态待确认";
    QLabel* feedback_;
};
