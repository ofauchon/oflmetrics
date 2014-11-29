#include "packet.h"
#include <QString>

#include <QDebug>
#include <QStringList>

Packet::Packet()
{
}

bool Packet::initialize(char* pData)
{
    QString *m = new QString(pData);
    QStringList liste = m->split('|', QString::KeepEmptyParts);
    if (liste.size()>3){
        src_node_id = liste.at(0);
        dst_node_id = liste.at(1);
        //msg_size = liste.at(2).toUInt();
        msg = liste.at(3);
        return true;
    } else {
        return false;
    }

}
