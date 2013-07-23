TEMPLATE = lib
TARGET = mythtvtheme
QT += qml quick sdk
CONFIG += qt plugin
TARGET = $$qtLibraryTarget($$TARGET)
# Input
OTHER_FILES += \
      template/*.qml
      template/*.xml
unix {
    installPath = $$[QT_INSTALL_QML]/$$replace(uri, \\., /)
    qmldir.path = $$installPath
    target.path = $$installPath
    INSTALLS += target qmldir
}

