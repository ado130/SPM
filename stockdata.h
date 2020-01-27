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
    double getTotalSell(const QDate &from, const QDate &to, double EUR2CZK, double USD2CZK, double GBP2CZK);

    void loadOnlineStockInfo();
    void saveOnlineStockInfo(const QString &ISIN, const sONLINEDATA &table);


    double getCachedISINPrice(QString ISIN);
private:
    StockDataType stockData;
    QVector<QPair<QString, sONLINEDATA> > cachedStockData;

    bool loadStockData();

signals:
    void updateStockData(QString ISIN, sONLINEDATA table);
};

QDataStream& operator<<(QDataStream& out, const sSTOCKDATA& param);
QDataStream& operator>>(QDataStream& in, sSTOCKDATA& param);

#endif // STOCKDATA_H
