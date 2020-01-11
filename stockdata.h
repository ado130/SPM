#ifndef STOCKDATA_H
#define STOCKDATA_H

#include <QObject>

#include "global.h"

class StockData : public QObject
{
    Q_OBJECT
public:
    explicit StockData(QObject *parent = nullptr);

    void setStockData(const StockDataType &value);
    StockDataType getStockData() const;

    double getTax(QString ticker, QDateTime date, eSTOCKEVENTTYPE type);
    void saveStockData();

    int getCurrentCount(QString ISIN, QDate from, QDate to);
    double getTotalPrice(QString ISIN, QDate from, QDate to, sSETTINGS setting);
    double getTotalFee(QString ISIN, QDate from, QDate to, sSETTINGS setting);
    double getReceivedDividend(QString ISIN, QDate from, QDate to, sSETTINGS setting);
signals:

private:
    StockDataType stockData;

    bool loadStockData();
};

QDataStream& operator<<(QDataStream& out, const sSTOCKDATA& param);
QDataStream& operator>>(QDataStream& in, sSTOCKDATA& param);

#endif // STOCKDATA_H
