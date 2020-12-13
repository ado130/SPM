#include "calculation.h"

Calculation::Calculation(Database *db, StockData *sd, QObject *parent) : QObject(parent), database(db), stockData(sd)
{

}

double Calculation::getPortfolioValue(const QDate &from, const QDate &to)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    QList<QString> keys = stockList.keys();

    QString rates;
    switch(database->getSetting().currency)
    {
        case USD: rates = "USD2USD";
            break;
        case CZK: rates = "USD2CZK";
            break;
        case EUR: rates = "USD2EUR";
            break;
        case GBP: rates = "USD2GBP";
            break;
    }

    double portfolioValue = 0.0;

    for(const QString &key : keys)
    {
        if( key.isEmpty() || stockList.value(key).count() == 0 ) continue;

        sSTOCKDATA stock = stockList.value(key).first();

        if(stock.stockName.toLower().contains("fundshare") ) continue;

        int totalCount = stockData->getTotalCount(stock.ISIN, from, to);

        if(totalCount <= 0 && !database->getSetting().showSoldPositions) continue;


        QString cachedPrice = stockData->getCachedISINParam(stock.ISIN, "Price");

        if(cachedPrice.isEmpty())
        {
            // ToDo the return price might be in EUR or USD or whatever
            cachedPrice = stockData->getCachedISINParam(stock.ISIN, "Previous Close");
        }

        double onlineStockPrice = 0.0;
        double totalOnlinePrice = 0.0;

        if(!cachedPrice.isEmpty())
        {
            onlineStockPrice = database->getExchangePrice(rates, cachedPrice.toDouble());
            totalOnlinePrice = onlineStockPrice*totalCount;
        }


        portfolioValue += (totalOnlinePrice);
    }

    return portfolioValue;
}

sOVERVIEWINFO Calculation::getOverviewInfo(const QDate &from, const QDate &to)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return sOVERVIEWINFO();
    }

    double deposit = 0.0;
    double withdrawal = 0.0;
    double invested = 0.0;
    double transFees = 0.0;
    double sell = 0.0;
    double dividends = 0.0;
    double divTax = 0.0;
    double fees = 0.0;

    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    double balance = 0.0;
    QDateTime lastDate = stockList.values().first().first().dateTime;

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if(stock.currency == EUR && stock.dateTime >= lastDate)
            {
                balance = stock.balance;
                lastDate = stock.dateTime;
            }

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            QString rates;
            eCURRENCY currencyFrom = stock.currency;

            switch(currencyFrom)
            {
                case USD: rates = "USD";
                    break;
                case CZK: rates = "CZK";
                    break;
                case EUR: rates = "EUR";
                    break;
                case GBP: rates = "GBP";
                    break;
            }

            rates += "2";

            switch(selectedCurrency)
            {
                case USD: rates += "USD";
                    break;
                case CZK: rates += "CZK";
                    break;
                case EUR: rates += "EUR";
                    break;
                case GBP: rates += "GBP";
                    break;
            }

            switch(stock.type)
            {
                case DEPOSIT:
                {
                    deposit += database->getExchangePrice(rates, stock.price);
                }
                break;

                case WITHDRAWAL:
                {
                    withdrawal += database->getExchangePrice(rates, stock.price);
                }
                break;

                case BUY:
                {
                    invested += database->getExchangePrice(rates, stock.price) * stock.count;
                    transFees += database->getExchangePrice(rates, stock.fee);
                }
                break;

                case SELL:
                {
                    sell += database->getExchangePrice(rates, stock.price) * stock.count;
                    transFees += database->getExchangePrice(rates, stock.fee);
                }
                break;

                case DIVIDEND:
                {
                    dividends += database->getExchangePrice(rates, stock.price);
                    divTax += database->getExchangePrice(rates, stock.fee);
                }
                break;

                case FEE:
                {
                    fees += database->getExchangePrice(rates, stock.price);
                }
                break;
            }
        }
    }

    sOVERVIEWINFO info;
    info.deposit = deposit;
    info.withdrawal = withdrawal;
    info.invested = invested;
    info.transFees = transFees;
    info.sell = sell;
    info.dividends = dividends;
    info.divTax = divTax;
    info.fees = fees;


    deposit = abs(deposit);
    withdrawal = abs(withdrawal);
    invested = abs(invested);
    transFees = abs(transFees);
    sell = abs(sell);
    dividends = abs(dividends);
    divTax = abs(divTax);
    fees = abs(fees);

    if(!qFuzzyIsNull(invested-sell))
    {
        info.DY = ((dividends-divTax)/(invested-sell))*100.0;
    }

    //info.account = (deposit + sell + dividends - divTax - invested - fees - transFees - withdrawal);
    info.account = balance;
    info.portfolio = getPortfolioValue(from, to);

    if(!qFuzzyIsNull(deposit))
    {
        info.performance = ((info.portfolio+dividends-divTax-fees-transFees)/deposit)*100.0;
    }
    else
    {
        info.performance = 0.0;
    }

    return info;
}

QVector<sOVERVIEWTABLE> Calculation::getOverviewTable(const QDate &from, const QDate &to)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    QVector<sOVERVIEWTABLE> table;

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return table;
    }

    QList<QString> keys = stockList.keys();
    std::sort(keys.begin(), keys.end(),
              [](QString a, QString b)
              {
                  return a < b;
              }
              );

    QString rates;
    switch(database->getSetting().currency)
    {
        case USD: rates = "USD2USD";
            break;
        case CZK: rates = "USD2CZK";
            break;
        case EUR: rates = "USD2EUR";
            break;
        case GBP: rates = "USD2GBP";
            break;
    }

    for(const QString &key : keys)
    {
        if( key.isEmpty() || stockList.value(key).count() == 0 ) continue;

        sSTOCKDATA stock = stockList.value(key).first();

        if(stock.stockName.toLower().contains("fundshare") ) continue;

        sOVERVIEWTABLE row;

        // Check the total count
        int totalCount = stockData->getTotalCount(stock.ISIN, from, to);

        if(totalCount <= 0 && !database->getSetting().showSoldPositions) continue;

        row.totalCount = totalCount;

        row.ISIN = stock.ISIN;
        row.ticker = stock.ticker;
        row.stockName = stock.stockName;

        // Find sector
        const QVector<sISINDATA> isinList = database->getIsinList();

        auto it = std::find_if(isinList.begin(), isinList.end(), [stock](sISINDATA a)
                               {
                                   return stock.ISIN == a.ISIN;
                               }
                               );

        if(it != isinList.end())
        {
            row.sector = it->sector;
        }

        // Get cached price and calculate total online price
        QString cachedPrice = stockData->getCachedISINParam(stock.ISIN, "Price");

        if(cachedPrice.isEmpty())
        {
            // ToDo the return price might be in EUR or USD or whatever
            cachedPrice = stockData->getCachedISINParam(stock.ISIN, "Previous Close");
        }

        if(!cachedPrice.isEmpty())
        {
            row.onlineStockPrice = database->getExchangePrice(rates, cachedPrice.toDouble());
            row.totalOnlinePrice = row.onlineStockPrice*totalCount;
        }
        else
        {
            row.onlineStockPrice = 0.0;
            row.totalOnlinePrice = 0.0;
        }

        row.totalStockPrice = stockData->getTotalPrice(stock.ISIN, from, to, database->getSetting().currency, database->getExchangeRatesFuncMap());
        row.averageBuyPrice = row.totalStockPrice/totalCount;

        row.totalFee = stockData->getTotalFee(stock.ISIN, from, to, database->getSetting().currency, database->getExchangeRatesFuncMap());
        row.dividend = stockData->getReceivedDividend(stock.ISIN, from, to, database->getSetting().currency, database->getExchangeRatesFuncMap());


        table.push_back(row);
    }

    double portfolioValue = getPortfolioValue(from, to);

    QMutableVectorIterator it(table);

    while (it.hasNext())
    {
        it.next();

        double percentage = (it.value().totalOnlinePrice/portfolioValue)*100.0;
        it.value().percentage = percentage;
    }

    return table;
}

/********************************
*
*  CHARTS
*
********************************/
QLineSeries* Calculation::getDepositSeries(const QDate &from, const QDate &to)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }

    double deposit = 0.0;
    QLineSeries *depositSeries = new QLineSeries();
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            if(stock.type == DEPOSIT)
            {
                QString rates;
                eCURRENCY currencyFrom = stock.currency;

                switch(currencyFrom)
                {
                    case USD: rates = "USD";
                        break;
                    case CZK: rates = "CZK";
                        break;
                    case EUR: rates = "EUR";
                        break;
                    case GBP: rates = "GBP";
                        break;
                }

                rates += "2";

                switch(selectedCurrency)
                {
                    case USD: rates += "USD";
                        break;
                    case CZK: rates += "CZK";
                        break;
                    case EUR: rates += "EUR";
                        break;
                    case GBP: rates += "GBP";
                        break;
                }

                deposit += database->getExchangePrice(rates, stock.price);

                depositSeries->append(stock.dateTime.toMSecsSinceEpoch(), deposit);
            }
        }
    }

    if(depositSeries->pointsVector().count() == 0)
    {
        return nullptr;
    }

    // Sort the dates
    QVector<QPointF> points = depositSeries->pointsVector();
    QVector<qreal> xPoints;

    for(int a = 0; a<points.count(); ++a)
    {
        xPoints.append(points.at(a).x());
    }

    std::sort(xPoints.begin(), xPoints.end());

    for(int a = 0; a<points.count(); ++a)
    {
        depositSeries->replace(points.at(a).x(), points.at(a).y(), xPoints.at(a), points.at(a).y());
    }

    if(depositSeries->pointsVector().count() == 1)
    {
        depositSeries->append(QDateTime(from, QTime(0, 0, 0)).toMSecsSinceEpoch(), 0);
    }

    return depositSeries;
}

QLineSeries* Calculation::getInvestedSeries(const QDate &from, const QDate &to)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }

    double invested = 0.0;
    QLineSeries *investedSeries = new QLineSeries();
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            if(stock.type == BUY)
            {
                QString rates;
                eCURRENCY currencyFrom = stock.currency;

                switch(currencyFrom)
                {
                    case USD: rates = "USD";
                        break;
                    case CZK: rates = "CZK";
                        break;
                    case EUR: rates = "EUR";
                        break;
                    case GBP: rates = "GBP";
                        break;
                }

                rates += "2";

                switch(selectedCurrency)
                {
                    case USD: rates += "USD";
                        break;
                    case CZK: rates += "CZK";
                        break;
                    case EUR: rates += "EUR";
                        break;
                    case GBP: rates += "GBP";
                        break;
                }


                invested += database->getExchangePrice(rates, (-1.0)*stock.price) * stock.count;

                investedSeries->append(stock.dateTime.toMSecsSinceEpoch(), invested);
            }
        }
    }

    if(investedSeries->pointsVector().count() == 0)
    {
        return nullptr;
    }

    // Sort the dates
    QVector<QPointF> points = investedSeries->pointsVector();
    QVector<qreal> xPoints;

    for(int a = 0; a<points.count(); ++a)
    {
        xPoints.append(points.at(a).x());
    }

    std::sort(xPoints.begin(), xPoints.end());

    for(int a = 0; a<points.count(); ++a)
    {
        investedSeries->replace(points.at(a).x(), points.at(a).y(), xPoints.at(a), points.at(a).y());
    }

    if(investedSeries->pointsVector().count() == 1)
    {
        investedSeries->append(QDateTime(QDate(QDate::currentDate().year(), 1, 1), QTime(0, 0, 0)).toMSecsSinceEpoch(), 0);
    }

    return investedSeries;
}

QBarSeries* Calculation::getDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis, const QString &ISIN)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }

    QHash<QString, QVector<QPair<QDate, double>> > dividends;
    double maxDividendAxis = 0.0;
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    if(!ISIN.isEmpty())
    {
        keys.clear();
        keys << ISIN;
    }

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            if(stock.type == DIVIDEND)
            {
                QString rates;
                eCURRENCY currencyFrom = stock.currency;

                switch(currencyFrom)
                {
                    case USD: rates = "USD";
                        break;
                    case CZK: rates = "CZK";
                        break;
                    case EUR: rates = "EUR";
                        break;
                    case GBP: rates = "GBP";
                        break;
                }

                rates += "2";

                switch(selectedCurrency)
                {
                    case USD: rates += "USD";
                        break;
                    case CZK: rates += "CZK";
                        break;
                    case EUR: rates += "EUR";
                        break;
                    case GBP: rates += "GBP";
                        break;
                }

                double price = 0.0;

                price = database->getExchangePrice(rates, stock.price);

                if(price > maxDividendAxis)
                {
                    maxDividendAxis = price;
                }

                QString ticker = stock.ticker;
                QDate date = stock.dateTime.date();

                QVector<QPair<QDate, double>> vector = dividends.value(ticker);

                if(ISIN.isEmpty())  // we are in DIVIDENDCHART mode, just add new record to the vector
                {
                    vector.push_back(qMakePair(date, price));
                }
                else    // We are in the ISINCHART mode, sum the dividends within a year
                {
                    auto it = std::find_if(vector.begin(), vector.end(), [date](QPair<QDate, double> a)
                                           {
                                               return date.year() == a.first.year();
                                           }
                                           );

                    if(it != vector.end())  // we need to sum
                    {
                        it->second += price;

                        if(it->second > maxDividendAxis)
                        {
                            maxDividendAxis = it->second;
                        }
                    }
                    else                    // create new record, because we have found new year
                    {
                        vector.push_back(qMakePair(date, price));
                    }
                }

                dividends.insert(ticker, vector);
            }
        }
    }


    // Sort from min to max and find the min and max
    QDate min;
    QDate max;

    QList<QString> divKeys = dividends.keys();

    if(divKeys.count() == 0)
    {
        return nullptr;
    }

    min = dividends.value(divKeys.first()).first().first;
    max = dividends.value(divKeys.first()).first().first;

    for (const QString &key : divKeys)
    {
        auto vector = dividends.value(key);

        std::sort(vector.begin(), vector.end(),
                  [] (QPair<QDate, double> &a, QPair<QDate, double> &b)
                  {
                      return a.first < b.first;
                  }
                  );

        dividends[key] = vector;

        QDate localMin = vector.first().first;
        QDate localMax = vector.last().first;

        if(localMin < min)
        {
            min = localMin;
        }

        if(localMax > max)
        {
            max = localMax;
        }
    }


    // Save categories - find all months between min and max date
    QStringList categories;
    QDate tmpMin = min;
    QVector<QDate> dates;

    if(ISIN.isEmpty())
    {
        while(tmpMin <= max)
        {
            QLocale locale;
            QString month = locale.toString(tmpMin, "MMM") + " " + QString::number(tmpMin.year()-2000);
            month = month.left(1).toUpper() + month.mid(1);     // first char to upper

            categories << month;
            dates.push_back(tmpMin);

            tmpMin = tmpMin.addMonths(1);
        }
    }
    else
    {
        while(tmpMin.year() <= max.year())
        {
            QString year = tmpMin.toString("yyyy");

            categories << year;
            dates.push_back(tmpMin);

            tmpMin = tmpMin.addYears(1);
        }
    }


    // Fill empty places between dates
    QMutableHashIterator it(dividends);

    while(it.hasNext())
    {
        it.next();

        tmpMin = min;

        auto vector = it.value();

        for(const QDate &d : dates)
        {
            auto found = std::find_if(vector.begin(), vector.end(), [d] (QPair<QDate, double> &a)
                                      {
                                          //return d.month() == a.first.month();
                                          return d.month() == a.first.month() && d.year() == a.first.year();
                                      }
                                      );

            if(found == vector.end())
            {
                int index = dates.indexOf(d);
                vector.insert(index, qMakePair(d, 0.0));
            }
        }

        it.value() = vector;
    }


    // Sort from min to max and find the min and max
    divKeys = dividends.keys();

    if(divKeys.count() == 0)
    {
        return nullptr;
    }

    for (const QString &key : divKeys)
    {
        auto vector = dividends.value(key);

        std::sort(vector.begin(), vector.end(),
                  [] (QPair<QDate, double> &a, QPair<QDate, double> &b)
                  {
                      return a.first < b.first;
                  }
                  );

        dividends[key] = vector;
    }


    // Set all sets, ticker and date
    QVector<QBarSet*> dividendsSets;
    divKeys = dividends.keys();

    for (const QString &key : divKeys)
    {
        QBarSet *bar = new QBarSet(key);

        auto vector = dividends.value(key);

        for (const QPair<QDate, double> &v : vector)
        {
            bar->append(v.second);
        }

        dividendsSets.push_back(bar);
    }


    QBarSeries *dividendSeries = new QBarSeries();

    for(QBarSet *set : dividendsSets)
    {
        dividendSeries->append(set);
    }

    if(xAxis != nullptr)
    {
        *xAxis = categories;
    }

    if(maxYAxis != nullptr)
    {
        *maxYAxis = maxDividendAxis;
    }

    return dividendSeries;
}

QStackedBarSeries* Calculation::getMonthDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }

    QHash<QString, QVector<QPair<QDate, double>> > dividends;
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();


    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            if(stock.type == DIVIDEND)
            {
                QString rates;
                eCURRENCY currencyFrom = stock.currency;

                switch(currencyFrom)
                {
                    case USD: rates = "USD";
                        break;
                    case CZK: rates = "CZK";
                        break;
                    case EUR: rates = "EUR";
                        break;
                    case GBP: rates = "GBP";
                        break;
                }

                rates += "2";

                switch(selectedCurrency)
                {
                    case USD: rates += "USD";
                        break;
                    case CZK: rates += "CZK";
                        break;
                    case EUR: rates += "EUR";
                        break;
                    case GBP: rates += "GBP";
                        break;
                }

                double price = 0.0;

                price = database->getExchangePrice(rates, stock.price);

                QString ticker = stock.ticker;
                QDate date = stock.dateTime.date();

                QVector<QPair<QDate, double>> vector = dividends.value(ticker);

                vector.push_back(qMakePair(date, price));

                dividends.insert(ticker, vector);
            }
        }
    }

    // Sort from min to max and find the min and max
    QDate min;
    QDate max;

    QList<QString> divKeys = dividends.keys();

    if(divKeys.count() == 0)
    {
        return nullptr;
    }

    min = dividends.value(divKeys.first()).first().first;
    max = dividends.value(divKeys.first()).first().first;

    for (const QString &key : divKeys)
    {
        auto vector = dividends.value(key);

        std::sort(vector.begin(), vector.end(),
                  [] (QPair<QDate, double> &a, QPair<QDate, double> &b)
                  {
                      return a.first < b.first;
                  }
                  );

        dividends[key] = vector;

        QDate localMin = vector.first().first;
        QDate localMax = vector.last().first;

        if(localMin < min)
        {
            min = localMin;
        }

        if(localMax > max)
        {
            max = localMax;
        }
    }

    // Save categories - find all months between min and max date
    QStringList categories;
    QDate tmpMin = min;
    QVector<QDate> dates;

    while(tmpMin <= max)
    {
        QLocale locale;
        QString month = locale.toString(tmpMin, "MMM") + " " + QString::number(tmpMin.year()-2000);
        month = month.left(1).toUpper() + month.mid(1);     // first char to upper

        categories << month;
        dates.push_back(tmpMin);

        tmpMin = tmpMin.addMonths(1);
    }


    // Fill empty places between dates
    QMutableHashIterator it(dividends);

    while(it.hasNext())
    {
        it.next();

        tmpMin = min;

        auto vector = it.value();

        for(const QDate &d : dates)
        {
            auto found = std::find_if(vector.begin(), vector.end(), [d] (QPair<QDate, double> &a)
                                      {
                                          return d.month() == a.first.month() && d.year() == a.first.year();
                                      }
                                      );

            if(found == vector.end())
            {
                int index = dates.indexOf(d);
                vector.insert(index, qMakePair(d, 0.0));
            }
        }

        it.value() = vector;
    }


    // Sort from min to max
    divKeys = dividends.keys();

    if(divKeys.count() == 0)
    {
        return nullptr;
    }

    for (const QString &key : divKeys)
    {
        auto vector = dividends.value(key);

        std::sort(vector.begin(), vector.end(),
                  [] (QPair<QDate, double> &a, QPair<QDate, double> &b)
                  {
                      return a.first < b.first;
                  }
                  );

        dividends[key] = vector;
    }


    // Set all sets, tickers and dates
    QVector<QBarSet*> dividendsSets;
    divKeys = dividends.keys();

    for (const QString &key : divKeys)
    {
        QBarSet *bar = new QBarSet(key);

        auto vector = dividends.value(key);

        for (const QPair<QDate, double> &v : vector)
        {
            bar->append(v.second);
        }

        dividendsSets.push_back(bar);
    }


    QStackedBarSeries *dividendSeries = new QStackedBarSeries();
    double maxDividendAxis = 0.0;

    for(QBarSet *set : dividendsSets)
    {
        dividendSeries->append(set);

        qreal sum = set->sum();

        if(sum > maxDividendAxis)
        {
            maxDividendAxis = sum;
        }
    }


    if(xAxis != nullptr)
    {
        *xAxis = categories;
    }

    if(maxYAxis != nullptr)
    {
        *maxYAxis = maxDividendAxis;
    }

    return dividendSeries;
}

QStackedBarSeries* Calculation::getYearDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis)
{
    Q_ASSERT(stockData);
    Q_ASSERT(database);

    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }


    QVector<QPair<double, int> > dividends;     // price, year
    double maxDividendAxis = 0.0;
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

            if( stock.stockName.toLower().contains("fundshare") ) continue;

            if(stock.type == DIVIDEND)
            {
                QString rates;
                eCURRENCY currencyFrom = stock.currency;

                switch(currencyFrom)
                {
                    case USD: rates = "USD";
                        break;
                    case CZK: rates = "CZK";
                        break;
                    case EUR: rates = "EUR";
                        break;
                    case GBP: rates = "GBP";
                        break;
                }

                rates += "2";

                switch(selectedCurrency)
                {
                    case USD: rates += "USD";
                        break;
                    case CZK: rates += "CZK";
                        break;
                    case EUR: rates += "EUR";
                        break;
                    case GBP: rates += "GBP";
                        break;
                }

                double price = 0.0;

                price = database->getExchangePrice(rates, stock.price);

                if(price > maxDividendAxis)
                {
                    maxDividendAxis = price;
                }

                QString ticker = stock.ticker;
                QDate date = stock.dateTime.date();


                auto it = std::find_if(dividends.begin(), dividends.end(), [date](QPair<double, int> a)
                                       {
                                           return date.year() == a.second;
                                       }
                                       );

                if(it != dividends.end())  // we need to sum
                {
                    it->first += price;

                    if(it->first > maxDividendAxis)
                    {
                        maxDividendAxis = it->first;
                    }
                }
                else                    // create new record, because we have found new year
                {
                    dividends.push_back(qMakePair(price, date.year()));
                }
            }
        }
    }

    if(dividends.count() == 0)
    {
        return nullptr;
    }

    std::sort(dividends.begin(), dividends.end(),
              [] (QPair<double, int> &a, QPair<double, int> &b)
              {
                  return a.second < b.second;
              }
              );

    int yearMin = dividends.first().second;
    int yearMax = dividends.last().second;

    // Set all sets, ticker and date
    QStringList categories;
    QStackedBarSeries *dividendSeries = new QStackedBarSeries();

    for (const QPair<double, int> &key : dividends)
    {
        QBarSet *bar = new QBarSet(QString::number(key.second));

        for(int a = yearMin; a<yearMax+1; ++a)
        {
            if(a == key.second)
            {
                bar->append(key.first);
            }
            else
            {
                bar->append(0);
            }
        }

        categories << QString::number(key.second);

        dividendSeries->append(bar);
    }

    if(xAxis != nullptr)
    {
        *xAxis = categories;
    }

    if(maxYAxis != nullptr)
    {
        *maxYAxis = maxDividendAxis;
    }

    return dividendSeries;
}

QPieSeries* Calculation::getSectorSeries(const QDate &from, const QDate &to)
{
    QVector<sOVERVIEWTABLE> table = getOverviewTable(from, to);

    if(table.isEmpty())
    {
        return nullptr;
    }

    QVector<QPair<QString, double> > sectors;

    for(const sOVERVIEWTABLE &item : table)
    {
        auto it = std::find_if(sectors.begin(), sectors.end(), [item](QPair<QString, double> a)
                               {
                                   return a.first == item.sector;
                               }
                               );

        if(it != sectors.end()) // sector already exists
        {
            it->second += item.totalOnlinePrice;
        }
        else
        {
            sectors.push_back(qMakePair(item.sector, item.totalOnlinePrice));
        }
    }

    QPieSeries *sectorSeries = new QPieSeries();

    for(const QPair<QString, double> &item : sectors)
    {
        sectorSeries->append(item.first, item.second);
    }

    sectorSeries->setLabelsVisible();

    for(QPieSlice *slice : sectorSeries->slices())
    {
        slice->setLabel(QString("%1 (%2%)").arg(slice->label()).arg(100*slice->percentage(), 0, 'f', 1));
    }

    sectorSeries->setHoleSize(0.35);

    return sectorSeries;
}

QPieSeries* Calculation::getStockSeries(const QDate &from, const QDate &to)
{
    QVector<sOVERVIEWTABLE> table = getOverviewTable(from, to);

    if(table.isEmpty())
    {
        return nullptr;
    }

    QPieSeries *sectorSeries = new QPieSeries();

    for(const sOVERVIEWTABLE &item : table)
    {
        sectorSeries->append(item.ticker, item.percentage);
    }

    sectorSeries->setLabelsVisible();

    for(QPieSlice *slice : sectorSeries->slices())
    {
        slice->setLabel(QString("%1 (%2%)").arg(slice->label()).arg(100*slice->percentage(), 0, 'f', 2));
    }

    sectorSeries->setHoleSize(0.35);

    return sectorSeries;
}

QChart *Calculation::getChart(const eCHARTTYPE &type, const QDate &from, const QDate &to, const QString &ISIN)
{
    Q_ASSERT(database);

    QString currencySign = database->getCurrencySign(database->getSetting().currency);
    QChart* chart = new QChart();
    chart->setAcceptHoverEvents(true);

    switch(type)
    {
        case DEPOSITCHART:
        {
            QLineSeries *depositSeries = getDepositSeries(from, to);

            if(depositSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(depositSeries);
            chart->legend()->hide();
            chart->setTitle("Deposit");
            chart->setTheme(QChart::ChartThemeQt);

            QDateTimeAxis *depositAxisX = new QDateTimeAxis;
            depositAxisX->setTickCount(10);

            QPointF first = depositSeries->pointsVector().first();
            QPointF last = depositSeries->pointsVector().last();

            QDateTime firstDate = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(first.x()));
            QDateTime lastDate = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(last.x()));

            if(firstDate.date().year() == lastDate.date().year() && firstDate.date().month() == lastDate.date().month())
            {
                depositAxisX->setFormat("dd MMM yy");
            }
            else
            {
                depositAxisX->setFormat("MMM yyyy");
            }

            depositAxisX->setTitleText("Date");
            chart->addAxis(depositAxisX, Qt::AlignBottom);
            depositSeries->attachAxis(depositAxisX);

            QValueAxis *depositAxisY = new QValueAxis;
            depositAxisY->setLabelFormat("%i");
            depositAxisY->setTitleText("Deposit " + currencySign);
            chart->addAxis(depositAxisY, Qt::AlignLeft);
            depositSeries->attachAxis(depositAxisY);



            Callout *tooltip = new Callout(chart);

            connect(depositSeries, &QLineSeries::hovered, [tooltip, chart](const QPointF &point, bool state) mutable
                    {
                        if (tooltip == nullptr)
                        {
                            tooltip = new Callout(chart);
                        }

                        if (state)
                        {
                            tooltip->setText(QString("X: %1 \nY: %2 ").arg(QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("dd.MM.yyyy")).arg(point.y()));
                            tooltip->setAnchor(point);
                            tooltip->setZValue(11);
                            tooltip->updateGeometry();
                            tooltip->show();
                        }
                        else
                        {
                            tooltip->hide();
                        }
                    }
                    );
        }
        break;

        case INVESTEDCHART:
        {
            QLineSeries *investedSeries = getInvestedSeries(from, to);

            if(investedSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(investedSeries);
            chart->legend()->hide();
            chart->setTitle("Invested");
            chart->setTheme(QChart::ChartThemeQt);

            QDateTimeAxis *investedAxisX = new QDateTimeAxis;
            investedAxisX->setTickCount(10);

            QPointF first = investedSeries->pointsVector().first();
            QPointF last = investedSeries->pointsVector().last();

            QDateTime firstDate = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(first.x()));
            QDateTime lastDate = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(last.x()));

            if(firstDate.date().year() == lastDate.date().year() && firstDate.date().month() == lastDate.date().month())
            {
                investedAxisX->setFormat("dd MMM yy");
            }
            else
            {
                investedAxisX->setFormat("MMM yyyy");
            }

            investedAxisX->setTitleText("Date");
            chart->addAxis(investedAxisX, Qt::AlignBottom);
            investedSeries->attachAxis(investedAxisX);

            QValueAxis *investedAxisY = new QValueAxis;
            investedAxisY->setLabelFormat("%i");
            investedAxisY->setTitleText("Invested " + currencySign);
            chart->addAxis(investedAxisY, Qt::AlignLeft);
            investedSeries->attachAxis(investedAxisY);


            Callout *tooltip = new Callout(chart);

            connect(investedSeries, &QLineSeries::hovered, [tooltip, chart](const QPointF &point, bool state) mutable
                    {
                        if (tooltip == nullptr)
                        {
                            tooltip = new Callout(chart);
                        }

                        if (state)
                        {
                            tooltip->setText(QString("X: %1 \nY: %2 ").arg(QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("dd.MM.yyyy")).arg(point.y()));
                            tooltip->setAnchor(point);
                            tooltip->setZValue(11);
                            tooltip->updateGeometry();
                            tooltip->show();
                        }
                        else
                        {
                            tooltip->hide();
                        }
                    }
                    );
        }
        break;

        case DIVIDENDCHART:
        {
            QStringList categories;
            double maxDividendAxis;
            QBarSeries *dividendSeries = getDividendSeries(from, to, &categories, &maxDividendAxis);

            if(dividendSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(dividendSeries);
            chart->setTitle("Dividends");
            chart->setAnimationOptions(QChart::SeriesAnimations);

            QBarCategoryAxis *axisX = new QBarCategoryAxis();
            axisX->append(categories);
            chart->addAxis(axisX, Qt::AlignBottom);
            dividendSeries->attachAxis(axisX);

            QValueAxis *dividendsAxisY = new QValueAxis();
            dividendsAxisY->setLabelFormat("%i");
            dividendsAxisY->setTitleText("Dividend " + currencySign);
            dividendsAxisY->setRange(0, static_cast<int>(maxDividendAxis*1.1));
            chart->addAxis(dividendsAxisY, Qt::AlignLeft);
            dividendSeries->attachAxis(dividendsAxisY);

            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);

        }
        break;

        case MONTHDIVIDEND:
        {
            QStringList categories;
            double maxDividendAxis;
            QStackedBarSeries *dividendSeries = getMonthDividendSeries(from, to, &categories, &maxDividendAxis);

            if(dividendSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(dividendSeries);
            chart->setTitle("Month dividends");
            chart->setAnimationOptions(QChart::SeriesAnimations);

            QBarCategoryAxis *axisX = new QBarCategoryAxis();
            axisX->append(categories);
            chart->addAxis(axisX, Qt::AlignBottom);
            dividendSeries->attachAxis(axisX);

            QValueAxis *dividendsAxisY = new QValueAxis();
            dividendsAxisY->setLabelFormat("%i");
            dividendsAxisY->setTitleText("Dividend " + currencySign);
            //dividendsAxisY->setRange(0, static_cast<int>(maxDividendAxis*1.1));
            chart->addAxis(dividendsAxisY, Qt::AlignLeft);
            dividendSeries->attachAxis(dividendsAxisY);

            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);


            Callout *tooltip = new Callout(chart);

            connect(dividendSeries, &QStackedBarSeries::hovered, [tooltip, chart, dividendSeries, currencySign](bool status, int index, QBarSet *barset) mutable
                    {
                        if (tooltip == nullptr)
                        {
                            tooltip = new Callout(chart);
                        }

                        if (status)
                        {
                            tooltip->setText(QString("%1: %2%3").arg(barset->label()).arg(currencySign).arg(barset->at(index)));
                            tooltip->setAnchor(QPointF(dividendSeries->barWidth()*index*2, 0));
                            tooltip->setZValue(11);
                            tooltip->updateGeometry();
                            tooltip->show();
                        }
                        else
                        {
                            tooltip->hide();
                        }
                    }
                    );
        }
        break;

        case YEARDIVIDENDCHART:
        {
            QStringList categories;
            double maxDividendAxis;
            QStackedBarSeries  *yearDividendSeries = getYearDividendSeries(from, to, &categories, &maxDividendAxis);

            if(yearDividendSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(yearDividendSeries);
            chart->setTitle("Year dividends");
            chart->setAnimationOptions(QChart::SeriesAnimations);

            QBarCategoryAxis *axisX = new QBarCategoryAxis();
            axisX->append(categories);
            chart->addAxis(axisX, Qt::AlignBottom);
            yearDividendSeries->attachAxis(axisX);

            QValueAxis *dividendsAxisY = new QValueAxis();
            dividendsAxisY->setLabelFormat("%i");
            dividendsAxisY->setTitleText("Dividend " + currencySign);
            dividendsAxisY->setRange(0, static_cast<int>(maxDividendAxis*1.1));
            chart->addAxis(dividendsAxisY, Qt::AlignLeft);
            yearDividendSeries->attachAxis(dividendsAxisY);

            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);


            Callout *tooltip = new Callout(chart);

            connect(yearDividendSeries, &QStackedBarSeries::hovered, [tooltip, chart, yearDividendSeries](bool status, int index, QBarSet *barset) mutable
                    {
                        if (tooltip == nullptr)
                        {
                            tooltip = new Callout(chart);
                        }

                        if (status)
                        {
                            tooltip->setText(QString("Y: %1").arg(barset->at(index)));
                            tooltip->setAnchor(QPointF(yearDividendSeries->barWidth()*index*2, 0));
                            tooltip->setZValue(11);
                            tooltip->updateGeometry();
                            tooltip->show();
                        }
                        else
                        {
                            tooltip->hide();
                        }
                    }
                    );
        }
        break;

        case SECTORCHART:
        {
            QPieSeries *sectorSeries = getSectorSeries(from, to);

            if(sectorSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(sectorSeries);
            chart->setTitle("Sectors");
            chart->legend()->hide();
        }
        break;

        case STOCKCHART:
        {
            QPieSeries *stockSeries = getStockSeries(from, to);

            if(stockSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(stockSeries);
            chart->setTitle("Stocks");
            chart->legend()->hide();
        }
        break;

        case ISINCHART:
        {
            QStringList categories;
            double maxDividendAxis;
            QBarSeries *dividendSeries = getDividendSeries(from, to, &categories, &maxDividendAxis, ISIN);

            if(dividendSeries == nullptr)
            {
                delete chart;
                chart = nullptr;

                return nullptr;
            }

            chart->addSeries(dividendSeries);
            chart->setTitle("Year dividends");
            chart->setAnimationOptions(QChart::SeriesAnimations);

            QBarCategoryAxis *dividendsAxisX = new QBarCategoryAxis();
            dividendsAxisX->append(categories);
            chart->addAxis(dividendsAxisX, Qt::AlignBottom);
            dividendSeries->attachAxis(dividendsAxisX);

            QValueAxis *dividendsAxisY = new QValueAxis();
            dividendsAxisY->setRange(0, static_cast<int>(maxDividendAxis+0.1*maxDividendAxis));
            chart->addAxis(dividendsAxisY, Qt::AlignLeft);
            dividendSeries->attachAxis(dividendsAxisY);

            chart->legend()->setVisible(false);
        }
        break;
    }

    return chart;
}

QChartView* Calculation::getChartView(const eCHARTTYPE &type, const QDate &from, const QDate &to, const QString &ISIN)
{
    QChartView *view = nullptr;

    QChart *chart = getChart(type, from, to, ISIN);

    if(chart == nullptr)
    {
        return nullptr;
    }

    switch(type)
    {
        case DEPOSITCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case INVESTEDCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case DIVIDENDCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case MONTHDIVIDEND:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case YEARDIVIDENDCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case SECTORCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case STOCKCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case ISINCHART:
        {
            view = new QChartView(chart);
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;
    }

    return view;
}
