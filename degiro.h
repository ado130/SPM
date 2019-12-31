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
    QVector<sDEGIRO> getDegiroData() const;

    bool getIsRAWFile() const;

public slots:

private:
    QVector<sDEGIRORAW> degiroRawData;
    QVector<sDEGIRO> degiroData;
    bool isRAWFile;

    bool loadDegiroRaw();
    void saveDegiroRaw();
    bool loadDegiro();
    void saveDegiro();
    QStringList parseLine(QString line, char delimeter);

};

QDataStream& operator<<(QDataStream& out, const sDEGIRORAW& param);
QDataStream& operator>>(QDataStream& in, sDEGIRORAW& param);

QDataStream& operator<<(QDataStream& out, const sDEGIRO& param);
QDataStream& operator>>(QDataStream& in, sDEGIRO& param);

#endif // DEGIRO_H
