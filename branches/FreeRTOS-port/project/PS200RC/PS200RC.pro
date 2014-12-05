#-------------------------------------------------
#
# Project created by QtCreator 2014-09-24T17:42:53
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PS200RC
TEMPLATE = app
CONFIG += c++11

INCLUDEPATH += ../../source/src/

SOURCES += main.cpp\
        mainwindow.cpp \
    singlevaluedialog.cpp \
    mydoublespinbox.cpp \
    settingsdialog.cpp \
    settingshelper.cpp \
    serialworker.cpp \
    serialtop.cpp \
    logviewer.cpp \
    txEedit.cpp \
    keydriver.cpp \
    keywindow.cpp \
	serialparser.cpp \
	../../source/src/uart_proto.c

HEADERS  += mainwindow.h \
    singlevaluedialog.h \
    mydoublespinbox.h \
    settingsdialog.h \
    settingshelper.h \
    serialworker.h \
    serialtop.h \
    logviewer.h \
    txEdit.h \
    keydriver.h \
    keywindow.h \
    serialparser.h \
	globaldef.h \
	../../source/src/uart_proto.h

FORMS    += mainwindow.ui \
    singlevaluedialog.ui \
    settingsdialog.ui \
	keywindow.ui

RESOURCES += \
    resources.qrc
