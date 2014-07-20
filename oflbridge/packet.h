#ifndef PACKET_H
#define PACKET_H

#include <QString>

class Packet
{
public:
    Packet();
    Packet(char* raw);

    QString src_node_id;
    QString dst_node_id;
    unsigned int msg_size;
    QString msg;
};

#endif // PACKET_H
