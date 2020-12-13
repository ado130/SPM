#ifndef DEGIRO_H
#define DEGIRO_H

#include <QObject>

#include "global.h"

class DeGiro : public QObject
{
    Q_OBJECT
public:
    explicit DeGiro(sSETTINGS set, QObject *parent = nullptr);

    void loadCSV(QString path, eDELIMETER delimeter);

    QVector<sDEGIRORAW> getRawData() const;

    bool getIsRAWFile() const;

signals:
    void setDegiroData(StockDataType data);

private:
    QVector<sDEGIRORAW> rawData;
    bool isRAWFileLoaded;
    sSETTINGS settings;

    bool loadRawData();
    void saveRawData();
    QStringList parseLine(QString line, char delimeter);

    StockDataType mergeFeeWithEvent(StockDataType &data);
};

QDataStream& operator<<(QDataStream& out, const sDEGIRORAW& param);
QDataStream& operator>>(QDataStream& in, sDEGIRORAW& param);


#endif // DEGIRO_H
