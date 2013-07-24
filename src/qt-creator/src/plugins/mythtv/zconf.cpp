#include <QtNetwork>
#include <QObject>
#include "zconf.h"
#include "bonjourservicebrowser.h"
#include "bonjourserviceresolver.h"

ZConf::ZConf(QObject *parent)
    : QObject(parent), bonjourResolver(0)
{
    BonjourServiceBrowser *bonjourBrowser = new BonjourServiceBrowser(this);
     setenv("AVAHI_COMPAT_NOWARN","1",1);
    connect(bonjourBrowser, SIGNAL(currentBonjourRecordsChanged(const QList<BonjourRecord> &)),
            this, SLOT(updateString(const QList<BonjourRecord> &)));
    bonjourBrowser->browseForServiceType(QLatin1String("_mythbackend-master._tcp"));
}
void ZConf::updateString(const QList<BonjourRecord> &list)
{
    foreach (BonjourRecord record, list) {
        backend.append(QString(record.serviceName));
    }
}
QString ZConf::getString()
{
    return backend;
}
