#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    Config();

    bool tcpserver_enable;
    unsigned int tcpserver_port;
    bool log_enable;
    QString log_file;
    unsigned short debug_level;
    unsigned int serial_speed;
    QString serial_path ;

    bool db_enable;
    QString db_host;
    QString db_user;
    QString db_pass;
    QString db_database;
};

#endif // CONFIG_H
