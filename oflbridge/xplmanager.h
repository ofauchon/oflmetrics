#ifndef XPLMANAGER_H
#define XPLMANAGER_H

#include <QObject>

class XPLManager : public QObject
{
    Q_OBJECT
public:
    explicit XPLManager(QObject *parent = 0);
    void init(void);

signals:

public slots:

};

#endif // XPLMANAGER_H
