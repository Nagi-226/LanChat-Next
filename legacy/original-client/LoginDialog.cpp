#include "LoginDialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include "MsgBuilder.h"
#include <QMessageBox>
#include "MainWindow.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    //setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    socket = HHTcpSocket::getInstance();
    connect(socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_pushButton_connect_clicked()
{
    socket->connectToServer(QHostAddress(ui->lineEdit_serverIp->text()), PORT);
}

void LoginDialog::on_pushButton_login_clicked()
{
    QString msg = MsgBuilder::buildLoginMsg(ui->lineEdit_id->text().toInt(), ui->lineEdit_password->text());
    socket->sendJson(msg);
}

void LoginDialog::on_pushButton_register_clicked()
{
    registerDialog = new RegisterDialog(this);
    registerDialog->show();
}

void LoginDialog::connected()
{
    ui->lineEdit_id->setEnabled(true);
    ui->lineEdit_password->setEnabled(true);
    ui->pushButton_login->setEnabled(true);
    ui->pushButton_register->setEnabled(true);
    ui->pushButton_connect->setEnabled(false);
}

void LoginDialog::readyRead()
{
    const QStringList frames = socket->readJsonFrames();
    for (QStringList::const_iterator it = frames.begin(); it != frames.end(); ++it) {
        const QString data = *it;
        switch(MsgBuilder::msgType(data))
        {
        case MsgBuilder::loginSucReturn:
            toMainwindow(data);
            break;
        case MsgBuilder::loginLoseReturn:
            QMessageBox::warning(this, "登录失败", "账号或密码错误");
            break;
        case MsgBuilder::registerUserReturn:
            if (registerDialog) {
                registerDialog->socketData(data);
            }
            break;
        default:
            break;
        }
    }
}

void LoginDialog::toMainwindow(QString data)
{
    loginSuc = true;
    MsgBuilder::UserData hostData;
    std::vector<MsgBuilder::UserData> friends = MsgBuilder::parseLoginSucReturnMsg(hostData, data);
    socket->setHostData(hostData);
    close();
    MainWindow* window = new MainWindow(friends);
    window->show();
}

void LoginDialog::closeEvent(QCloseEvent *e)
{
    disconnect(socket, 0, this, 0);
    if(!loginSuc)
        HHTcpSocket::releaseInstance();
}


