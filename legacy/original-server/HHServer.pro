#-------------------------------------------------
#
# Project created by QtCreator 2021-01-28T08:47:45
#
#-------------------------------------------------

QT       += core gui
QT       += sql
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HHServer
TEMPLATE = app


SOURCES += main.cpp\
    ../legacy-common/LegacyLogger.cpp \
    MsgBuilder.cpp \
    MainWindow.cpp \
    MessageDao.cpp \
    NetworkThread.cpp \
    TcpServerForThread.cpp \
    UserDao.cpp \
    UserEntity.cpp

HEADERS  += \
    ../legacy-common/LegacyLogger.h \
    ../legacy-common/LegacyProtocolCodec.h \
    MsgBuilder.h \
    MainWindow.h \
    MessageDao.h \
    NetworkThread.h \
    PasswordHash.h \
    Publicdef.h \
    TcpServerForThread.h \
    UserDao.h \
    UserEntity.h

FORMS    += mainwindow.ui

CONFIG += c++17
INCLUDEPATH += ../../protocol ../legacy-common

