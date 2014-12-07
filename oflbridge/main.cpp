#include <QtCore/QCoreApplication>
#include <QFile>

#include <stdio.h>
#include <QStringList>
#include <QDateTime>
#include <QtDebug>
#include <QThread>
#include <tcpserver.h>
#include <serialdevicethread.h>
#include <config.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#define BUFSZ 256
char in_buf[BUFSZ];


/* PACKET STUFF */
#define PAQUET_MAX_DATASIZE 128

#define uint8_t unsigned char
typedef struct {
    uint8_t         smac[4];
    uint8_t         dmac[4];
    uint8_t         datalen;
    uint8_t         data[PAQUET_MAX_DATASIZE];
    uint8_t         crc[2];
} paquet;





int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("OFLbridge");
    QCoreApplication::setApplicationVersion("1.0");

    Config config;


// PARSER
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "OFLmetrics daemon."));
    parser.addHelpOption(); // Standard -h / --help options.
    parser.addVersionOption(); // Standard -v / --version options.

    QCommandLineOption serialportOption(QStringList() << "s" << "serial_port", QCoreApplication::translate("main", "Dongle serial port"), QCoreApplication::translate("main", "/dev/ttyXXX@speed"));
    parser.addOption(serialportOption);

    QCommandLineOption tcpportOption(QStringList() << "p" << "tcp_port", QCoreApplication::translate("main", "Enable TCP Server port."), QCoreApplication::translate("main", "0-65535"));
    parser.addOption(tcpportOption);

    QCommandLineOption statsdOption(QStringList() << "S" << "statsd_destination", QCoreApplication::translate("main", "Send metrics to statsd server."), QCoreApplication::translate("main", "statsd host"));
    parser.addOption(statsdOption);

    QCommandLineOption logOption(   QStringList() << "l" << "logfile",    QCoreApplication::translate("main", "Log file (./oflmetrics.log)"), QCoreApplication::translate("main", "logfile"));
    parser.addOption(logOption);

    QCommandLineOption influxdbOption(   QStringList() << "i" << "influxdb_destination",    QCoreApplication::translate("main", "Sends metric to InfluxDB server (ttp://influxdb-server:8083/db/mydatabase/series?u={user}&p={pass})"), QCoreApplication::translate("main", "influxdb url"));
    parser.addOption(influxdbOption);

    QCommandLineOption sqlOption(   QStringList() << "m" << "mysql-connection",    QCoreApplication::translate("main", "MySQL (user@server:port:db"), QCoreApplication::translate("main", "mysql connection"));
    parser.addOption(sqlOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    // Check that format is a number between 1 and 4.

    if ( true || parser.value(serialportOption) != "" )   // Check tcp port server
    {
        QRegExp re_serial("(/dev/[a-zA-Z0-9]+):([0-9]+)");
        if ( re_serial.indexIn( parser.value(serialportOption)) > -1 ){
            config.serial_path=re_serial.cap(1);
            config.serial_speed=re_serial.cap(2).toInt();
        }
        else
        {
        fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: Serial Port should look like /dev/ttyUSB1:115200  ")));
        parser.showHelp(1);
        }
    }

    if ( parser.value(influxdbOption) != "" )   // Check tcp port server
    {
        QRegExp re_influx("^(http://.*)");
        if ( re_influx.indexIn( parser.value(influxdbOption)) >  -1 ){
            config.influxdb_url=re_influx.cap(1);
        }
        else
        {
        fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: influxdb parameter should look like http://influxdb.server/db/database/series?u=USER&p=PASS")));
        parser.showHelp(1);
        }
    }

    if ( parser.value(sqlOption) != "" )   // Check tcp port server
    {
        QRegExp re_sql("(\\w+):(\\w+)@([\\w\\.]+):(\\d+):(\\w+)");
        if ( re_sql.indexIn( parser.value(sqlOption)) > -1 ){
            config.db_user=re_sql.cap(1);
            config.db_pass=re_sql.cap(2);
            config.db_host=re_sql.cap(3);
            config.db_port=re_sql.cap(4).toInt();
            config.db_database=re_sql.cap(5);
        }
        else
        {
        fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: mysql parameter should look like user:pass@server:port:database")));
        parser.showHelp(1);
        }
    }
    if ( parser.value(statsdOption) != "" )   // Check tcp port server
    {
        QRegExp re_statsd("([\\w\\.]+):(\\d+)");
        if ( re_statsd.indexIn( parser.value(statsdOption)) > -1 ){
            config.statsd_host=re_statsd.cap(1);
            config.statsd_port=re_statsd.cap(2).toInt();
        }
        else
        {
            fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: statsd parameter should look like host:port (ex:mysserver:8125)")));
            parser.showHelp(1);
        }
    }


    if ( parser.value(tcpportOption) != "" )   // Check tcp port server
    {
        config.tcpserver_port=parser.value(tcpportOption).toInt();
        if ( config.tcpserver_port<1 || config.tcpserver_port>65535) {
            config.tcpserver_port=0;
            fprintf(stderr, "%s\n", qPrintable(QCoreApplication::translate("main", "Error: tcp server port should be >0 and <65535")));
            parser.showHelp(1);
        }
    }



    qDebug("Config:\n\
           Serial '%s':%d\n\
           STATSD '%s':%d\n\
           InfluxDB '%s'\n\
           MySQL: %s:%s@%s:%d:%s\n\
           Tcpserver_port:%u\n",
           qPrintable(config.serial_path), config.serial_speed,
           qPrintable(config.statsd_host), config.statsd_port,
           qPrintable(config.influxdb_url),
           qPrintable(config.db_user),qPrintable(config.db_pass),qPrintable(config.db_host),config.db_port,qPrintable(config.db_database),
           config.tcpserver_port);



















    PacketProcessor *processor = new PacketProcessor(&config);

    // SERIAL THREAD
    SerialDeviceThread *serial_thread = new SerialDeviceThread(&config);
    serial_thread->start();

    QObject::connect(serial_thread, SIGNAL(broadcastPacket(Packet*)), processor, SLOT(insertPacket(Packet*)));

    // TCP SERVER
    if (config.tcpserver_port!=0){
        TcpServer *server = new TcpServer(processor);

        if ( !server->listen(QHostAddress::LocalHost, config.tcpserver_port) ){
            qDebug() << "Main: Server can't bind on port TCP" << config.tcpserver_port;
            return(-1);
        }
    }

    app.exec();
}


