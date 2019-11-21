#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QDataStream>
#include <QMap>
#include <QVector>

#include "global.h"



class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);

    QString getDegiroCSV() const;
    void setDegiroCSV(const QString &value);

    int getLastScreenerIndex() const;
    void setLastScreenerIndex(const int &value);

    sSETTINGS getSetting() const;

    QList<sDEGIRORAW> getDegiroData() const;
    void setDegiroData(const QList<sDEGIRORAW> &value);

    QString getCurrencyText(eCURRENCY currency);

    QList<sSCREENERPARAM> getScreenerParams() const;
    void setScreenerParams(const QList<sSCREENERPARAM> &value);

    QStringList getEnabledScreenerParams() const;

    QVector<sFILTER> getFilterList() const;
    void setFilterList(const QVector<sFILTER> &value);

signals:

public slots:
    void setSetting(const sSETTINGS &value);

private:
    sSETTINGS setting;
    QStringList enabledScreenerParams;
    QVector<sFILTER> filterList;

    void loadConfig();
    void saveConfig();

    void loadScreenParams();
    void saveScreenerParams();

    void setEnabledScreenerParams();

    void loadFilterList();
    void saveFilterList();
};

QDataStream& operator<<(QDataStream& out, const sSCREENERPARAM& param);
QDataStream& operator>>(QDataStream& in, sSCREENERPARAM& param);

QDataStream& operator<<(QDataStream& out, const sFILTER& param);
QDataStream& operator>>(QDataStream& in, sFILTER& param);

#endif // DATABASE_H
