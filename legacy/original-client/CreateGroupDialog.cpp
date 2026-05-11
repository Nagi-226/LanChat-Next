#include "CreateGroupDialog.h"
#include "ui_creategroupdialog.h"
#include "HHTcpSocket.h"
#include "MsgBuilder.h"

CreateGroupDialog::CreateGroupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateGroupDialog)
{
    ui->setupUi(this);
}

CreateGroupDialog::~CreateGroupDialog()
{
    delete ui;
}

void CreateGroupDialog::showGroupId(int groupId)
{
    ui->label_groupId->setText(QString("缇ゅ彿 %1 蹇幓鏌ユ壘缇ゅ惂!").arg(groupId));
}

void CreateGroupDialog::on_pushButton_clicked()
{
    HHTcpSocket* socket = HHTcpSocket::getInstance();
    QString data = MsgBuilder::buildCreateGroup(ui->lineEdit->text());
    socket->sendJson(data);
}

