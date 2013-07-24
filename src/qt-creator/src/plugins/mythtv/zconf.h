#ifndef ZCONF_H
#define ZCONF_H
#include <QString>
#include <QObject>
#include <QTcpSocket>
#include "bonjourrecord.h"
class BonjourServiceBrowser;
class BonjourServiceResolver;
class QHostInfo;
class ZConf : public QObject
{
    Q_OBJECT
    public slots:
   void updateString(const QList<BonjourRecord> &list);
public:
    explicit ZConf(QObject *parent = 0);
    QString getString();
private:
    quint16 blockSize;
    BonjourServiceBrowser *bonjourBrowser;
    BonjourServiceResolver *bonjourResolver;
    QString backend;
};
#endif // ZCONF_H
