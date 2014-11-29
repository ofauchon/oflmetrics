#ifndef PACKET_H
#define PACKET_H

#include <QString>
#include <QHash>




/*
 * Packet class contains an OFLNode Frame
 *
 * src_node_id :    Source node identifier
 * dst_node_id :    Destination node identifier
 * msg :            Packet payload
 */

class Packet
{
public:
    Packet();
    bool initialize(char* pData);
    QString src_node_id;
    QString dst_node_id;
    QString msg;
};






#endif // PACKET_H
