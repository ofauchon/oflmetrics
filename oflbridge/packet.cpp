#include "packet.h"
#include <QString>

#include <QDebug>
#include <QStringList>

Packet::Packet()
{
}

Packet::Packet(char *raw){
    QString *m = new QString(raw);
    // qDebug() << "Packet: New Instance: " << *m;
    QStringList liste = m->split('|', QString::KeepEmptyParts);
    if (liste.size()>3){
        src_node_id = liste.at(0);
        dst_node_id = liste.at(1);
        //msg_size = liste.at(2).toUInt();
        msg = liste.at(3);
    }

}
