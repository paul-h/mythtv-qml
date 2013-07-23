#include "mythsql.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
MythSql::MythSql(QObject *parent) :
    QObject(parent)
{
}
void MythSql::createConnection(const QString &m_HostName,const  QString &m_DbName,const QString &m_User,const QString &m_Password)
{
    QSqlDatabase  db = QSqlDatabase::addDatabase(QLatin1String("QMYSQL") );
    db.setHostName(m_HostName);
    db.setDatabaseName(m_DbName);
    db.setUserName(m_User);
    db.setPassword(m_Password);
    db.setPort(3306);
    if (!db.open())
    {
        qDebug() << db.lastError().text();
        getBack.append("Database Error");
    }
}
void MythSql::insertRecordedArtwork(const QString &m_initref,const QString &m_season,const QString &m_host,const QString &m_coverart,const QString &m_fanart, const QString &m_banner)
{
    QSqlQuery qry;
    qry.prepare("INSERT INTO recordedartwork (inetref, season, host,coverart,fanart,banner)VALUES ( :initref,:season,:host,:coverart,:fanart,:banner)");
    qry.bindValue(":initref", m_initref);
    qry.bindValue(":season", m_season);
    qry.bindValue(":host", m_host);
    qry.bindValue(":coverart", m_coverart);
    qry.bindValue(":fanart", m_fanart);
    qry.bindValue(":banner", m_banner);
    if(qry.exec())
    {
        qDebug() << "Inserted Data";
    }
    else {
        qDebug() << "SOMETHING WHEN WRONG";
    }
}

//void MythSql::killConnecition(){
//    db.close();
//}

QString MythSql::getString()
{
    return getBack ;
}
