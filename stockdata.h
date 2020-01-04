#ifndef STOCKDATA_H
#define STOCKDATA_H

#include <QObject>

class StockData : public QObject
{
    Q_OBJECT
public:
    explicit StockData(QObject *parent = nullptr);

signals:

};

#endif // STOCKDATA_H
