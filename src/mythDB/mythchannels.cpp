#include "mythchannels.h"
#include <QtSql>
#include <QDebug>
#include <QObject>
#include "mythsql.h"
MythChannels::MythChannels(QObject *parent) :
    QObject(parent)
{
}
void MythChannels::getChannels(const QString &m_ChannelID)
{
    QSqlQuery model("SELECT * FROM program WHERE chanid=:chanelID");
    model.bindValue("channelID",m_ChannelID);
     int startNo = model.record().indexOf("starttime");
    while(model.next()){
            QDateTime desc = model.value(startNo).toDateTime();
            QList<QDateTime> k ;
            k << desc;
            qSort(k);
            foreach (QDateTime s, k ) {
                qDebug() << s ;
            }
    }
}

