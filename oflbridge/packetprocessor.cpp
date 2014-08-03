#include "packetprocessor.h"

#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

/*
| incoming | CREATE TABLE `incoming` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `dt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `src` char(20) DEFAULT NULL,
  `dst` char(20) DEFAULT NULL,
  `msg` char(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=30 DEFAULT CHARSET=utf8 |


 GRANT SELECT,INSERT,UPDATE ON oflmetrics.* TO oflmetrics@localhost IDENTIFIED BY 'oflmetrics';

*/

PacketProcessor::PacketProcessor(Config *qConfig)
{
    config = qConfig;

    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(config->db_host);
    db.setDatabaseName(config->db_database);
    db.setUserName(config->db_user);
    db.setPassword(config->db_pass);

}

void PacketProcessor::insertPacket(Packet *p)
{
    qDebug("PacketProcessor: RX SRC:%s DST:%s MSG:%s",
           qPrintable(p->src_node_id), qPrintable(p->dst_node_id), qPrintable(p->msg));


    if (db.open()){
        QSqlQuery q;
        QString s = "INSERT INTO incoming (src, dst, msg) VALUES ('%1','%2','%3')";
        QString t = s.arg(qPrintable(p->src_node_id), qPrintable(p->dst_node_id), qPrintable(p->msg));
        qDebug("PacketProcessor: SQL : %s", qPrintable(t));
        if (!q.exec(t)) qDebug() << "PacketProcessor: SQL Error : " << q.lastError();
    } else qDebug() << "PacketProcessor: ERROR : Can't open database !!!!" << db.lastError();
    db.close();
}

