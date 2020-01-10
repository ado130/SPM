#ifndef TASTYWORKS_H
#define TASTYWORKS_H

#include <QObject>
#include "global.h"

class Tastyworks : public QObject
{
    Q_OBJECT
public:
    explicit Tastyworks(QObject *parent = nullptr);

    void loadCSV(QString path, eDELIMETER delimeter);

signals:

};

#endif // TASTYWORKS_H
