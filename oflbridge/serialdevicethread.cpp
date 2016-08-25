#include "serialdevicethread.h"

#include <QtDebug>
#include <config.h>


SerialDeviceThread::SerialDeviceThread(Config *pConfig)
{
    this->myconfig = pConfig;
    this->state=0;
    qDebug() << "SerialDeviceThread: Created on port ." << myconfig->serial_path<< " at speed " << myconfig->serial_speed;

}

int SerialDeviceThread::write(char* pBuf, unsigned int pSize){
    //this->oDebug("serial write %d bytes to serial", pSize);
   int ret= LS.Write(pBuf, pSize);
   if (ret>0) oDebug("serial write returned %d", ret);
   return ret;
}

int SerialDeviceThread::readString(char* pBuf, char pStop){
    //this->oDebug("readString with separator '%c'", pStop);
    int ret = LS.ReadString(pBuf,pStop,128,SerialDeviceThread::READ_TMOUT);
    //if (ret>0) oDebug("readString read string: '%s' (%d bytes)", specialChar(pBuf), strlen(pBuf));
   return ret;
}

int SerialDeviceThread::writeString(char* pString){
    this->oDebug("writeString '%s'", specialChar(pString));
    int ret = LS.WriteString(pString);
    if (ret<0) oDebug("writeString returns %d", ret);
   return ret;
}

// WTF... malloc for string cleaning ... is that smart ?
char* SerialDeviceThread::specialChar(char* src)
{
  // output buffer to 0
  char *dst;
  dst=(char*)malloc(SerialDeviceThread::BUFFER_SIZE);
  memset(dst,0,SerialDeviceThread::BUFFER_SIZE);

  int srcSz = strlen(src);
  if (srcSz > SerialDeviceThread::BUFFER_SIZE-1) srcSz=SerialDeviceThread::BUFFER_SIZE-1;

  int posSrc,posDst;

  for (posSrc=0,posDst=0; posSrc<srcSz; posSrc++ ){

      if (src[posSrc]=='\r')
      {
          dst[posDst++]='\\';
          dst[posDst++]='r';
      }
      else if (src[posSrc]=='\n')
      {
          dst[posDst++]='\\';
          dst[posDst++]='n';
      } else dst[posDst++]=src[posSrc];
  }
  return dst;
}


void SerialDeviceThread::run(){

    qDebug("SerialDeviceTread:: Thread started.");
    for(;;){
        switch (state)
        {
        case 0:{
            oDebug("SerialDeviceTread:: State 0");
            int ret = LS.Open(myconfig->serial_path.toStdString().c_str(),myconfig->serial_speed);
            if (ret!=1) {
                oDebug("SerialDeviceTread:: Can't open serial port %s, wait 3000ms", myconfig->serial_path.toStdString().c_str());
                this->msleep(3000);
                break;
            }
            //qDebug() << "SerialDeviceThread: Serial port opened.";
            state++;
            break;
        }
        case 1:{
            oDebug("SerialDeviceTread:: State 1, send ':dump_human' command");
            if (LS.WriteString(":dump_human\n") <0){
                oDebug("SerialDeviceTread:: Can't write command to serial port, closing port");
                LS.Close();
                state=0;
            } else state++;
            break;
        }
        case 2:{
            oDebug("SerialDeviceTread:: State 2, waiting for 'OK'");
            memset(rx_buf,0,SerialDeviceThread::BUFFER_SIZE);
            int ret=this->readString(rx_buf,'\n');
            if (ret>0) {
                //oDebug("SerialDeviceTread:: Receiving %d bytes '%.*s'",ret , ret, rx_buf);
                if ( strstr(rx_buf,"OK dump_human mode")!=NULL ){
                   oDebug("SerialDeviceTread:: Now in dump_human mode (State 3) ");
                   state=3;
                }
            }
            break;
        }
        case 3:{
            //oDebug("SerialDeviceTread:: State 3, Listening node's message");
            memset(rx_buf,0,SerialDeviceThread::BUFFER_SIZE);
            int ret= this->readString(rx_buf,'\n');
            if (ret>1) {
                if (rx_buf[0] == 'R' && rx_buf[1] == 'X'){
               // oDebug("SerialDeviceTread:: Receiving data '%s'", rx_buf);
                Packet *p = new Packet();
                p->initialize(rx_buf+4);
                emit broadcastPacket(p);
                }
            }
            break;
        }
        }//switch

        //qDebug("Sleeping (state:%d)", state);
        this->msleep(100);

    }//for

}

// Obsolete code  ?
void SerialDeviceThread::oDebug(const char* msg, ...)
{
    fprintf(stdout, "SERIAL: ");
    va_list ap;
    va_start(ap,msg);
    vprintf(msg,ap);
    va_end(ap);
    fprintf(stdout, "\n");
    fflush(stdout);
}



