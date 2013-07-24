include(../../qtcreatorplugin.pri)
QT += network \
             sql
#LIBS += --lmysqlclient_r
HEADERS += \
    mythtvwindow.h \
    mythtvplugin.h \
    mainwindow.h

SOURCES += \
    mythtvwindow.cpp \
    mythtvplugin.cpp \
    mainwindow.cpp

OTHER_FILES += \
icon.png

RESOURCES +=

FORMS += \
    mainwindow.ui
