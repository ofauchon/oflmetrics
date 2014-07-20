#ifndef SERVERTHREAD_H
#define SERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>

class ServerThread : public QThread
{
    Q_OBJECT

public:
    ServerThread(int socketDescriptor, QObject *parent);
    void run();

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    int socketDescriptor;


public slots:
    void debugMessage(QString s);

};

#endif // SERVERTHREAD_H
