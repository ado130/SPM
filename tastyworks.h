#ifndef TASTYWORKS_H
#define TASTYWORKS_H

#include <QObject>
#include "global.h"

class Tastyworks : public QObject
{
    Q_OBJECT
public:
    explicit Tastyworks(QObject *parent = nullptr);

    void loadCSV(QString path, eDELIMETER delimeter);

    bool getIsRAWFile() const;

    QVector<sTASTYWORKSRAW> getRawData() const;

signals:

private:
    QVector<sTASTYWORKSRAW> rawData;
    bool isRAWFile;

    bool loadRawData();
    void saveRawData();
};

QDataStream &operator<<(QDataStream &out, const sTASTYWORKSRAW &param);
QDataStream &operator>>(QDataStream &in, sTASTYWORKSRAW &param);

#endif // TASTYWORKS_H
