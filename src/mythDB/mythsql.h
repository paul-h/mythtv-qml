#ifndef MYTHSQL_H
#define MYTHSQL_H
#include <QObject>
class MythSql : public QObject
       {
     Q_OBJECT
public:
    explicit MythSql(QObject *parent = 0);
    Q_INVOKABLE QString getString();
    Q_INVOKABLE void createConnection(const QString &m_HostName,const  QString &m_DbName,const QString &m_User,const QString &m_Password);
    Q_INVOKABLE void insertRecordedArtwork(const QString &m_initref,const QString &m_season,const QString &m_host,const QString &m_coverart,const QString &m_fanart, const QString &m_banner);
//    Q_INVOKABLE void killConnecition();
private:
    QString getBack;
};
#endif // MYTHSQL_H
