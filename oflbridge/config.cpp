#include "config.h"

Config::Config()
{

    debug_level=0;

    log_file="";

    serial_speed=0;
    serial_path="";

    tcpserver_port=5050;

    statsd_host="";
    statsd_port=0;

    db_port=0;
    db_user="";
    db_pass="";
    db_host="";
    db_database="";
}
