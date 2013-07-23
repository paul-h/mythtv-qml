#include "plugin.h"
#include "scriptlauncher.h"
#include "deviceinfo.h"
#include <qqml.h>
void Plugin::registerTypes(const char *uri)
{
    // @uri AppLauncher
    qmlRegisterType<ScriptLauncher>(uri,1,0,"ScriptLauncher");
    qmlRegisterType<DeviceInfo>(uri, 1,0, "DeviceInfo");
}
