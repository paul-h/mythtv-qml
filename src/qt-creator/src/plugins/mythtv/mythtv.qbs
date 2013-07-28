import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Mythtv"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: [
            "sql",
            "widgets",
            "xml",
            "network",
            "script",
            "declarative",
            "webkitwidgets",
        ]
    }

    files: [
        "bonjourserviceresolver.cpp",
        "bonjourserviceresolver.h",
        "bonjourservicebrowser.cpp",
        "bonjourservicebrowser.h",
        "mainwindow.cpp",
        "mainwindow.h",
        "moresdkdialog.cpp",
        "moresdkdialog.h",
        "mythtvwindow.cpp",
        "mythtvwindow.h",
        "sshdialog.cpp",
        "sshdialog.h",
        "zconf.cpp",
        "zconf.h",
    ]
}
