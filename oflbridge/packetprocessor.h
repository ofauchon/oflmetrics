#ifndef PACKETPROCESSOR_H
#define PACKETPROCESSOR_H

#include <QObject>
#include <packet.h>
#include <config.h>
#include <QtSql/QSqlDatabase>
#include <QNetworkAccessManager>



class PacketProcessor : public QObject
{
    Q_OBJECT

public:
    PacketProcessor(Config *qConfig);
    Config *config;
    QSqlDatabase db;
    void autotest(void);
    QNetworkAccessManager nam; 
private:
    void influx_sendmetric(QString node, QString type, QString val);

signals:


public slots:
    void insertPacket(Packet *p);

};

#endif // PACKETPROCESSOR_H
