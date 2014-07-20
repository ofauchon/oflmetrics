#ifndef SERIALDEVICETHREAD_H
#define SERIALDEVICETHREAD_H

#include <QThread>
#include "serialib.h"
#include "packet.h"
#include "config.h"
class SerialDeviceThread : public QThread
{
    Q_OBJECT

public:
    SerialDeviceThread(Config *pConfig);
    SerialDeviceThread();
    void run();
    
signals:
    void broadcastPacket(Packet *p);
    
public slots:

private:
    Config *myconfig;
    serialib LS;
    unsigned int state;
    static const int BUFFER_SIZE=255;
    char rx_buf[BUFFER_SIZE];


};

#endif // SERIALDEVICETHREAD_H
