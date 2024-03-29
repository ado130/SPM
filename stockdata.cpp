#include "stockdata.h"

#include <cmath>
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
}


int StockData::getTotalCount(const QString &ISIN, const QDate &from, const QDate &to)
{
    QVector<sSTOCKDATA> vector = stockData.value(ISIN);

    int count = 0.0;

    for(const sSTOCKDATA &stock : vector)
    {
        if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if(stock.type == BUY)
        {
            count += stock.count;
        }
        else if(stock.type == SELL)
        {
            count -= stock.count;
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
                case CAD: rates = "CAD";
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
                case CAD: rates += "CAD";
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

    for (const sSTOCKDATA &stock : vector)
    {
        if ( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

        if (stock.type == BUY || stock.type == SELL)
        {
            QString rates;
            eCURRENCY currencyFrom = stock.currency;

            switch (currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
                case CAD: rates = "CAD";
                    break;
            }

            rates += "2";

            switch (selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
                case CAD: rates += "CAD";
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

            switch (currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
                case CAD: rates = "CAD";
                    break;
            }

            rates += "2";

            switch (selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
                case CAD: rates += "CAD";
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

    for (const QString &key : keys)
    {
        for (const sSTOCKDATA &stock : stockData.value(key))
        {
            if ( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if(stock.type == SELL)
            {
                switch (stock.currency)
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

QVector<sPDFEXPORTDATA> StockData::prepareDataToExport(const QDate &from, const QDate &to, const double &USD2CZK, const double &EUR2CZK, const double &GBP2CZK)
{
    StockDataType stockList = getStockData();

    if (stockList.isEmpty())
    {
        return QVector<sPDFEXPORTDATA>();
    }

    QVector<sPDFEXPORTDATA> exportData;

    bool isSellValueTest = getTotalSell(from, to, EUR2CZK, USD2CZK, GBP2CZK) > 100000 ? true : false;

    QList<QString> keys = stockList.keys();

    for (const QString &key : keys)
    {
        for (const sSTOCKDATA &deg : stockList.value(key))
        {
            if ( !(deg.dateTime.date() >= from && deg.dateTime.date() <= to) ) continue;

            if ( deg.stockName.toLower().contains("fundshare") ) continue;


            if(deg.type == SELL)
            {
                sPDFEXPORTDATA pdfRow;
                pdfRow.type = SELL;

                if(isSellValueTest)      // more than 100000 CZK, so we have to do the tax
                {

                }
                else                        // less than 100000 CZK, print all CP, however we do not have to do any tax
                {
                    switch(deg.currency)
                    {
                        case USD:
                            pdfRow.priceInCZK = round(deg.price * USD2CZK) * deg.count;
                            pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price * deg.count).arg("USD");
                            break;
                        case CZK:
                            pdfRow.priceInCZK = round(deg.price) * deg.count;
                            pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price * deg.count).arg("CZK");
                            break;
                        case EUR:
                            pdfRow.priceInCZK = round(deg.price * EUR2CZK) * deg.count;
                            pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price * deg.count).arg("EUR");
                            break;
                        case GBP:
                            pdfRow.priceInCZK = round(deg.price * GBP2CZK) * deg.count;
                            pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price * deg.count).arg("GBP");
                            break;                         
                    }

                    pdfRow.date = deg.dateTime;
                    pdfRow.name = deg.stockName;

                    exportData.append(pdfRow);
                }
            }
            else if (deg.type == DIVIDEND)
            {
                sPDFEXPORTDATA pdfRow;

                pdfRow.type = DIVIDEND;

                switch(deg.currency)
                {
                    case USD:
                        pdfRow.priceInCZK = round(deg.price * USD2CZK);
                        pdfRow.tax = round(getTax(key, deg.dateTime, DIVIDEND) * USD2CZK);
                        pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price).arg("USD");
                        break;
                    case CZK:
                        pdfRow.priceInCZK = round(deg.price);
                        pdfRow.tax = round(getTax(key, deg.dateTime, DIVIDEND));
                        pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price).arg("CZK");
                        break;
                    case EUR:
                        pdfRow.priceInCZK = round(deg.price * EUR2CZK);
                        pdfRow.tax = round(getTax(key, deg.dateTime, DIVIDEND) * EUR2CZK);
                        pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price).arg("EUR");
                        break;
                    case GBP:
                        pdfRow.priceInCZK = round(deg.price * GBP2CZK);
                        pdfRow.tax = round(getTax(key, deg.dateTime, DIVIDEND) * GBP2CZK);
                        pdfRow.priceInOriginal = QString("%1 %2").arg(deg.price).arg("GBP");
                        break;
                }

                pdfRow.date = deg.dateTime;
                pdfRow.name = deg.stockName;
                pdfRow.tax = abs(pdfRow.tax);

                // Find duplicates, then sum them or create new record
                auto it = std::find_if(exportData.begin(), exportData.end(),
                                       [pdfRow]
                                       (const sPDFEXPORTDATA& pdf) -> bool { return ( (pdf.date.date() == pdfRow.date.date()) && (pdf.name == pdfRow.name) ); }
                                       );

                if (it != exportData.end())
                {
                    it->priceInCZK += pdfRow.priceInCZK;
                    it->tax += pdfRow.tax;
                }
                else
                {
                    exportData.append(pdfRow);
                }
            }
        }
    }

    std::sort(exportData.begin(), exportData.end(),
              []
              (sPDFEXPORTDATA a, sPDFEXPORTDATA b) {return a.date > b.date; }
              );

    return exportData;
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

bool StockData::updateStockDataVector(QString ISIN, QVector<sSTOCKDATA> vector)
{
    auto it = stockData.find(ISIN);

    if (it != stockData.end())
    {
        stockData[ISIN] = vector;
        saveStockData();

        return true;
    }

    return false;
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
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + STOCKFILE);

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
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + STOCKFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << stockData;
        qFile.close();
    }
}

QString StockData::getCachedISINParam(const QString &ISIN, const QString &param)
{
    auto it = std::find_if(cachedStockData.begin(), cachedStockData.end(), [ISIN](QPair<QString, sONLINEDATA> a)
                           {
                               return a.first == ISIN;
                           }
                           );

    if (it != cachedStockData.end())
    {
        if (it->second.row. contains(param))
        {
            return it->second.row.value(param);
        }
    }

    return QString();
}

void StockData::loadOnlineStockInfo()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache/";

    QDir dir(path);

    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    QStringList files = dir.entryList(QStringList() << "*.json" << "*.JSON", QDir::Files);

    for (const QString &file : files)
    {
        QString filePath = path + file;

        QFile loadFile(filePath);

        if (!loadFile.open(QIODevice::ReadOnly))
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

        sONLINEDATA table;

        if (json.keys().count() > 0)
        {
            QString ISIN = json.keys().at(0);
            QJsonObject arr = json.value(ISIN).toObject();

            for (const QString& key : arr.keys())
            {
                QJsonValue value = arr.value(key);

                if (key.contains("Sector"))
                {
                    table.info.sector = value.toString();
                }
                else if (key.contains("Ticker"))
                {
                    table.info.ticker = value.toString();
                }
                else if (key.contains("Country"))
                {
                    table.info.country = value.toString();
                }
                else if (key.contains("Industry"))
                {
                    table.info.industry = value.toString();
                }
                else if (key.contains("Stockname"))
                {
                    table.info.stockName = value.toString();
                }
                else
                {
                    table.row.insert(key, value.toString());
                }

                //qDebug() << "Key = " << key << ", Value = " << value.toString();
            }

            cachedStockData.push_back(qMakePair(ISIN, table));
            //emit updateStockData(ISIN, table);
        }
    }
}

void StockData::saveOnlineStockInfo(const QString &ISIN, const sONLINEDATA &table)
{
    if (table.row.isEmpty() || ISIN.isEmpty()) return;

    auto it = std::find_if(cachedStockData.begin(), cachedStockData.end(), [ISIN](QPair<QString, sONLINEDATA> a)
                           {
                               return a.first == ISIN;
                           }
                           );

    if(it != cachedStockData.end())
    {
        cachedStockData.erase(it);
    }

    cachedStockData.push_back(qMakePair(ISIN, table));

    QJsonObject recordObject;
    recordObject.insert("Sector", table.info.sector);
    recordObject.insert("Ticker", table.info.ticker);
    recordObject.insert("Country", table.info.country);
    recordObject.insert("Industry", table.info.industry);
    recordObject.insert("Stockname", table.info.stockName);

    QStringList keys = table.row.keys();

    for (const QString &key : keys)
    {
        recordObject.insert(key, table.row.value(key));
    }

    QJsonObject obj;
    obj[ISIN]= recordObject;

    QJsonDocument doc(obj);

    QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache/";

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
    out << param.balance;
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
    in >> param.balance;
    in >> param.fee;

    int buffer3;
    in >> buffer3;
    param.source = static_cast<eSTOCKSOURCE>(buffer3);

    return in;
}
