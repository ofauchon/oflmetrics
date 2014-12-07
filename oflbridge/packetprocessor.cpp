#include "packetprocessor.h"

#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUdpSocket>
#include <QHostAddress>

// For InfluxDB POST
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>


PacketProcessor::PacketProcessor(Config *qConfig)
{
    config = qConfig;

    //db = QSqlDatabase::addDatabase("QMYSQL");
    //db.setHostName(config->db_host);
    //db.setDatabaseName(config->db_database);
    //db.setUserName(config->db_user);
    //db.setPassword(config->db_pass);

}

void PacketProcessor::influx_sendmetric(QString node, QString type, QString temp)
{
    QByteArray jsonString = QString(tr("[{\"name\":\"%1.%2.gauge\",\"columns\":[\"value\"],\"points\":[[%3]]}]")).arg(node).arg(type).arg(temp).toLatin1();
    qDebug("PacketProcessor:: InfluxDb json message: '%s'", qPrintable(jsonString));

    QByteArray postDataSize = QByteArray::number(jsonString.size());

    QUrl req(config->influxdb_url);
    QNetworkRequest request(req);
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkReply * reply = nam->post( request, jsonString);
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
        qDebug(QString("PacketProcessor: Found message : ID:%1 TYPE:%2 VALUE:%3").arg(p->dst_node_id).arg(cap_type).arg(cap_val).toStdString().c_str() );

        message+= QString(tr("sensor_%1_%2:0|g\n")).arg(p->src_node_id).arg(cap_type);
        message+= QString(tr("sensor_%1_%2:%3|g\n")).arg(p->src_node_id).arg(cap_type).arg(cap_val);

        if (config->influxdb_url != "" ) influx_sendmetric(p->src_node_id, cap_type, cap_val);

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



void PacketProcessor::autotest(){
    this->influx_sendmetric("sensor_0000001", "temp", "23.107");
}
