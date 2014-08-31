#-------------------------------------------------
#
# Project created by QtCreator 2013-11-15T11:14:14
#
#-------------------------------------------------

QT       += core gui

TARGET = QMenuSim
TEMPLATE = app

DEFINES += _GUITESTPROJ_

INCLUDEPATH = ./
INCLUDEPATH += ./gui_top/
INCLUDEPATH += ../common/
INCLUDEPATH += ../../source/src/
INCLUDEPATH += ../../source/gui/
INCLUDEPATH += ../../source/gui/guiWidgets/
INCLUDEPATH += ../../source/gui/guiCore/
INCLUDEPATH += ../../source/gui/utils/
INCLUDEPATH += ../../source/gui/guiGraphics/

SOURCES += main.cpp\
		mainwindow.cpp \
    ../common/pixeldisplay.cpp \
	../common/keydriver.cpp \
	../../source/gui/guiGraphics/font_6x8_mono.c \
    ../../source/gui/guiGraphics/font_h32.c \
    ../../source/gui/guiGraphics/font_h10.c \
    ../../source/gui/guiGraphics/font_h10_bold.c \
	../../source/gui/guiGraphics/font_h16.c \
	../../source/gui/guiGraphics/widget_images.c \
	../../source/gui/guiGraphics/other_images.c \
    ../../source/gui/guiGraphics/guiGraphHAL.c \
    ../../source/gui/guiGraphics/guiGraphPrimitives.c \
    ../../source/gui/guiGraphics/guiGraphWidgets.c \
    ../../source/gui/guiCore/guiCore.c \
	../../source/gui/guiWidgets/guiPanel.c \
    ../../source/gui/guiWidgets/guiCheckBox.c \
	../../source/gui/guiWidgets/guiTextLabel.c \
	../../source/gui/guiGraphics/font_h11.c \
    ../../source/gui/guiWidgets/guiSpinBox.c \
	../../source/gui/guiWidgets/guiStringList.c \
    ../../source/gui/utils/utils.c \
    ../../source/gui/guiMainForm.c \
    ../../source/gui/guiSetupPanel.c \
    ../../source/gui/guiMasterPanel.c \
    ../../source/gui/guiEditPanel1.c \
	../../source/gui/guiMessagePanel1.c \
    gui_top/guiTop.c \
    ../../source/gui/guiEditPanel2.c \
    ../../source/gui/guiWidgets/guiTextSpinBox.c \
    ../../source/gui/guiWidgets/guiRadioButton.c \
    ../../source/gui/guiWidgets/guiSelectTextBox.c
    

HEADERS  += mainwindow.h \
    ../common/pixeldisplay.h \
	../common/keydriver.h \
	../../source/gui/guiGraphics/guiFonts.h \
	../../source/gui/guiGraphics/guiImages.h \
	../../source/gui/guiGraphics/guiGraphHAL.h \
    ../../source/gui/guiGraphics/guiGraphPrimitives.h \
	../../source/gui/guiGraphics/guiGraphWidgets.h \
	../../source/gui/guiCore/guiEvents.h \
    ../../source/gui/guiCore/guiCore.h \
	../../source/gui/guiWidgets/guiPanel.h \
	../../source/gui/guiWidgets/guiTextLabel.h \
	../../source/gui/guiWidgets/guiCheckBox.h \
	../../source/gui/guiWidgets/guiStringList.h \
	../../source/gui/guiWidgets/guiSpinBox.h \
	../../source/gui/utils/utils.h \    
    ../../source/gui/guiMainForm.h \
    ../../source/gui/guiSetupPanel.h \
    ../../source/gui/guiMasterPanel.h \
    ../../source/gui/guiEditPanel1.h \
	../../source/gui/guiMessagePanel1.h \
    ../../source/src/key_def.h \
    ../../source/src/global_def.h \
	../../source/gui/guiConfig.h \
	gui_top/guiTop.h \
	gui_top/taps.h \
    ../../source/gui/guiEditPanel2.h \
    ../../source/gui/guiWidgets/guiTextSpinBox.h \
    ../../source/gui/guiWidgets/guiRadioButton.h \
    ../../source/gui/guiWidgets/guiSelectTextBox.h \
    ../../source/gui/guiCore/guiWidgets.h \
    ../../source/gui/guiConfig.h
    
    
    
	


FORMS    += mainwindow.ui

