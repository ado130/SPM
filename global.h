#ifndef VARIABLES_H
#define VARIABLES_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QMap>

// x lines of y values of pair key-value
typedef QVector<QPair<QString, QString> > TickerDataType;
typedef QVector<TickerDataType> ScreenerDataType;
typedef QMap<QString, std::function<double(double)> > ExchangeRatesFunctions;

#define STOCKFILE           "/stock.bin"
#define ISINFILE            "/isin.bin"
#define DEGIRORAWFILE       "/degiroRAW.bin"
#define TASTYWORKSRAWFILE   "/tastyworksRAW.bin"
#define SCREENERPARAMSFILE  "/screenParams.bin"
#define SCREENERALLDATA     "/screenerAllData.bin"
#define FILTERLISTFILE      "/filterList.bin"
#define CONFIGFILE          "/config.ini"


enum eDELIMETER
{
    COMMA_SEPARATED = 0,
    SEMICOLON_SEPARATED = 1,
    POINT_SEPARATED = 2
};

struct sSCREENERPARAM
{
    QString name;
    bool enabled;
};

enum eCURRENCY
{
    CZK = 0,
    EUR = 1,
    USD = 2,
    GBP = 3
};

struct sSETTINGS
{
    // Window
    int width;
    int height;
    int xPos;
    int yPos;
    eCURRENCY currency;
    int lastOpenedTab;

    // Overview
    QDate lastOverviewFrom;
    QDate lastOverviewTo;
    bool showSoldPositions;

    // Exchange
    QDate lastExchangeRatesUpdate;
    double CZK2USD;
    double CZK2EUR;
    double CZK2GBP;

    double EUR2USD;
    double EUR2GBP;
    double EUR2CZK;

    double GBP2USD;
    double GBP2EUR;
    double GBP2CZK;

    double USD2EUR;
    double USD2GBP;
    double USD2CZK;   

    double EUR2CZKDAP;
    double USD2CZKDAP;
    double GBP2CZKDAP;

    // DeGiro
    QString degiroCSV;
    eDELIMETER degiroCSVdelimeter;
    bool degiroAutoLoad;

    // Tastyworks
    QString tastyworksCSV;
    eDELIMETER tastyworksCSVdelimeter;
    bool tastyworksAutoLoad;

    // Screener
    QVector<sSCREENERPARAM> screenerParams;
    int lastScreenerIndex;
    bool filterON;
    bool screenerAutoLoad;
};

struct sOVERVIEWTABLE
{
    QString ISIN;
    QString ticker;
    QString stockName;
    QString sector;
    double percentage;
    int totalCount;
    double averageBuyPrice;
    double totalStockPrice;
    double totalFee;
    double onlineStockPrice;
    double totalOnlinePrice;
    double dividend;
};

struct sOVERVIEWINFO
{
    double deposit;
    double invested;
    double withdrawal;
    double dividends;
    double divTax;
    double DY;
    double fees;
    double transFees;
    double account;
    double sell;
    double portfolio;
    double performance;
};

enum eCHARTTYPE
{
    DEPOSITCHART = 0,
    INVESTEDCHART,
    DIVIDENDCHART,
    SECTORCHART,
    ISINCHART
};


struct sPDFEXPORT
{
    QString paid;
    QDateTime date;
    double price;
    double tax;
    QString name;
};


enum eSTOCKEVENTTYPE
{
    DEPOSIT = 0,
    WITHDRAWAL = 1,
    BUY = 2,
    SELL = 3,
    FEE = 4,
    DIVIDEND = 5,
    TAX = 6,
    TRANSACTIONFEE = 7
};

enum eSTOCKSOURCE
{
    MANUALLY = 0,
    DEGIRO = 1,
    TASTYWORKS = 2,
    LYNX = 4
};

struct sSTOCKDATA
{
    QDateTime dateTime;
    eSTOCKEVENTTYPE type;
    QString ticker;
    QString ISIN;
    QString stockName;
    eCURRENCY currency;

    int count;
    double price;

    double fee;         // dividend--tax; buy/sell--transactionfee

    eSTOCKSOURCE source;
};

struct sNEWRECORD
{
    QDateTime dateTime;
    eSTOCKEVENTTYPE type;
    QString ticker;
    QString ISIN;
    eCURRENCY currency;
    int count;
    double price;
    double fee;
};

/**
 * @brief StockDataType
 * ISIN, sSTOCKDATA
 */
typedef QHash<QString, QVector<sSTOCKDATA>> StockDataType;


struct sDEGIRORAW
{
    QDateTime dateTime;
    QString product;
    QString ISIN;
    QString description;
    eCURRENCY currency;
    double price;
};

struct sTASTYWORKSRAW
{
    QDateTime dateTime;
    eSTOCKEVENTTYPE type;
    QString ticker;
    QString description;
    double price;
    int count;
    double fee;
};

struct sTICKERINFO
{
    QString stockName;
    QString sector;
    QString industry;
    QString country;
    QString ticker;
};

struct sONLINEDATA
{
    QHash<QString, QString> row;    // name, value
    sTICKERINFO info;
};


enum eSCREENSOURCE
{
    FINVIZ = 0,
    YAHOO = 1
};

struct sSCREENER
{
    ScreenerDataType screenerData;
    QString screenerName;
};

enum eFILTER
{
    LOWER = 0,
    HIGHER = 1,
    BETWEEN = 2
};

struct sFILTER
{
    QString param;
    eFILTER filter;
    QString color;
    double val1;
    double val2;
};

struct sISINDATA
{
    QString ISIN;
    QString ticker;
    QString name;
    QString sector;
    QString industry;

    QDateTime lastUpdate;
};

#endif // VARIABLES_H
