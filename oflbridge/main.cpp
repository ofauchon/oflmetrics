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



void help(void){
    printf ("-OFLbridge\n\n");
    printf ("\n");
    printf ("Global options-d\n\n");
    printf (" -d\n\n");
    printf ("-h this help message\n");
    printf ("  -s Serial port speed  (default 115200)\n");
    printf ("  -p Serial port device (default /dev/ttyUSB0)\n");
    printf ("  -t 5050\n");
    printf ("\n");
    printf ("MySQL backend:e\n");
    printf ("  -D Database name\n");
    printf ("  -H Database host\n");
    printf ("  -U Database user\n");
    printf ("  -P Database host\n\n");
}

void parseArgs(QCoreApplication *qApplication, Config *qConfig)
{


    int num = qApplication->arguments().size();

    if (num == 1) {
        help();
        exit(0);
    }

    for ( int i = 0; i < num; i++ ) {
        QString s = qApplication->arguments()[i] ;

        if ( s.startsWith( "-d" ) ) {
            qConfig->debug_level=1;
        }
        // Log file
        else if ( s.startsWith( "-l" ) && i < num) {
            qConfig->log_enable=true;
            qConfig->log_file= QString(qApplication->arguments()[i+1]);
        }
        // Serial port
        else if ( s.startsWith( "-s" ) && i < num) {
            qConfig->serial_speed = qApplication->arguments()[i+1].toInt();
        }
        // Socket
        else if ( s.startsWith( "-t" ) && i < num) {
            qConfig->tcpserver_enable= true;
            qConfig->tcpserver_port = qApplication->arguments()[i+1].toInt();
        }
        else if ( s.startsWith( "-D" ) && i < num) {
            qConfig->db_database = qApplication->arguments()[i+1];
        }
        else if ( s.startsWith( "-H" ) && i < num) {
            qConfig->db_host = qApplication->arguments()[i+1];
        }
        else if ( s.startsWith( "-U" ) && i < num) {
            qConfig->db_user = qApplication->arguments()[i+1];
        }
        else if ( s.startsWith( "-P" ) && i < num) {
            qConfig->db_pass = qApplication->arguments()[i+1];
        }

        // Seria port path
        else if ( s.startsWith( "-p" ) && i < num) {
            qConfig->serial_path = QString(qApplication->arguments()[i+1]);
        }
        else if ( s.startsWith( "-h" ) ) {
            help();
            exit(0);
        }

    }

    if (qConfig->serial_speed==0 ) qConfig->serial_speed=115200;
    if (qConfig->serial_path.length()==0) qConfig->serial_path=QString("/dev/ttyUSB0");

    if (qConfig->log_enable) qDebug() << "Log file: " << qConfig->log_file;
    if (qConfig->debug_level) qDebug() << "Debug enabled";
    if (qConfig->serial_speed>0) qDebug() << "Serial:" << qConfig->serial_path << ":" << qConfig->serial_speed;
    if (qConfig->tcpserver_enable) qDebug() << "Tcpserver Enabled on port :" << qConfig->tcpserver_port;
    if (qConfig->db_database.length() && qConfig->db_host.length() && qConfig->db_user.length() && qConfig->db_pass.length()){
        qConfig->db_enable = true;
    }
    qDebug("DB Enabled : host:%s user:%s database:%s",
           qPrintable(qConfig->db_host), qPrintable(qConfig->db_user), qPrintable(qConfig->db_database));


}





int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Config config;
    parseArgs(&app, &config);

    PacketProcessor *processor = new PacketProcessor(&config);

    // SERIAL THREAD
    SerialDeviceThread *serial_thread = new SerialDeviceThread(config.serial_path, config.serial_speed);
    serial_thread->start();
    QObject::connect(serial_thread, SIGNAL(broadcastPacket(Packet*)), processor, SLOT(insertPacket(Packet*)));


    // TCP SERVER
    if (config.tcpserver_enable){
        TcpServer *server = new TcpServer(processor);
        if ( !server->listen(QHostAddress::LocalHost, config.tcpserver_port) ){
            qDebug() << "Main: Server can't bind on port TCP" << config.tcpserver_port;
            return(-1);
        }
    }

    app.exec();
}


