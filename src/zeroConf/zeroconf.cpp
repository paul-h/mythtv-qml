#include "zeroconf.h"
#include "bonjourbackend.h"
#include   "bonjourfrontend.h"
#include <qqml.h>
    void ZeroConf::registerTypes(const char *uri)
    {
        // @uri AppLauncher
        qmlRegisterType<BonjourBackend>(uri, 1, 0, "BonjourBackend");
        qmlRegisterType<BonjourFrontend>(uri,1,0,"BonjourFrontend");
    }
