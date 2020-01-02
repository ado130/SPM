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
    DegiroDataType getDegiroData() const;

    bool getIsRAWFile() const;

    double getTax(QString ticker, QDateTime date, eDEGIROTYPE type);
public slots:

private:
    QVector<sDEGIRORAW> degiroRawData;
    DegiroDataType degiroData;
    bool isRAWFile;

    bool loadDegiroRaw();
    void saveDegiroRaw();
    bool loadDegiro();
    void saveDegiro();
    QStringList parseLine(QString line, char delimeter);

};

QDataStream& operator<<(QDataStream& out, const sDEGIRORAW& param);
QDataStream& operator>>(QDataStream& in, sDEGIRORAW& param);

QDataStream& operator<<(QDataStream& out, const sDEGIRODATA& param);
QDataStream& operator>>(QDataStream& in, sDEGIRODATA& param);

#endif // DEGIRO_H
