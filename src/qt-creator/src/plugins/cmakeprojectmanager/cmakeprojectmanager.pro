include(../../qtcreatorplugin.pri)

HEADERS = cmakeproject.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    makestep.h \
    cmakerunconfiguration.h \
    cmakeopenprojectwizard.h \
    cmakebuildconfiguration.h \
    cmakeeditorfactory.h \
    cmakeeditor.h \
    cmakehighlighter.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmakevalidator.h \
    cmakeparser.h

SOURCES = cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    makestep.cpp \
    cmakerunconfiguration.cpp \
    cmakeopenprojectwizard.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditorfactory.cpp \
    cmakeeditor.cpp \
    cmakehighlighter.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmakevalidator.cpp \
    cmakeparser.cpp


RESOURCES += cmakeproject.qrc
