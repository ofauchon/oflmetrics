#-------------------------------------------------
#
# Project created by QtCreator 2012-02-25T08:06:13
#
#-------------------------------------------------

QT       += core network sql
QT       -= gui

COBJ = utils.o


LIBS += -L./xPLLib -lxPLLib
INCLUDEPATH += ./xPLLib

TARGET = oflbridge
CONFIG   += console debug
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += serialib.cpp main.cpp \
    tcpserver.cpp \
    serverthread.cpp \
    serialdevicethread.cpp \
    packetprocessor.cpp \
    packet.cpp \
    config.cpp \
    xplmanager.cpp

OTHER_FILES +=

HEADERS += \
    tcpserver.h \
    serverthread.h \
    serialdevicethread.h \
    packetprocessor.h \
    packet.h \
    config.h \
    xplmanager.h
