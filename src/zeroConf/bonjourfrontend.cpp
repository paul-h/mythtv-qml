#include <QtNetwork>
#include "bonjourfrontend.h"
#include "bonjourservicebrowser.h"
#include "bonjourserviceresolver.h"
BonjourFrontend::BonjourFrontend(QObject *parent)
    : QObject(parent), bonjourResolver(0)
{
    BonjourServiceBrowser *bonjourBrowser = new BonjourServiceBrowser(this);
     setenv("AVAHI_COMPAT_NOWARN","1",1);
    connect(bonjourBrowser, SIGNAL(currentBonjourRecordsChanged(const QList<BonjourRecord> &)),
            this, SLOT(updateString(const QList<BonjourRecord> &)));
    bonjourBrowser->browseForServiceType(QLatin1String("_mythfrontend._tcp"));
}
void BonjourFrontend::updateString(const QList<BonjourRecord> &list)
{
    foreach (BonjourRecord record, list) {
        frontend.append(record.serviceName.toLatin1());
    }
}
QString BonjourFrontend::getFrontendString()
{
    return frontend;
}
