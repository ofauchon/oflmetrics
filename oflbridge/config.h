#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    Config();

    unsigned short debug_level;

    unsigned int tcpserver_port;
    unsigned int serial_speed;

    QString serial_path ;
    QString log_file;

    int statsd_port;
    QString statsd_host;

    int db_port;
    QString db_user;
    QString db_pass;
    QString db_host;
    QString db_database;
};

#endif // CONFIG_H
