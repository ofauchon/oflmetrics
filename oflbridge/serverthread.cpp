#include "serverthread.h"

#include <QtNetwork>



ServerThread::ServerThread(int socketDescriptor, QObject *parent)
    : QThread(parent), socketDescriptor(socketDescriptor) {
}


void ServerThread::run()
{
    qDebug() << "Thread run()";

    QTcpSocket tcpSocket;
    if (!tcpSocket.setSocketDescriptor(socketDescriptor)){
        emit error(tcpSocket.error());
        return;
    }


    tcpSocket.write("Server TCP\r\n",15);
    this->exec();
}

void ServerThread::debugMessage(QString s){
    qDebug() << " ServerThread debug : " << s;
}

