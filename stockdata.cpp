#include "stockdata.h"

#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>

StockData::StockData(QObject *parent) : QObject(parent)
{
    loadStockData();
}


int StockData::getCurrentCount(QString ISIN, QDate from, QDate to)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    int count = 0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == BUY || stock.type == SELL)
        {
            count += stock.count;
        }
    }

    return count;
}

double StockData::getTotalPrice(QString ISIN, QDate from, QDate to, sSETTINGS setting)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == BUY || stock.type == SELL)
        {
            double moneyInUSD = 0.0;

            switch(stock.currency)
            {
                case USD: moneyInUSD = stock.price;
                    break;
                case CZK: moneyInUSD = (stock.price * setting.CZK2USD);
                    break;
                case EUR: moneyInUSD = (stock.price * setting.EUR2USD);
                    break;
            }

            if(stock.type == BUY)
            {
                moneyInUSD *= -1.0;
            }

            switch(setting.currency)
            {
                case USD: price += moneyInUSD * stock.count;
                    break;
                case CZK: price += (moneyInUSD * stock.count * setting.USD2CZK);
                    break;
                case EUR: price += (moneyInUSD * stock.count * setting.USD2EUR);
                    break;
            }
        }
    }

    return price;
}

double StockData::getTotalFee(QString ISIN, QDate from, QDate to, sSETTINGS setting)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == TRANSACTIONFEE)
        {
            double moneyInUSD = 0.0;

            switch(stock.currency)
            {
                case USD: moneyInUSD = stock.price;
                    break;
                case CZK: moneyInUSD = (stock.price * setting.CZK2USD);
                    break;
                case EUR: moneyInUSD = (stock.price * setting.EUR2USD);
                    break;
            }

            switch(setting.currency)
            {
                case USD: price += moneyInUSD;
                    break;
                case CZK: price += (moneyInUSD * setting.USD2CZK);
                    break;
                case EUR: price += (moneyInUSD * setting.USD2EUR);
                    break;
            }
        }
    }

    return abs(price);
}

double StockData::getReceivedDividend(QString ISIN, QDate from, QDate to, sSETTINGS setting)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == DIVIDEND || stock.type == TAX)
        {
            double moneyInUSD = 0.0;

            switch(stock.currency)
            {
                case USD: moneyInUSD = stock.price;
                    break;
                case CZK: moneyInUSD = (stock.price * setting.CZK2USD);
                    break;
                case EUR: moneyInUSD = (stock.price * setting.EUR2USD);
                    break;
            }

            switch(setting.currency)
            {
                case USD: price += moneyInUSD;
                    break;
                case CZK: price += (moneyInUSD * setting.USD2CZK);
                    break;
                case EUR: price += (moneyInUSD * setting.USD2EUR);
                    break;
            }
        }
    }

    return price;
}


void StockData::setStockData(const StockDataType &value)
{
    stockData = value;
    saveStockData();
}

StockDataType StockData::getStockData() const
{
    return stockData;
}

double StockData::getTax(QString ticker, QDateTime date, eSTOCKEVENTTYPE type)
{
    QVector<sSTOCKDATA> vector = stockData.value(ticker);

    for(const sSTOCKDATA &data : vector)
    {
        if(data.dateTime == date && data.type == type)
        {
            return data.price;
        }
    }

    return 0.0;
}

bool StockData::loadStockData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + STOCKFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> stockData;
            qFile.close();
            return true;
        }
    }

    return false;
}

void StockData::saveStockData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + STOCKFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << stockData;
        qFile.close();
    }
}

QDataStream &operator<<(QDataStream &out, const sSTOCKDATA &param)
{
    out << param.dateTime;
    out << param.ticker;
    out << param.ISIN;
    out << param.stockName;
    out << static_cast<int>(param.type);
    out << static_cast<int>(param.currency);
    out << param.count;
    out << param.price;
    out << static_cast<int>(param.source);

    return out;
}

QDataStream &operator>>(QDataStream &in, sSTOCKDATA &param)
{
    in >> param.dateTime;
    in >> param.ticker;
    in >> param.ISIN;
    in >> param.stockName;

    int buffer1;
    in >> buffer1;
    param.type = static_cast<eSTOCKEVENTTYPE>(buffer1);

    int buffer2;
    in >> buffer2;
    param.currency = static_cast<eCURRENCY>(buffer2);

    in >> param.count;
    in >> param.price;

    int buffer3;
    in >> buffer3;
    param.source = static_cast<eSTOCKSOURCE>(buffer3);

    return in;
}
