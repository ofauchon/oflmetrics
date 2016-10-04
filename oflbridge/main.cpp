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
#include <QMutex>

#include <unistd.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#define BUFSZ 256
char in_buf[BUFSZ];


	
static QMutex mutex;

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

Config config;


int daemonize(){
    pid_t pid;
    pid_t sid;

    qDebug("daemonize: start");
    pid = fork();  

    if (pid < 0 )
    { 
        qFatal("daemonize: Fork failed");
    	exit(-1);  
    }
    if (pid > 0) {  
	qInfo("daemonize: Fork done"); 
        exit(EXIT_SUCCESS);  
    }

    sid = setsid(); 
    if (sid < 0) {
	qFatal("daemonize: Fork error (sid<0)"); 
        exit(1);
    }

    /* set the working directory to the root directory */  
    if (chdir ("/") == -1) {
	qFatal("daemonize: Can't chdir /"); 
	    exit(1);  
    }

    /* close all open files--NR_OPEN is overkill, but works */  
//    for (i = 0; i < NR_OPEN; i++)  close (i);  

    /* redirect fd's 0,1,2 to /dev/null */  
    open ("/dev/null", O_RDWR);  
    /* stdin */  
    dup (0);  
    /* stdout */  
    dup (0);  
    /* stderror */  
    return true;
}


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    if (config.log_file.length() >0) {
    mutex.lock();
    QDateTime dateTime(QDateTime::currentDateTime());

    QString timeStr(dateTime.toString("dd-MM-yyyy HH:mm:ss:zzz"));
    QString contextString(QString("(%1, %2)").arg(context.file).arg(context.line));

    QString tDateFic(dateTime.toString("dd-MM-yyyy"));
    QFile outFile(config.log_file);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream stream(&outFile);
    stream << timeStr << ": " << msg << endl;

    outFile.close();
    mutex.unlock();
    } else {


    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "D: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "I: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "W: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "C: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "F: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }

    }
}



int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("OFLbridge");
    QCoreApplication::setApplicationVersion("1.0");



// PARSER
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "OFLmetrics daemon."));
    parser.addHelpOption(); // Standard -h / --help options.
    parser.addVersionOption(); // Standard -v / --version options.

    QCommandLineOption daemonizeOption(QStringList() << "d" << "daemonize", QCoreApplication::translate("main", "Daemonize (fork to background)"));
    parser.addOption(daemonizeOption);

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

    if ( parser.isSet(daemonizeOption) ) 
    { 
	    daemonize();
    }

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

    if ( parser.value(logOption) != "" ) {
	    config.log_file=parser.value(logOption); 
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



    qInfo("Config: Serial '%s':%d InfluxDB '%s'",
           qPrintable(config.serial_path), config.serial_speed,
           qPrintable(config.influxdb_url));



    PacketProcessor *processor = new PacketProcessor(&config);

    //processor->autotest();

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


