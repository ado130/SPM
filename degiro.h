#ifndef DEGIRO_H
#define DEGIRO_H

#include <QObject>

#include "global.h"

class DeGiro : public QObject
{
    Q_OBJECT
public:
    explicit DeGiro(QObject *parent = nullptr);

    void loadDegiroCSV(QString path, eDELIMETER delimeter);

    QVector<sDEGIRORAW> getDegiroRawData() const;
    StockDataType getStockData() const;

    bool getIsRAWFile() const;

signals:
    void setDegiroData(StockDataType data);

private:
    QVector<sDEGIRORAW> degiroRawData;
    bool isRAWFile;

    bool loadDegiroRaw();
    void saveDegiroRaw();
    QStringList parseLine(QString line, char delimeter);

};

QDataStream& operator<<(QDataStream& out, const sDEGIRORAW& param);
QDataStream& operator>>(QDataStream& in, sDEGIRORAW& param);


#endif // DEGIRO_H
