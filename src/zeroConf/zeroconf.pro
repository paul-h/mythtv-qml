TEMPLATE = lib
TARGET = zeroconf
QT += qml quick network
CONFIG += qt plugin
TARGET = $$qtLibraryTarget($$TARGET)
uri = ZeroConf
unix:!macx:!android: LIBS += -ldns_sd
!contains(CONFIG,NO_AVAHI): unix:!macx:LIBS +=  -lavahi-client -lavahi-common
win32:LIBS += -L"c:\\PROGRA~1\\BONJOU~1\\lib\\win32" -ldnssd
win32:INCLUDEPATH += "c:\\program files\\bonjour sdk\\include"
android:LIBS += -jmdns
SOURCES += \
         bonjourservicebrowser.cpp \
         bonjourserviceresolver.cpp \
         bonjourbackend.cpp \
         bonjourfrontend.cpp \
         zeroconf.cpp

HEADERS += \
         bonjourservicebrowser.h \
         bonjourserviceresolver.h \
         bonjourrecord.h \
         bonjourbackend.h \
         bonjourfrontend.h \
         zeroconf.h

OTHER_FILES += \
         qmldir
!equals(_PRO_FILE_PWD_, $$OUT_PWD) {
    copy_qmldir.target = $$OUT_PWD/qmldir
    copy_qmldir.depends = $$_PRO_FILE_PWD_/qmldir
    copy_qmldir.commands = $(COPY_FILE) \"$$replace(copy_qmldir.depends, /, $$QMAKE_DIR_SEP)\" \"$$replace(copy_qmldir.target, /, $$QMAKE_DIR_SEP)\"
    QMAKE_EXTRA_TARGETS += copy_qmldir
    PRE_TARGETDEPS += $$copy_qmldir.target
}

qmldir.files = qmldir

unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}
