#ifndef VARIABLES_H
#define VARIABLES_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <QMap>

// x lines of y values of pair key-value
typedef QVector<QPair<QString, QString> > TickerDataType;
typedef QVector<TickerDataType> ScreenerDataType;

#define STOCKFILE           "/stock.bin"
#define ISINFILE            "/isin.bin"
#define DEGIRORAWFILE       "/degiroRAW.bin"
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
    USD = 2
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

    // Exchange
    QDate lastExchangeRatesUpdate;
    double USD2EUR;
    double USD2CZK;
    double EUR2CZK;
    double CZK2USD;
    double EUR2USD;

    // DeGiro
    QString degiroCSV;
    eDELIMETER CSVdelimeter;
    bool degiroAutoLoad;

    // Screener
    QVector<sSCREENERPARAM> screenerParams;
    int lastScreenerIndex;
    bool filterON;
    bool screenerAutoLoad;
};

struct sPDFEXPORT
{
    QString currency;
    QDateTime date;
    double price;
    double tax;
    QString name;
};


enum eSTOCKTYPE
{
    DIVIDEND,
    TAX,
    TRANSACTIONFEE,
    FEE,
    DEPOSIT,
    WITHDRAWAL,
    BUY,
    SELL
};

struct sSTOCKDATA
{
    QDateTime dateTime;
    eSTOCKTYPE type;
    QString ticker;
    QString ISIN;
    eCURRENCY currency;
    int count;
    double price;

    bool isDegiroSource;
};

struct sNEWRECORD
{
    QDateTime dateTime;
    eSTOCKTYPE type;
    QString ticker;
    QString ISIN;
    eCURRENCY currency;
    int count;
    double price;
    double fee;
};

typedef QHash<QString, QVector<sSTOCKDATA>> StockDataType;


struct sDEGIRORAW
{
    QDateTime dateTime;
    QString product;
    QString ISIN;
    QString description;
    eCURRENCY currency;
    double money;
};


struct sTICKERINFO
{
    QString stockName;
    QString sector;
    QString industry;
    QString country;
    QString ticker;
};

struct sTABLE
{
    QHash<QString, QString> row;    // name, value
    sTICKERINFO info;
};


enum eSCREENSOURCE
{
    FINVIZ = 0,
    YAHOO
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

struct sISINLIST
{
    QString ISIN;
    QString ticker;
    QString name;
    QString sector;
    QString industry;
};

#endif // VARIABLES_H
