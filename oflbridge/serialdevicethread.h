#ifndef SERIALDEVICETHREAD_H
#define SERIALDEVICETHREAD_H

#include <QThread>
#include "serialib.h"
#include <packet.h>

class SerialDeviceThread : public QThread
{
    Q_OBJECT

public:
    SerialDeviceThread(QString pSerialPath, unsigned int pSerialSpeed);
    SerialDeviceThread();
    void run();
    
signals:
    void broadcastPacket(Packet *p);
    
public slots:

private:
    serialib LS;
    unsigned int state;
    QString serial_path;
    unsigned int serial_speed;
    static const int BUFFER_SIZE=255;
    char rx_buf[BUFFER_SIZE];


};

#endif // SERIALDEVICETHREAD_H
