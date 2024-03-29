include(../../qtcreatorplugin.pri)
QT += network sql webkitwidgets declarative

unix:!macx:!android: LIBS += -ldns_sd
!contains(CONFIG,NO_AVAHI): unix:!macx:LIBS +=  -lavahi-client -lavahi-common
win32:LIBS += -L"c:\\PROGRA~1\\BONJOU~1\\lib\\win32" -ldnssd
win32:INCLUDEPATH += "c:\\program files\\bonjour sdk\\include"
android:LIBS += -jmdns
#LIBS += -lmysqlclient_r
LIBS += -lmysqlclient_r
HEADERS += \
    mythtvwindow.h \
    mythtvplugin.h \
    mainwindow.h \
    sshdialog.h \
    zconf.h \
    bonjourrecord.h \
    bonjourservicebrowser.h \
    bonjourserviceresolver.h \
    moresdkdialog.h

SOURCES += \
    mythtvwindow.cpp \
    mythtvplugin.cpp \
    mainwindow.cpp \
    sshdialog.cpp \
    zconf.cpp \
    bonjourservicebrowser.cpp \
    bonjourserviceresolver.cpp \
    moresdkdialog.cpp

OTHER_FILES += \
    icon.png \
    Main.qml \
    mythtv.qbs

RESOURCES += \
    mythtvResources.qrc

FORMS += \
    mainwindow.ui \
    sshdialog.ui \
    moresdkdialog.ui
