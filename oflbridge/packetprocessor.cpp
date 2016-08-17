#include "packetprocessor.h"

#include <QDebug>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QUdpSocket>
#include <QHostAddress>

// For InfluxDB POST
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>


PacketProcessor::PacketProcessor(Config *qConfig)
{
    config = qConfig;
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));

    //db = QSqlDatabase::addDatabase("QMYSQL");
    //db.setHostName(config->db_host);
    //db.setDatabaseName(config->db_database);
    //db.setUserName(config->db_user);
    //db.setPassword(config->db_pass);

}

void PacketProcessor::influx_sendmetric(QString node, QString type, QString val)
{
    if (val.startsWith("+") ) val.remove(0,1);

    // InfluxDB payload (https://docs.influxdata.com/influxdb/v0.13/guides/writing_data/)
    QByteArray jsonString = QString(tr("sensors,id=RDB_%1,type=%2 value=%3")).arg(node).arg(type).arg(val).toLatin1();
    QByteArray postDataSize = QByteArray::number(jsonString.size());
    qDebug("PacketProcessor: InfluxDb message: '%s' length: %d", qPrintable(jsonString), jsonString.size());


    QUrl req(config->influxdb_url);
    QNetworkRequest request(req);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    request.setRawHeader("Content-Length", postDataSize);
    request.setRawHeader("User-Agent", "oflbridge");

    nam->post( request, jsonString);

    qDebug("PacketProcessor: InfluxDb destination: '%s'", qPrintable(config->influxdb_url));

}



void PacketProcessor::insertPacket(Packet *p)
{
    qDebug("PacketProcessor: SRC:%s DST:%s MSG:'%s'",
           qPrintable(p->src_node_id), qPrintable(p->dst_node_id), qPrintable(p->msg));

    // Walk through the packet to find datas;
    QRegExp re("(TEMP|BATLEV|FWVER|CAPA|ERROR):([^;]+);");
    int pos=0;

    while ((pos = re.indexIn(p->msg, pos)) != -1) {

        QString cap_type = re.cap(1).toLower();
        QString cap_val = re.cap(2);
        qDebug(QString("PacketProcessor: Found message : ID:%1 TYPE:%2 VALUE:%3").arg(p->dst_node_id).arg(cap_type).arg(cap_val).toStdString().c_str() );

        if (config->influxdb_url != "" ) influx_sendmetric(p->src_node_id, cap_type, cap_val);

        pos += re.matchedLength();

    }

    /*
    if (message != ""){
        qDebug(QString("PacketProcessor: Sending statsd command '" + message + "'").toStdString().c_str());
        if (config->statsd_host!="") {
            QUdpSocket *udpSocket = new QUdpSocket(this);
            udpSocket->writeDatagram( message.toStdString().c_str(), message.length(), QHostAddress(config->statsd_host), config->statsd_port);
        }
}

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


/*
    void PacketProcessor::replyMetaDataChanged() {
        qDebug() << "Metadata changed";
        qDebug() << "Content-Length:" << reply->header(QNetworkRequest::ContentLengthHeader);
        size = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
    }
    void PacketProcessor::slotReadyRead(QNetworkReply* reply) {
        QByteArray ba = reply->readAll();
        fetched += ba.size();
        qDebug() << "Read" << fetched << "of" << size;
 
    }
*/
    void PacketProcessor::replyFinished(QNetworkReply* reply) {
        int httpStatus = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "PacketProcessor: Finished "  << reply->error() << " status:" << httpStatus;
    if(reply->error())
    {
        qDebug() << "PacketProcessor: ERROR! " << reply->errorString();
    } else {
	qDebug() << "PacketProcessor: HTTP Response body:" << reply->readAll();
	}
    reply->deleteLater();
    }

// End Slots

void PacketProcessor::autotest(){
    this->influx_sendmetric("sensor_0000001", "temp", "23.107");
}
