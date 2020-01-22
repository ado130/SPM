#include "stockdata.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>


StockData::StockData(QObject *parent) : QObject(parent)
{
    loadStockData();
    loadOnlineStockInfo();
}


int StockData::getCurrentCount(const QString &ISIN, const QDate &from, const QDate &to)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    int count = 0.0;

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

double StockData::getTotalPrice(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0.0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == BUY || stock.type == SELL)
        {
            QString rates;
            eCURRENCY currencyFrom = stock.currency;

            switch(currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
            }

            rates += "2";

            switch(selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
            }

            price += echangeRates[rates](stock.price) * stock.count;
        }
    }

    return abs(price);
}

double StockData::getTotalFee(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0.0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == BUY)
        {
            QString rates;
            eCURRENCY currencyFrom = stock.currency;

            switch(currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
            }

            rates += "2";

            switch(selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
            }

            price += echangeRates[rates](stock.fee);
        }
    }

    return abs(price);
}

double StockData::getReceivedDividend(const QString &ISIN, const QDate &from, const QDate &to, const eCURRENCY selectedCurrency, ExchangeRatesFunctions echangeRates)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    double price = 0.0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == DIVIDEND)
        {
            QString rates;
            eCURRENCY currencyFrom = stock.currency;

            switch(currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
            }

            rates += "2";

            switch(selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
            }

            price += echangeRates[rates](stock.price) + echangeRates[rates](stock.fee);
        }
    }

    return price;
}

double StockData::getTotalSell(const QDate &from, const QDate &to, double EUR2CZK, double USD2CZK, double GBP2CZK)
{
    double price = 0.0;
    QList<QString> keys = stockData.keys();

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockData.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if(stock.type == SELL)
            {
                switch(stock.currency)
                {
                    case USD: price += stock.price * stock.count * USD2CZK;
                        break;
                    case CZK: price += stock.price * stock.count;
                        break;
                    case EUR: price += stock.price * stock.count * EUR2CZK;
                        break;
                    case GBP: price += stock.price * stock.count * GBP2CZK;
                        break;
                }
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

double StockData::getTax(const QString &ticker, const QDateTime &date, const eSTOCKEVENTTYPE &type)
{
    QVector<sSTOCKDATA> vector = stockData.value(ticker);

    for(const sSTOCKDATA &data : vector)
    {
        if(data.dateTime == date && data.type == type)
        {
            return data.fee;
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
    out << param.fee;
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
    in >> param.fee;

    int buffer3;
    in >> buffer3;
    param.source = static_cast<eSTOCKSOURCE>(buffer3);

    return in;
}

void StockData::loadOnlineStockInfo()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/";

    QDir dir(path);

    if(!dir.exists())
    {
        dir.mkpath(".");
    }

    QStringList files = dir.entryList(QStringList() << "*.json" << "*.JSON", QDir::Files);

    for(const QString &file : files)
    {
        QString filePath = path + file;

        QFile loadFile(filePath);

        if(!loadFile.open(QIODevice::ReadOnly))
        {
            qWarning("Couldn't open json file.");
        }

        QJsonParseError errorPtr;
        QJsonDocument doc = QJsonDocument::fromJson(loadFile.readAll(), &errorPtr);

        if (doc.isNull())
        {
            qWarning() << "Parse failed";
        }

        QJsonObject json = doc.object();

        if(json.keys().count() > 0)
        {
            QString ISIN = json.keys().at(0);
            QJsonObject arr = json.value(ISIN).toObject();

            for(const QString& key : arr.keys())
            {
                QJsonValue value = arr.value(key);
                qDebug() << "Key = " << key << ", Value = " << value.toString();
            }
        }
    }
}

void StockData::saveOnlineStockInfo(const sTABLE &table, const QString &ISIN)
{
    if(table.row.isEmpty() || ISIN.isEmpty()) return;


    QJsonObject recordObject;
    recordObject.insert("Sector", table.info.sector);
    recordObject.insert("Ticker", table.info.ticker);
    recordObject.insert("Country", table.info.country);
    recordObject.insert("Industry", table.info.industry);
    recordObject.insert("Stockname", table.info.stockName);

    QStringList keys = table.row.keys();

    for(const QString &key : keys)
    {
        recordObject.insert(key, table.row.value(key));
    }

    QJsonObject obj;
    obj[ISIN]= recordObject;

    QJsonDocument doc(obj);
    qDebug() << doc.toJson();


    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache/";

    QDir dir(path);

    if(!dir.exists())
    {
        dir.mkpath(".");
    }

    QFile file(path + "/" + ISIN + ".json");

    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Couldn't open json file.");
    }

    file.write(doc.toJson(QJsonDocument::Indented));

    file.close();
}
