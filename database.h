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

    QString getCurrencyText(eCURRENCY currency);
    QString getCurrencySign(eCURRENCY currency);

    QVector<sSCREENERPARAM> getScreenerParams() const;
    void setScreenerParams(const QVector<sSCREENERPARAM> &value);

    QStringList getEnabledScreenerParams() const;

    QVector<sFILTER> getFilterList() const;
    void setFilterList(const QVector<sFILTER> &value);

    QVector<sISINDATA> getIsinList() const;
    void setIsinList(const QVector<sISINDATA> &value);

    double getExchangePrice(const eCURRENCY &rates, const double &price);
    ExchangeRatesFunctions getExchangeRatesFuncMap() const;

signals:

public slots:
    void setSettingSlot(const sSETTINGS &value);

private:
    sSETTINGS setting;
    QStringList enabledScreenerParams;
    QVector<sFILTER> filterList;
    QVector<sISINDATA> isinList;

    ExchangeRatesFunctions exchangeRatesFuncMap;

    void loadConfig();
    void saveConfig();

    void loadScreenParams();
    void saveScreenerParams();

    void setEnabledScreenerParams();

    void loadFilterList();
    void saveFilterList();

    bool loadIsinData();
    void saveIsinData();
};

QDataStream& operator<<(QDataStream& out, const sSCREENERPARAM& param);
QDataStream& operator>>(QDataStream& in, sSCREENERPARAM& param);

QDataStream& operator<<(QDataStream& out, const sFILTER& param);
QDataStream& operator>>(QDataStream& in, sFILTER& param);

QDataStream& operator<<(QDataStream& out, const sISINDATA& param);
QDataStream& operator>>(QDataStream& in, sISINDATA& param);

#endif // DATABASE_H
