#include "tastyworks.h"

#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QDataStream>

Tastyworks::Tastyworks(QObject *parent) : QObject(parent)
{
    isRAWFile = loadRawData();
}

void Tastyworks::loadCSV(QString path, eDELIMETER delimeter)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
        return;
    }

    StockDataType stockData;

    char chDelimeter = ',';
    switch (delimeter)
    {
        case COMMA_SEPARATED:
            chDelimeter = ','; break;
        case SEMICOLON_SEPARATED:
            chDelimeter = ';'; break;
        case POINT_SEPARATED:
            chDelimeter = '.'; break;
    }

    bool isFirstLine = false;

    while (!file.atEnd())
    {
        QString line = file.readLine();
        QStringList list = line.split(chDelimeter, Qt::KeepEmptyParts);

        if(!isFirstLine)
        {
            isFirstLine = true;
            continue;
        }

        QString tmp = list.at(0).mid(0, list.at(0).indexOf("T"));
        QDate d = QDate::fromString(tmp, "yyyy-MM-dd");

        tmp = list.at(0).mid(list.at(0).indexOf("T")+1, 8);
        QTime t = QTime::fromString(tmp, "hh:mm:ss");

        QDateTime dt(d, t);

        sTASTYWORKSRAW raw;
        raw.dateTime = dt;

        if(!list.at(12).isEmpty())
        {
            raw.ticker = list.at(12);
        }
        else
        {
            raw.ticker = list.at(3);
        }


        raw.description = list.at(5);

        if(list.at(2).contains("BUY"))
        {
            raw.type = BUY;
            raw.count = list.at(7).toInt();
            raw.price = list.at(8).toDouble();
            raw.fee = list.at(9).toDouble() + list.at(10).toDouble();
        }
        else if(list.at(2).contains("SELL"))
        {
            raw.type = SELL;
            raw.count = list.at(7).toInt();
            raw.price = list.at(8).toDouble();
            raw.fee = list.at(9).toDouble() + list.at(10).toDouble();
        }
        else if(list.at(5).contains("Regulatory fee"))
        {
            raw.type = FEE;
            raw.count = list.at(7).toInt();
            raw.price = list.at(6).toDouble();
            raw.fee = list.at(9).toDouble() + list.at(10).toDouble();
        }

        if(list.at(1).contains("Money Movement") && list.at(4).contains("Equity"))
        {
            double price = list.at(6).toDouble();

            raw.count = list.at(7).toInt();
            raw.price = price;
            raw.fee = 0.0;

            if(price > 0)   // dividend
            {
                raw.type = DIVIDEND;
            }
            else            //fee
            {
                raw.type = TAX;
            }

            raw.type = SELL;
            raw.count = list.at(7).toInt();
            raw.price = list.at(8).toDouble();
            raw.fee = list.at(9).toDouble() + list.at(10).toDouble();
        }


        rawData.push_back(raw);


        sSTOCKDATA tastyData;
        tastyData.dateTime = raw.dateTime;
        tastyData.type = raw.type;
        tastyData.ticker = raw.ticker;
        tastyData.currency = USD;
        tastyData.count = raw.count;
        tastyData.price = raw.price;
        tastyData.source = TASTYWORKS;
    }
}

bool Tastyworks::getIsRAWFile() const
{
    return isRAWFile;
}

QVector<sTASTYWORKSRAW> Tastyworks::getRawData() const
{
    return rawData;
}

bool Tastyworks::loadRawData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + TASTYWORKSRAWFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> rawData;
            qFile.close();
            return true;
        }
    }

    return false;
}

void Tastyworks::saveRawData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + TASTYWORKSRAWFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << rawData;
        qFile.close();
    }
}


QDataStream &operator<<(QDataStream &out, const sTASTYWORKSRAW &param)
{
    out << param.dateTime;
    out << static_cast<int>(param.type);
    out << param.ticker;
    out << param.description;
    out << param.price;
    out << param.count;
    out << param.fee;

    return out;
}

QDataStream &operator>>(QDataStream &in, sTASTYWORKSRAW &param)
{
    in >> param.dateTime;

    int buffer;
    in >> buffer;
    param.type = static_cast<eSTOCKEVENTTYPE>(buffer);

    in >> param.ticker;
    in >> param.description;
    in >> param.price;
    in >> param.count;
    in >> param.fee;

    return in;
}
