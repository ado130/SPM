#include "degiro.h"

#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>

DeGiro::DeGiro(QObject *parent) : QObject(parent)
{
    isRAWFile = loadRawData();
}

void DeGiro::loadCSV(QString path, eDELIMETER delimeter)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
        return;
    }

    StockDataType stockData;
    degiroRawData.clear();

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
        QStringList list = parseLine(line, chDelimeter);

        if(!isFirstLine)
        {
            isFirstLine = true;
            continue;
        }

        QDate d = QDate::fromString(list.at(0), "dd-MM-yyyy");
        QTime t = QTime::fromString(list.at(1), "hh:mm");

        QDateTime dt(d, t);

        sDEGIRORAW degiroRaw;
        degiroRaw.dateTime = dt;
        degiroRaw.product = list.at(3);
        degiroRaw.ISIN = list.at(4);
        degiroRaw.description = list.at(5);

        if(list.at(7).contains("CZK"))
        {
            degiroRaw.currency = CZK;
        }
        else if(list.at(7).contains("USD"))
        {
            degiroRaw.currency = USD;
        }
        else if(list.at(7).contains("EUR"))
        {
            degiroRaw.currency = EUR;
        }
        else if(list.at(9).contains("CZK"))
        {
            degiroRaw.currency = CZK;
        }
        else if(list.at(9).contains("USD"))
        {
            degiroRaw.currency = USD;
        }
        else if(list.at(9).contains("EUR"))
        {
            degiroRaw.currency = EUR;
        }

        bool ok;
        QString val = list.at(8);
        val.replace(",", ".");
        degiroRaw.price = val.toDouble(&ok);

        if(!ok)
        {
            val = list.at(10);
            val.replace(",", ".");
            degiroRaw.price = val.toDouble(&ok);

            if(!ok)
            {
                degiroRaw.price = 0.0;
            }
        }


        degiroRawData.push_back(degiroRaw);


        sSTOCKDATA degData;
        degData.count = 0;

        bool found = true;

        if(degiroRaw.description.toLower().contains("vklad") || degiroRaw.description.toLower().contains("deposit"))
        {
            degData.type = DEPOSIT;
        }
        else if(degiroRaw.description.toLower().contains("výběr") || degiroRaw.description.toLower().contains("withdrawal"))
        {
            degData.type = WITHDRAWAL;
        }
        else if(degiroRaw.description.toLower().contains("transakční poplatek") || degiroRaw.description.toLower().contains("transaction fee"))
        {
            degData.type = TRANSACTIONFEE;
        }
        else if(degiroRaw.description.toLower().contains("poplatek") || degiroRaw.description.toLower().contains("fee"))
        {
            degData.type = FEE;
        }
        else if(degiroRaw.description.toLower().contains("daň") || degiroRaw.description.toLower().contains("tax"))
        {
            degData.type = TAX;
        }
        else if(degiroRaw.description.toLower().contains("dividend"))
        {
            degData.type = DIVIDEND;
        }
        else if(degiroRaw.description.toLower().contains("nákup") || degiroRaw.description.toLower().contains("buy"))
        {
            degData.type = BUY;

            int start = degiroRaw.description.indexOf(" ");
            int end = degiroRaw.description.indexOf(" ", start+1);

            degData.count = degiroRaw.description.mid(start + 1, end-start-1).toInt();
        }
        else if(degiroRaw.description.toLower().contains("prodej") || degiroRaw.description.toLower().contains("sell"))
        {
            degData.type = SELL;

            int start = degiroRaw.description.indexOf(" ");
            int end = degiroRaw.description.indexOf(" ", start + 1);

            degData.count = degiroRaw.description.mid(start + 1, end-start-1).toInt();
        }
        else
        {
            found = false;
        }

        if(found)
        {
            degData.dateTime = degiroRaw.dateTime;
            degData.ISIN = degiroRaw.ISIN;
            degData.stockName = degiroRaw.product;

            if(degData.count != 0)
            {
                degData.price = degiroRaw.price/degData.count;
            }
            else
            {
                degData.price = degiroRaw.price;
            }

            degData.currency = degiroRaw.currency;
            degData.isDegiroSource = true;

            QVector<sSTOCKDATA> vector = stockData[degiroRaw.ISIN];

            // check for multiple dividends/fees for the ISIN at the same time and sum them
            // it might happen that degiro add and sub the dividend/fee
            if(degData.type == DIVIDEND || degData.type == TAX)
            {
                auto exists = std::find_if(vector.begin(), vector.end(), [degData] (sSTOCKDATA &s)
                                           {
                                               return ( (s.dateTime.date() == degData.dateTime.date()) && (s.type == degData.type) );
                                           }
                                           );

                if(exists != vector.end())
                {
                    exists->price += degData.price;
                    exists->count += degData.count;
                }
                else
                {
                    vector.append(degData);
                }
            }
            else
            {
                vector.append(degData);
            }

            stockData[degiroRaw.ISIN] = vector;
        }
    }

    if(degiroRawData.count() > 0)
    {
        emit setDegiroData(stockData);
        saveRawData();
        isRAWFile = true;
    }
}

QStringList DeGiro::parseLine(QString line, char delimeter)
{
    QStringList list;

    int tmp = 0;
    int index = line.indexOf(delimeter, tmp);
    list << line.mid(tmp, index-tmp);

    do
    {
        if(line.at(index+1) == ("\""))
        {
            tmp = index + 2;
            index = line.indexOf("\"", tmp);
            list << line.mid(tmp, index-tmp);
            index += 1;
        }
        else
        {
            tmp = index + 1;
            index = line.indexOf(delimeter, tmp);
            list << line.mid(tmp, index-tmp);
        }
    }while(list.count() != 12);

    return list;
}


QVector<sDEGIRORAW> DeGiro::getRawData() const
{
    return degiroRawData;
}

bool DeGiro::loadRawData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + DEGIRORAWFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> degiroRawData;
            qFile.close();
            return true;
        }
    }

    return false;
}

void DeGiro::saveRawData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + DEGIRORAWFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << degiroRawData;
        qFile.close();
    }
}

bool DeGiro::getIsRAWFile() const
{
    return isRAWFile;
}


QDataStream &operator<<(QDataStream &out, const sDEGIRORAW &param)
{
    out << param.dateTime;
    out << param.product;
    out << param.ISIN;
    out << param.description;
    out << static_cast<int>(param.currency);
    out << param.price;

    return out;
}

QDataStream &operator>>(QDataStream &in, sDEGIRORAW &param)
{
    in >> param.dateTime;
    in >> param.product;
    in >> param.ISIN;
    in >> param.description;

    int buffer;
    in >> buffer;
    param.currency = static_cast<eCURRENCY>(buffer);

    in >> param.price;

    return in;
}
