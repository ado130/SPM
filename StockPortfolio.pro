#-------------------------------------------------
#
# Project created by QtCreator 2019-06-16T12:18:46
#
#-------------------------------------------------

QT += core gui network charts printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SPM
TEMPLATE = app

VERSION = 1.0.8
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_BUILD = 8


DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR"\
           "VERSION_MINOR=$$VERSION_MINOR"\
           "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}
DEFINES += VERSION_STR=\\\"$${VERSION}\\\"

RC_ICONS = icons/favicon.ico

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += QT_NO_FOREACH

CONFIG += c++17
/std:c++17

# CMAKE
# -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic

SOURCES += \
        database.cpp \
        degiro.cpp \
        downloadmanager.cpp \
        filterform.cpp \
        main.cpp \
        mainwindow.cpp \
        screener.cpp \
        screenerform.cpp \
        screenertab.cpp \
        settingsform.cpp \
        stockdata.cpp \
        tastyworks.cpp

HEADERS += \
        database.h \
        degiro.h \
        downloadmanager.h \
        filterform.h \
        global.h \
        mainwindow.h \
        screener.h \
        screenerform.h \
        screenertab.h \
        settingsform.h \
        stockdata.h \
        tastyworks.h

FORMS += \
        filterform.ui \
        mainwindow.ui \
        screenerform.ui \
        screenertab.ui \
        settingsform.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
