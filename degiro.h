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
    void setDegiroRawData(const QVector<sDEGIRORAW> &value);

    bool getIsRAWFile() const;

    signals:

public slots:

private:
    QVector<sDEGIRORAW> degiroRawData;
    QVector<sDEGIRO> degiroData;
    bool isRAWFile;


    bool loadDegiroRaw();
    void saveDegiroRaw();
    QStringList parseLine(QString line, char delimeter);
};

QDataStream& operator<<(QDataStream& out, const sDEGIRORAW& param);
QDataStream& operator>>(QDataStream& in, sDEGIRORAW& param);

#endif // DEGIRO_H
