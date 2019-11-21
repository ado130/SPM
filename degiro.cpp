#include "degiro.h"

#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>

DeGiro::DeGiro(QObject *parent) : QObject(parent)
{
    isRAWFile = loadDegiroRaw();
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

    QList<QStringList> wordList;

    while (!file.atEnd())
    {
        QString line = file.readLine();
        QStringList list = parseLine(line, chDelimeter);
        wordList.append(list);

        if(wordList.count() == 1) continue;  // first line

        QDateTime dt;
        QDate d = QDate::fromString(list.at(0), "dd-MM-yyyy");

        QTime t = QTime::fromString(list.at(1), "hh:mm");

        dt.setDate(d);
        dt.setTime(t);

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
        else if(tmp.description.contains("výběr") || tmp.description.contains("withdrawal"))
        {
            degData.type = WITHDRAWAL;
        }
        else if(tmp.description.contains("poplatek") || tmp.description.contains("fee"))
        {
            degData.type = FEE;
        }
        else if(tmp.description.contains("dividend"))
        {
            degData.type = DIVIDEND;
        }
        else if(tmp.description.contains("nákup") || tmp.description.contains("buy"))
        {
            degData.type = BUY;
        }
        else if(tmp.description.contains("prodej") || tmp.description.contains("sell"))
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
        }
        sDEGIRO deg;
        deg.data = degData;
        deg.ticker = degData.product;

        degiroRawData.append(tmp);
    }

    saveDegiroRaw();
    isRAWFile = true;
}

QVector<sDEGIRORAW> DeGiro::getDegiroRawData() const
{
    return degiroRawData;
}

void DeGiro::setDegiroRawData(const QVector<sDEGIRORAW> &value)
{
    degiroRawData = value;
}

bool DeGiro::getIsRAWFile() const
{
    return isRAWFile;
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

