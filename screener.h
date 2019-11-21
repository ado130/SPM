#ifndef SCREENER_H
#define SCREENER_H

#include <QObject>

#include "global.h"

class Screener : public QObject
{
    Q_OBJECT
public:
    explicit Screener(QObject *parent = nullptr);

    sTABLE yahooParse(QString data);
    sTABLE finvizParse(QString data);

    QVector<sSCREENER> getAllScreenerData() const;
    void setAllScreenerData(const QVector<sSCREENER> &value);

signals:

public slots:

private:
    QVector<sSCREENER> allScreenerData;

    void saveAllScreenerData();
    void loadAllScreenerData();
};

QDataStream& operator<<(QDataStream& out, const sSCREENER& param);
QDataStream& operator>>(QDataStream& in, sSCREENER& param);

#endif // SCREENER_H
