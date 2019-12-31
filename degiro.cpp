#include "degiro.h"

#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>

DeGiro::DeGiro(QObject *parent) : QObject(parent)
{
    isRAWFile = (loadDegiro() & loadDegiroRaw());
}

void DeGiro::loadDegiroCSV(QString path, eDELIMETER delimeter)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
        return;
    }

    degiroRawData.clear();
    degiroData.clear();

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

        sDEGIRORAW tmp;
        tmp.dateTime = dt;
        tmp.product = list.at(3);
        tmp.ISIN = list.at(4);
        tmp.description = list.at(5);

        if(list.at(7).contains("CZK"))
        {
            tmp.currency = CZK;
        }
        else if(list.at(7).contains("USD"))
        {
            tmp.currency = USD;
        }
        else if(list.at(7).contains("EUR"))
        {
            tmp.currency = EUR;
        }
        else if(list.at(9).contains("CZK"))
        {
            tmp.currency = CZK;
        }
        else if(list.at(9).contains("USD"))
        {
            tmp.currency = USD;
        }
        else if(list.at(9).contains("EUR"))
        {
            tmp.currency = EUR;
        }

        bool ok;
        QString val = list.at(8);
        val.replace(",", ".");
        tmp.money = val.toDouble(&ok);

        if(!ok)
        {
            val = list.at(10);
            val.replace(",", ".");
            tmp.money = val.toDouble(&ok);

            if(!ok)
            {
                tmp.money = 0.0;
            }
        }

        sDEGIRODATA degData;
        bool found = true;

        if(tmp.description.toLower().contains("vklad") || tmp.description.toLower().contains("deposit"))
        {
            degData.type = DEPOSIT;
        }
        else if(tmp.description.toLower().contains("výběr") || tmp.description.toLower().contains("withdrawal"))
        {
            degData.type = WITHDRAWAL;
        }
        else if(tmp.description.toLower().contains("poplatek") || tmp.description.toLower().contains("fee"))
        {
            degData.type = FEE;
        }
        else if(tmp.description.toLower().contains("daň") || tmp.description.toLower().contains("tax"))
        {
            degData.type = TAX;
        }
        else if(tmp.description.toLower().contains("dividend"))
        {
            degData.type = DIVIDEND;
        }
        else if(tmp.description.toLower().contains("nákup") || tmp.description.toLower().contains("buy"))
        {
            degData.type = BUY;
        }
        else if(tmp.description.toLower().contains("prodej") || tmp.description.toLower().contains("sell"))
        {
            degData.type = SELL;
        }
        else
        {
            found = false;
        }

        if(found)
        {
            degData.dateTime = tmp.dateTime;
            degData.product = tmp.product;
            degData.ISIN = tmp.ISIN;
            degData.money = tmp.money;
            degData.currency = tmp.currency;

            sDEGIRO deg;
            deg.data = degData;
            deg.ticker = degData.product;

            degiroRawData.push_back(tmp);
            degiroData.push_front(deg);
        }
    }

    if(degiroRawData.count() > 0)
    {
        saveDegiro();
        saveDegiroRaw();
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

QVector<sDEGIRORAW> DeGiro::getDegiroRawData() const
{
    return degiroRawData;
}

bool DeGiro::loadDegiroRaw()
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

void DeGiro::saveDegiroRaw()
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

QVector<sDEGIRO> DeGiro::getDegiroData() const
{
    return degiroData;
}

bool DeGiro::loadDegiro()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + DEGIROFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> degiroData;
            qFile.close();
            return true;
        }
    }

    return false;
}

void DeGiro::saveDegiro()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + DEGIROFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << degiroData;
        qFile.close();
    }
}

QDataStream &operator<<(QDataStream &out, const sDEGIRORAW &param)
{
    out << param.dateTime;
    out << param.product;
    out << param.ISIN;
    out << param.description;
    out << static_cast<int>(param.currency);
    out << param.money;

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

    in >> param.money;

    return in;
}

QDataStream &operator<<(QDataStream &out, const sDEGIRO &param)
{
    out << param.ticker;
    out << param.data.dateTime;
    out << param.data.product;
    out << param.data.ISIN;
    out << static_cast<int>(param.data.type);
    out << static_cast<int>(param.data.currency);
    out << param.data.money;

    return out;
}

QDataStream &operator>>(QDataStream &in, sDEGIRO &param)
{
    in >> param.ticker;
    in >> param.data.dateTime;
    in >> param.data.product;
    in >> param.data.ISIN;

    int buffer1;
    in >> buffer1;
    param.data.type = static_cast<eDEGIROTYPE>(buffer1);

    int buffer2;
    in >> buffer2;
    param.data.currency = static_cast<eCURRENCY>(buffer2);

    in >> param.data.money;

    return in;
}

