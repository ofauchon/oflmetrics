#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <serialdevicethread.h>
#include <packetprocessor.h>

class TcpServer : public QTcpServer
{
    Q_OBJECT

public:
    TcpServer(QObject *parent = 0, PacketProcessor *pp = 0);


protected:
    void incomingConnection(int socketDescriptor);

private:
    SerialDeviceThread *serialThread;
    PacketProcessor *processor;
};



#endif // TCPSERVER_H
