#ifndef PACKETPROCESSOR_H
#define PACKETPROCESSOR_H

#include <QObject>
#include <packet.h>
#include <config.h>
#include <QtSql/QSqlDatabase>


class PacketProcessor : public QObject
{
    Q_OBJECT

public:
    PacketProcessor(Config *qConfig);
    Config *config;
    QSqlDatabase db;
signals:


public slots:
    void insertPacket(Packet *p);

};

#endif // PACKETPROCESSOR_H
