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

    double getTax(const QString &ticker, const QDateTime &date, const eSTOCKEVENTTYPE &type);
    void saveStockData();

    int getTotalCount(const QString &ISIN, const QDate &from, const QDate &to);
    double getTotalPrice(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates);
    double getTotalFee(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates);
    double getReceivedDividend(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates);
    void saveOnlineStockInfo(const sTABLE &table, const QString &ISIN);

    double getTotalSell(const QDate &from, const QDate &to, double EUR2CZK, double USD2CZK, double GBP2CZK);
signals:

private:
    StockDataType stockData;

    bool loadStockData();
    void loadOnlineStockInfo();
};

QDataStream& operator<<(QDataStream& out, const sSTOCKDATA& param);
QDataStream& operator>>(QDataStream& in, sSTOCKDATA& param);

#endif // STOCKDATA_H
