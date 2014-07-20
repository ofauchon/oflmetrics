#include "config.h"

Config::Config()
{

    debug_level=0;
    log_enable=false;
    log_file="";
    serial_speed=0;
    serial_path="";
    tcpserver_enable=false;
    tcpserver_port=5050;
    db_enable=false;
}
