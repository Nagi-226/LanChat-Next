#pragma once
#include <QWidget>
class BoardConnection;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class Session;
class QPlainTextEdit;
class QPushButton;
class QComboBox;

class BoardConnectionPanel : public QWidget {
    Q_OBJECT
public:
    explicit BoardConnectionPanel(BoardConnection* bridge, QWidget* parent = nullptr, Session* session = nullptr);
signals:
    void returnRequested();
    void aiChatRequested(int accountId);
private:
    void refresh();
    BoardConnection* bridge_;
    QLabel* status_;
    QLabel* stats_;
    QLabel* account_;
    QSpinBox* port_;
    QSpinBox* target_;
    QSpinBox* group_;
    QCheckBox* automatic_;
    QLineEdit* consolePath_;
    QCheckBox* aiEnabled_;
    QComboBox* aiProvider_;
    QLineEdit *aiEndpoint_, *aiModel_, *aiKey_;
    QLabel* aiState_;
    Session* session_;
    QPlainTextEdit* message_;
    QPushButton* send_;
    QLabel* delivery_;
    QWidget* controls_;
    QLabel* view_;
    QString pendingText_,pendingId_;
    qint64 pendingSince_ = 0;
};
