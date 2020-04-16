#ifndef CALCULATION_H
#define CALCULATION_H

#include <QObject>
#include <QtCharts>

#include "callout.h"
#include "database.h"
#include "stockdata.h"

class Calculation : public QObject
{
    Q_OBJECT
public:
    explicit Calculation(Database *db, StockData *sd, QObject *parent = nullptr);

    QVector<sOVERVIEWTABLE> getOverviewTable(const QDate &from, const QDate &to);
    double getPortfolioValue(const QDate &from, const QDate &to);
    sOVERVIEWINFO getOverviewInfo(const QDate &from, const QDate &to);
    QChartView *getChartView(const eCHARTTYPE &type, const QDate &from, const QDate &to, const QString &ISIN = QString());
private:
    Database *database;
    StockData *stockData;

    QChart *getChart(const eCHARTTYPE &type, const QDate &from, const QDate &to, const QString &ISIN = QString());
    QLineSeries *getInvestedSeries(const QDate &from, const QDate &to);
    QLineSeries *getDepositSeries(const QDate &from, const QDate &to);
    QBarSeries *getDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis, const QString &ISIN = QString());
    QStackedBarSeries *getMonthDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis);
    QStackedBarSeries *getYearDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis);
    QPieSeries *getSectorSeries(const QDate &from, const QDate &to);
    QPieSeries *getStockSeries(const QDate &from, const QDate &to);

signals:
};



#endif // CALCULATION_H
