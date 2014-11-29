#include "packetprocessor.h"

#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUdpSocket>
#include <QHostAddress>


PacketProcessor::PacketProcessor(Config *qConfig)
{
    config = qConfig;

    //db = QSqlDatabase::addDatabase("QMYSQL");
    //db.setHostName(config->db_host);
    //db.setDatabaseName(config->db_database);
    //db.setUserName(config->db_user);
    //db.setPassword(config->db_pass);

}

void PacketProcessor::insertPacket(Packet *p)
{
    qDebug("PacketProcessor: SRC:%s DST:%s MSG:'%s'",
           qPrintable(p->src_node_id), qPrintable(p->dst_node_id), qPrintable(p->msg));


    // Walk through the packet to find datas;
    QRegExp re("(TEMP|BATLEV):([^;]+);");
    QString message;
    int pos=0;

    while ((pos = re.indexIn(p->msg, pos)) != -1) {
            QString cap_type = re.cap(1).toLower();
            QString cap_val = re.cap(2);
            message+= QString(tr("sensor_%1_%2:0|g\n")).arg(p->src_node_id).arg(cap_type);
            message+= QString(tr("sensor_%1_%2:%3|g\n")).arg(p->src_node_id).arg(cap_type).arg(cap_val);
            pos += re.matchedLength();

    }


    if (message != ""){
        qDebug(QString("PacketProcessor: Sending statsd command '" + message + "'").toStdString().c_str());
        if (config->statsd_host!="") {
            QUdpSocket *udpSocket = new QUdpSocket(this);
            udpSocket->writeDatagram( message.toStdString().c_str(), message.length(), QHostAddress(config->statsd_host), config->statsd_port);
        }

    }
    else
    {
        qDebug("PacketProcessor:Nothing to decode in this packet");
    }



    /*
    if (db.open()){
        QSqlQuery q;
        QString s = "INSERT INTO incoming (src, dst, msg) VALUES ('%1','%2','%3')";
        QString t = s.arg(qPrintable(p->src_node_id), qPrintable(p->dst_node_id), qPrintable(p->msg));
        qDebug("PacketProcessor: SQL : %s", qPrintable(t));
        if (!q.exec(t)) qDebug() << "PacketProcessor: SQL Error : " << q.lastError();
    } else qDebug() << "PacketProcessor: ERROR : Can't open database !!!!" << db.lastError();
    db.close();
    */
}

