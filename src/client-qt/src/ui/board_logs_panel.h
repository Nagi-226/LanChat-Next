#pragma once
#include <QWidget>
#include <QJsonArray>
class BoardConnection;
class Session;
class QLabel;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QPlainTextEdit;
class QTextEdit;
class QPushButton;

class BoardLogsPanel : public QWidget {
public:
    BoardLogsPanel(BoardConnection* bridge,Session* session,QWidget* parent=nullptr);
private:
    bool eventFilter(QObject* watched,QEvent* event) override;
    void setFollowing(bool following);
    void scrollToLatest();
    void refresh();
    void render();
    QString filteredText() const;
    QJsonArray filteredRows() const;
    void updateCounts();
    void query(const QString& command);
    BoardConnection* bridge_;Session* session_;
    QLabel* source_;QLabel* counts_;QLabel* feedback_;QLabel* lastTime_;
    QComboBox* port_;QComboBox* level_;QComboBox* module_;
    QLineEdit* search_;QLineEdit* command_;
    QCheckBox* raw_;QCheckBox* follow_;QTextEdit* logs_;QPlainTextEdit* replies_;
    QPushButton* pause_;
    QJsonArray rows_;bool paused_=false,following_=true;qint64 cleared_=0,displayed_=0;
    bool rendering_=false,scrollScheduled_=false;
};
