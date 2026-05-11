#ifndef MSGBUILDER_H
#define MSGBUILDER_H

#include "../../protocol/message_types.h"
#include <QString>
#include <vector>

// Type compatibility: MsgBuilder enum values (0-19) match lanchat::protocol::MsgType ordinal.
// Extended types (20-33) defined here mirror protocol/message_types.h.
// When migrating to Tauri client, replace this entire file with TypeScript types from protocol/.

class MsgBuilder
{
public:
    enum MsgType
    {
        // Legacy types (0-19) 鈥?compatible with lanchat::protocol::MsgType
        registerUser,       // 0
        registerUserReturn, // 1
        login,              // 2
        loginSucReturn,     // 3
        loginLoseReturn,    // 4
        sendMsg,            // 5
        receiveMsg,         // 6
        userOnline,         // 7
        userOffline,        // 8
        /********* group chat ***********/
        createGroup,        // 9
        createGroupReturn,  // 10
        searchGroup,        // 11
        searchGroupReturn,  // 12
        joinGroup,          // 13
        joinGroupReturn,    // 14
        leaveGroup,         // 15
        sendGroupMsg,       // 16
        receiveGroupMsg,    // 17
        userJoinGroup,      // 18
        userLeaveGroup,     // 19

        // New types (20-33) for LanChat-Next
        heartbeat,          // 20
        heartbeatAck,       // 21
        offlineMessages,    // 22
        logout,             // 23
        requestHistory,     // 24
        historyResponse,    // 25
        sendFile,           // 26
        receiveFile,        // 27
        fileTransferDone,   // 28
        aiRequest,          // 29
        aiResponse,         // 30
        aiStreamChunk,      // 31
        userProfileUpdate,  // 32
        systemBroadcast     // 33
    };
    struct UserData
    {
        int id = 0;
        QString password;
        int headId = 0;
        QString nickname;
        bool operator<(const UserData& other)const
        {
            return id < other.id;
        }
    };
    
    static int msgType(QString jsonStr);
    static QString buildErrorMsg(int code, QString message);
    //娉ㄥ唽鐢ㄦ埛    
    static QString buildRegisterUserMsg(QString password, int headId, QString nickname);
    static UserData parseRegisterUserMsg(QString jsonStr);
    //娉ㄥ唽鐢ㄦ埛杩斿洖
    static QString buildRegisterUserReturnMsg(int id);
    static int parseRegisterUserReturnMsg(QString jsonStr);
    //鐧诲綍
    static QString buildLoginMsg(int id, QString password);
    static UserData parseLoginMsg(QString jsonStr);
    //鐧诲綍杩斿洖
    static QString buildLoginSucReturnMsg(MsgBuilder::UserData hostData, std::vector<UserData>& friends);
    static std::vector<UserData> parseLoginSucReturnMsg(MsgBuilder::UserData& hostData, QString jsonStr);
    static QString buildLoginLoseReturnMsg();
    //鍙戦€佽亰澶╂暟鎹?
    static QString buildSendMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg);
    static QString parseSendMsg(QString jsonStr, MsgBuilder::UserData& from, MsgBuilder::UserData& to);
    //鏀跺埌鑱婂ぉ淇℃伅
    static QString buildReceiveMsg(MsgBuilder::UserData from, MsgBuilder::UserData to, QString msg);
    static QString parseReceiveMsg(QString jsonStr, MsgBuilder::UserData& from, MsgBuilder::UserData& to);
    //鐢ㄦ埛涓婄嚎
    static QString buildUserOnline(MsgBuilder::UserData user);
    static UserData parseUserOnline(QString jsonStr);
    //鐢ㄦ埛涓嬬嚎
    static QString buildUserOffline(MsgBuilder::UserData user);
    static UserData parseUserOffline(QString jsonStr);
    //鍒涘缓缇よ亰
    static QString buildCreateGroup(QString name);
    static QString parseCreateGroup(QString jsonStr);
    //鍒涘缓缇よ亰杩斿洖
    static QString buildCreateGroupReturn(int groupId);
    static int parseCreateGroupReturn(QString jsonStr);
    //鏌ユ壘缇よ亰
    static QString buildSearchGroup();
    //static int parseSearchGroup(QString jsonStr);
    //鏌ユ壘缇よ亰杩斿洖
    static QString buildSearchGroupReturn(std::vector<int>& ids, std::vector<QString>& names);
    static void parseSearchGroupReturn(QString jsonStr, std::vector<int>& ids, std::vector<QString>& names);
    //鍔犲叆缇よ亰
    static QString buildJoinGroup(UserData user, int groupId);
    static UserData parseJoinGroup(QString jsonStr, int& groupId);
    //鍔犲叆缇よ亰杩斿洖
    static QString buildJoinGroupReturn(int groupId, std::vector<UserData>& users);
    static int parseJoinGroupReturn(QString jsonStr, std::vector<UserData>& users);
    //绂诲紑缇よ亰
    static QString buildLeaveGroup(UserData user, int groupId);
    static int parseLeaveGroup(QString jsonStr, UserData& user);
    //鍙戦€佺兢鑱婁俊鎭?
    static QString buildSendGroupMsg(UserData from, int to, QString msg);
    static QString parseSendGroupMsg(QString jsonStr, UserData& from, int& to);
    //鎺ユ敹缇よ亰淇℃伅
    static QString buildReceiveGroupMsg(int from, UserData to, QString msg);
    static QString parseReceiveGroupMsg(QString jsonStr, int& from, UserData& to);
    //鐢ㄦ埛鍔犲叆缇よ亰
    static QString buildUserJoinGroup(UserData user, int groupId);
    static void parseUserJoinGroup(QString jsonStr, UserData& user, int& groupId);
    //鐢ㄦ埛绂诲紑缇よ亰
    static QString buildUserLeaveGroup(UserData user, int groupId);
    static void parseUserLeaveGroup(QString jsonStr, UserData& user, int& groupId);
};

#endif // MSGBUILDER_H



