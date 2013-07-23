#ifndef BONJOURFRONTEND_h
#define BONJOURFRONTEND_h
#include <QString>
#include <QTcpSocket>
#include "bonjourrecord.h"
class BonjourServiceBrowser;
class BonjourServiceResolver;
class QHostInfo;
class BonjourFrontend : public QObject
{
    Q_OBJECT
    public slots:
   void updateString(const QList<BonjourRecord> &list);
public:
    explicit BonjourFrontend(QObject *parent = 0);
    Q_INVOKABLE QString getFrontendString();
private:
    quint16 blockSize;
    BonjourServiceBrowser *bonjourBrowser;
    BonjourServiceResolver *bonjourResolver;
    QString frontend;
};
#endif
