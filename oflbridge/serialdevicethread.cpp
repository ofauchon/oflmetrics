#include "serialdevicethread.h"

#include <QtDebug>
#include <config.h>


SerialDeviceThread::SerialDeviceThread(Config *pConfig)
{
    this->myconfig = pConfig;
    this->state=0;
    qDebug() << "SerialDeviceThread: Created on port ." << myconfig->serial_path<< " at speed " << myconfig->serial_speed;

}


void SerialDeviceThread::run(){

    qDebug() << "SerialDeviceThread: Thread started.";

    for(;;){
        switch (state)
        {
        case 0:{

            int ret = LS.Open(myconfig->serial_path.toStdString().c_str(),myconfig->serial_speed);
            if (ret!=1) {
                qDebug("SerialDeviceThread: Can't open serial port %s, wait 3000ms", myconfig->serial_path.toStdString().c_str());
                this->msleep(3000);
                break;
            }
            //qDebug() << "SerialDeviceThread: Serial port opened.";
            state++;
            break;
        }
        case 1:{
            qDebug() << "SerialDeviceThread: Switch to dump_hex mode";
            if (LS.WriteString(":dump_hex\n") <0){
                qDebug() << "SerialDeviceThread:state=1: Can't send command dump_hex";
                state=0;
            }
            state++;
            break;
        }
        case 2:{
            memset(rx_buf,0,SerialDeviceThread::BUFFER_SIZE);
            int ret=LS.ReadString(rx_buf,'\n',128,50);
            if (ret>0) qDebug("SerialDeviceThread:state=2: Reading %d bytes '%s'", ret, rx_buf);
            if ( strlen(rx_buf)>11 && strstr(rx_buf,"OK dump_hex")){
                state=3;
            }
        }
        case 3:{
            memset(rx_buf,0,SerialDeviceThread::BUFFER_SIZE);
            int ret=LS.ReadString(rx_buf,'\n',128,50);
            //            if (ret>0) qDebug("SerialDeviceThread:state=3: Reading %d bytes '%s'", ret, rx_buf);
            if (ret>1 && rx_buf[0] == '>'){
                qDebug("SerialDeviceThread:state=3: Processing command '%s'", rx_buf);
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


