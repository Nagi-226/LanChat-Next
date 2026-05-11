#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "HHTcpSocket.h"
#include "MsgBuilder.h"
#include <vector>
#include <QListWidgetItem>
#include "FriendItem.h"
#include "ChatDialog.h"
#include <map>
#include "CreateGroupDialog.h"
#include "SearchGroupDialog.h"
#include "GroupChatDialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(std::vector<MsgBuilder::UserData> friends, QWidget *parent = 0);
    ~MainWindow();
    struct ItemAndDialog
    {
        FriendItem* item;
        ChatDialog* dialog;
    };
    void addFriendsList(MsgBuilder::UserData user);//娣诲姞濂藉弸淇℃伅
    
private slots:
    void readyRead();
    void on_listWidget_friends_itemDoubleClicked(QListWidgetItem *item);
    void rejected();
    void createGroupDialogRejected();
    void searchGroupDialogRejected();
    void groupChatDialogRejected();
    void createGroupChatDialog(int id, QString name);
    void on_pushButton_createGroup_clicked();
    
    void on_pushButton_searchGroup_clicked();
    
private:
    void initSocket();
    void initFriendsList(std::vector<MsgBuilder::UserData> friends);//鍒濆鍖栧ソ鍙嬩俊鎭?  
    void receiveMsg(QString jsonStr);
    void removeFromFriendList(MsgBuilder::UserData user);//鍒犻櫎濂藉弸淇℃伅
    void createGroupReturn(QString jsonStr);
    void searchGroupReturn(QString jsonStr);
    void joinGroupReturn(QString jsonStr);
    void userJoinGroup(QString jsonStr);
    void userLeaveGroup(QString jsonStr);
    void receiveGroupMsg(QString jsonStr);
private:
    Ui::MainWindow *ui;
    HHTcpSocket* socket;
    std::map<MsgBuilder::UserData, ItemAndDialog> chatFriends;//姝ｅ湪鑱婂ぉ鐨勫ソ鍙?
    CreateGroupDialog* createGroupDialog = 0;
    SearchGroupDialog* searchGroupDialog = 0;
    std::map<int, GroupChatDialog*> groupChats;//缇よ亰绐楀彛
};

#endif // MAINWINDOW_H

