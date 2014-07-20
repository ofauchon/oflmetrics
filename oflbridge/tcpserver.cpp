#include "tcpserver.h"
#include "serverthread.h"
#include "serialdevicethread.h"



TcpServer::TcpServer(QObject *parent, PacketProcessor *pp)
    : QTcpServer(parent)
{

    //qDebug() << "Server constructor";
    processor=pp;

}

void TcpServer::incomingConnection(int socketDescriptor)
{
    qDebug() << "TcpServer: Incoming connection";
    ServerThread *thread = new ServerThread(socketDescriptor, this);
    connect(serialThread, SIGNAL(rxPacket(QString)), thread, SLOT(debugMessage(QString)));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}


