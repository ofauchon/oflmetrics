#ifndef XPLBRIDGE_H
#define XPLBRIDGE_H

#include <QObject>

class XPLBridge : public QCoreApplication
{
    Q_OBJECT
public:
    explicit XPLBridge(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // XPLBRIDGE_H
