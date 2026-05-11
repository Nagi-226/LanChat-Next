#-------------------------------------------------
#
# Project created by QtCreator 2021-01-27T14:56:08
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HHClient
TEMPLATE = app


SOURCES += main.cpp\
    MsgBuilder.cpp \
    ChatDialog.cpp \
    ChatItem.cpp \
    ChatWidget.cpp \
    CreateGroupDialog.cpp \
    FriendItem.cpp \
    GroupChatDialog.cpp \
    HHTcpSocket.cpp \
    LoginDialog.cpp \
    MainWindow.cpp \
    RegisterDialog.cpp \
    SearchGroupDialog.cpp \
    MsgMarke.cpp

HEADERS  += \
    ../legacy-common/LegacyProtocolCodec.h \
    MsgBuilder.h \
    ChatDialog.h \
    ChatItem.h \
    ChatWidget.h \
    CreateGroupDialog.h \
    FriendItem.h \
    GroupChatDialog.h \
    HHTcpSocket.h \
    LoginDialog.h \
    MainWindow.h \
    Publicdef.h \
    RegisterDialog.h \
    SearchGroupDialog.h \
    MsgMarke.h

FORMS    += mainwindow.ui \
    logindialog.ui \
    registerdialog.ui \
    frienditem.ui \
    chatdialog.ui \
    creategroupdialog.ui \
    groupchatdialog.ui \
    searchgroupdialog.ui \
    MsgMarke.ui

CONFIG += c++17
INCLUDEPATH += ../../protocol ../legacy-common

RESOURCES += \
    heads.qrc

