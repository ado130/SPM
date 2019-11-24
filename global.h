#ifndef VARIABLES_H
#define VARIABLES_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QMap>

// x lines of y values of pair key-value
typedef QList<QPair<QString, QString> > tickerDataType;
typedef QList<tickerDataType> screenerDataType;

#define DEGIRORAWFILE       "/degiroRAW.bin"
#define SCREENERPARAMSFILE  "/screenParams.bin"
#define SCREENERALLDATA     "/screenerAllData.bin"
#define FILTERLISTFILE      "/filterList.bin"
#define CONFIGFILE          "/config.ini"

struct sINFO
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
    sINFO info;
};

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

struct sSETTINGS
{
    // DeGiro
    QString degiroCSV;
    eDELIMETER CSVdelimeter;
    bool autoload;

    // Window
    int width;
    int height;
    int xPos;
    int yPos;
    int lastOpenedTab;

    // Screener
    QList<sSCREENERPARAM> screenerParams;
    int lastScreenerIndex;
    bool filterON;
};

enum eSCREENSOURCE
{
    FINVIZ = 0,
    YAHOO
};

enum eCURRENCY
{
    CZK = 0,
    EUR = 1,
    USD = 2
};

struct sSCREENER
{
    screenerDataType screenerData;
    QString screenerName;
};

struct sDEGIRORAW
{
    QDateTime dateTime;
    QString product;
    QString ISIN;
    QString description;
    eCURRENCY currency;
    double money;
};

enum eDEGIROTYPE
{
    DIVIDEND,
    FEE,
    DEPOSIT,
    WITHDRAWAL,
    BUY,
    SELL

};

struct sDEGIRODATA
{
    QDateTime dateTime;
    QString product;
    QString ISIN;
    eDEGIROTYPE type;
    eCURRENCY currency;
    double money;
};

struct sDEGIRO
{
    QString ticker;
    sDEGIRODATA data;
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

#endif // VARIABLES_H
