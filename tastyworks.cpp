#include "tastyworks.h"

#include <QFile>
#include <QDebug>

Tastyworks::Tastyworks(QObject *parent) : QObject(parent)
{

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
        QStringList list = line.split(chDelimeter, QString::KeepEmptyParts);

        if(!isFirstLine)
        {
            isFirstLine = true;
            continue;
        }


    }
}
