#include "calculation.h"



Calculation::Calculation(Database *db, StockData *sd, QObject *parent) : QObject(parent), database(db), stockData(sd)
{

}

double Calculation::getPortfolioValue(const QDate &from, const QDate &to)
{
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

        if(totalCount == 0 && !database->getSetting().showSoldPositions) continue;


        QString cachedPrice = stockData->getCachedISINParam(stock.ISIN, "Price");
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

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

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
                    sell += database->getExchangePrice(rates, stock.price) * stock.count;
                    transFees += database->getExchangePrice(rates, stock.fee);

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

    info.account = (deposit + sell + dividends - divTax - invested - fees - transFees - withdrawal);
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

        if(totalCount == 0 && !database->getSetting().showSoldPositions) continue;

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

            if(stock.type == DEPOSIT)
            {
                deposit += database->getExchangePrice(rates, stock.price);

                depositSeries->append(stock.dateTime.toMSecsSinceEpoch(), deposit);
            }
        }
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
        depositSeries->append(QDateTime(QDate(QDate::currentDate().year(), 1, 1)).toMSecsSinceEpoch(), 0);
    }

    return depositSeries;
}

QLineSeries* Calculation::getInvestedSeries(const QDate &from, const QDate &to)
{
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

            if(stock.type == BUY)
            {
                invested += database->getExchangePrice(rates, (-1.0)*stock.price) * stock.count;

                investedSeries->append(stock.dateTime.toMSecsSinceEpoch(), invested);
            }
        }
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
        investedSeries->append(QDateTime(QDate(QDate::currentDate().year(), 1, 1)).toMSecsSinceEpoch(), 0);
    }

    return investedSeries;
}

QBarSeries* Calculation::getDividendSeries(const QDate &from, const QDate &to, QStringList *xAxis, double *maxYAxis)
{
    StockDataType stockList = stockData->getStockData();

    if(stockList.isEmpty())
    {
        return nullptr;
    }

    QHash<QString, QVector<QPair<QDate, double>> > dividends;
    double maxDividendAxis = 0.0;
    eCURRENCY selectedCurrency = database->getSetting().currency;
    QList<QString> keys = stockList.keys();

    for(const QString &key : keys)
    {
        for(const sSTOCKDATA &stock : stockList.value(key))
        {
            if( !(stock.dateTime.date() >= from && stock.dateTime.date() <= to) ) continue;

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

            if(stock.type == DIVIDEND)
            {
                double price = 0.0;

                price = database->getExchangePrice(rates, stock.price);

                if(price > maxDividendAxis) maxDividendAxis = price;

                QString ticker = stock.ticker;
                QDate date = stock.dateTime.date();

                auto vector = dividends.value(ticker);
                vector.push_back(qMakePair(date, price));
                dividends.insert(ticker, vector);
            }
        }
    }

    // Dividends
    // Sort from min to max and find the min and max
    QDate min;
    QDate max;

    QList<QString> divKeys = dividends.keys();

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

        if(localMin < min) min = localMin;
        if(localMax > max) max = localMax;
    }

    // Save categories - find all months between min and max date
    QStringList categories;
    QDate tmpMin = min;
    QVector<QDate> dates;

    while(tmpMin < max)
    {
        QString month = tmpMin.toString("MMM");
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
                                          return d.month() == a.first.month();
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

QChart* Calculation::getChart(const eCHARTTYPE &type, const QDate &from, const QDate &to)
{
    QString currencySign = database->getCurrencySign(database->getSetting().currency);
    QChart* chart = new QChart();

    switch(type)
    {
        case DEPOSITCHART:
        {
            QLineSeries *depositSeries = getDepositSeries(from, to);

            if(depositSeries == nullptr)
            {
                return nullptr;
            }

            chart->addSeries(depositSeries);
            chart->legend()->hide();
            chart->setTitle("Deposit");
            chart->setTheme(QChart::ChartThemeQt);

            QDateTimeAxis *depositAxisX = new QDateTimeAxis;
            depositAxisX->setTickCount(10);
            depositAxisX->setFormat("MMM yyyy");
            depositAxisX->setTitleText("Date");
            chart->addAxis(depositAxisX, Qt::AlignBottom);
            depositSeries->attachAxis(depositAxisX);

            QValueAxis *depositAxisY = new QValueAxis;
            depositAxisY->setLabelFormat("%i");
            depositAxisY->setTitleText("Deposit " + currencySign);
            chart->addAxis(depositAxisY, Qt::AlignLeft);
            depositSeries->attachAxis(depositAxisY);
        }
        break;

        case INVESTEDCHART:
        {
            QLineSeries *investedSeries = getInvestedSeries(from, to);

            if(investedSeries == nullptr)
            {
                return nullptr;
            }

            chart->addSeries(investedSeries);
            chart->legend()->hide();
            chart->setTitle("Invested");
            chart->setTheme(QChart::ChartThemeQt);

            QDateTimeAxis *investedAxisX = new QDateTimeAxis;
            investedAxisX->setTickCount(10);
            investedAxisX->setFormat("MMM yyyy");
            investedAxisX->setTitleText("Date");
            chart->addAxis(investedAxisX, Qt::AlignBottom);
            investedSeries->attachAxis(investedAxisX);

            QValueAxis *investedAxisY = new QValueAxis;
            investedAxisY->setLabelFormat("%i");
            investedAxisY->setTitleText("Invested " + currencySign);
            chart->addAxis(investedAxisY, Qt::AlignLeft);
            investedSeries->attachAxis(investedAxisY);
        }
        break;

        case DIVIDENDCHART:
        {
            QStringList categories;
            double maxDividendAxis;
            QBarSeries *dividendSeries = getDividendSeries(from, to, &categories, &maxDividendAxis);

            if(dividendSeries == nullptr)
            {
                return nullptr;
            }

            chart->addSeries(dividendSeries);
            chart->addSeries(dividendSeries);
            chart->setTitle("Dividends");
            chart->setAnimationOptions(QChart::SeriesAnimations);

            QBarCategoryAxis *axisX = new QBarCategoryAxis();
            axisX->append(categories);
            chart->addAxis(axisX, Qt::AlignBottom);
            dividendSeries->attachAxis(axisX);

            QValueAxis *dividendsAxisY = new QValueAxis();
            dividendsAxisY->setRange(0, static_cast<int>(maxDividendAxis+0.1*maxDividendAxis));
            chart->addAxis(dividendsAxisY, Qt::AlignLeft);
            dividendSeries->attachAxis(dividendsAxisY);

            chart->legend()->setVisible(true);
            chart->legend()->setAlignment(Qt::AlignBottom);
        }
        break;

        case SECTORCHART:
        {

        }
        break;
    }

    return chart;
}

QChartView* Calculation::getChartView(const eCHARTTYPE &type, const QDate &from, const QDate &to)
{
    QChartView *view = nullptr;

    switch(type)
        {
        case DEPOSITCHART:
        {
            view = new QChartView(getChart(type, from, to));
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case INVESTEDCHART:
        {
            view = new QChartView(getChart(type, from, to));
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case DIVIDENDCHART:
        {
            view = new QChartView(getChart(type, from, to));
            view->setRenderHint(QPainter::Antialiasing);
            view->setMinimumSize(512, 512);
            view->setRubberBand(QChartView::HorizontalRubberBand);
        }
        break;

        case SECTORCHART:
        {

        }
        break;
    }

    return view;
}
