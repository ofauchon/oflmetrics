#include "serialdevicethread.h"

#include <QtDebug>


SerialDeviceThread::SerialDeviceThread(QString pSerialPath, unsigned int pSerialSpeed)
{
    this->serial_path = pSerialPath;
    this->serial_speed = pSerialSpeed;
    this->state=0;
    qDebug() << "SerialDeviceThread: Created on port ." << pSerialPath << " at speed " << pSerialSpeed;

}


void SerialDeviceThread::run(){

    qDebug() << "SerialDeviceThread: Thread started.";

    for(;;){
        switch (state)
        {
        case 0:{

            int ret = LS.Open("/dev/ttyUSB1" , 115200);
            if (ret!=1) {
                qDebug("SerialDeviceThread: Can't open serial port %s, wait 3000ms", serial_path.toStdString().c_str());
                this->msleep(3000);
                break;
            }
            //qDebug() << "SerialDeviceThread: Serial port opened.";
            state++;
            break;
        }
        case 1:{
            qDebug() << "SerialDeviceThread: Switch to dump2 mode";
            if (LS.WriteString(":dump2\n") <0){
                qDebug() << "SerialDeviceThread: Can't send command dump2";
                state=0;
            }
            state++;
            break;
        }
        case 2:{
            memset(rx_buf,0,SerialDeviceThread::BUFFER_SIZE);
            int ret=LS.ReadString(rx_buf,'\n',128,50);
            if (ret>1 && rx_buf[0] == '>'){
                //qDebug("SerialDeviceThread: Reading %d bytes '%s'", ret, rx_buf);
                Packet *p = new Packet(rx_buf+1);
                emit broadcastPacket(p);
            }
            //instruction3;
            break;
        }
        }

        //qDebug("SerialDeviceThread: Sleep: state : %d", state);
        this->msleep(1000);

    }




}


