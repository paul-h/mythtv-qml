#ifndef MYTHCHANNELS_H
#define MYTHCHANNELS_H
#include <QObject>
#include <QtSql>
class MythChannels : public QObject
{
    Q_OBJECT
public:
    explicit MythChannels(QObject *parent = 0);
    Q_INVOKABLE void getChannels(const QString &m_ChannelID);
private:
//    QSqlQueryModel *model;
};
#endif // MYTHCHANNELS_H
