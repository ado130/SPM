#include "database.h"

#include <QSettings>
#include <QCoreApplication>
#include <QDebug>
#include <QDate>
#include <QFileInfo>
#include <QTime>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

Database::Database(QObject *parent) : QObject(parent)
{
    QString writablePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    if(writablePath.isEmpty())
    {
        qFatal("Cannot determine settings storage location");
    }

    QDir localDir(writablePath);

    if(!localDir.exists())
    {
        localDir.mkpath(".");
    }


    // Copy content of old directory (Roaming in AppData) into local folder (Local in AppData)
    QDir roamingDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    if(roamingDir.exists())
    {
        bool b = copyDirectoryFiles(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
                                    writablePath,
                                    true,
                                    true);

        if(b)
        {
            roamingDir.removeRecursively();
        }
    }


    /*
     * Fill exchange rates function
     */
    exchangeRatesFuncMap =
        {
            { "CZK2CZK", [](double x){return x; }},
            { "CZK2EUR", [this](double x){return (x * setting.CZK2EUR); }},
            { "CZK2USD", [this](double x){return (x * setting.CZK2USD); }},
            { "CZK2GBP", [this](double x){return (x * setting.CZK2GBP); }},
            { "CZK2CAD", [this](double x){return (x * setting.CZK2CAD); }},
            { "EUR2EUR", [](double x){return x; }},
            { "EUR2CZK", [this](double x){return (x * setting.EUR2CZK); }},
            { "EUR2USD", [this](double x){return (x * setting.EUR2USD); }},
            { "EUR2GBP", [this](double x){return (x * setting.EUR2GBP); }},
            { "EUR2CAD", [this](double x){return (x * setting.EUR2CAD); }},
            { "USD2USD", [](double x){return x; }},
            { "USD2CZK", [this](double x){return (x * setting.USD2CZK); }},
            { "USD2EUR", [this](double x){return (x * setting.USD2EUR); }},
            { "USD2GBP", [this](double x){return (x * setting.USD2GBP); }},
            { "USD2CAD", [this](double x){return (x * setting.USD2CAD); }},
            { "GBP2GBP", [](double x){return x; }},
            { "GBP2CZK", [this](double x){return (x * setting.GBP2CZK); }},
            { "GBP2USD", [this](double x){return (x * setting.GBP2USD); }},
            { "GBP2EUR", [this](double x){return (x * setting.GBP2EUR); }},
            { "GBP2CAD", [this](double x){return (x * setting.GBP2CAD); }},
            { "CAD2CAD", [](double x){return x; }},
            { "CAD2CZK", [this](double x){return (x * setting.CAD2CZK); }},
            { "CAD2USD", [this](double x){return (x * setting.CAD2USD); }},
            { "CAD2EUR", [this](double x){return (x * setting.CAD2EUR); }},
            { "CAD2GBP", [this](double x){return (x * setting.CAD2GBP); }}
        };

    loadConfig();
    loadScreenParams();
    loadFilterList();
    loadIsinData();
}

void Database::loadConfig()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + CONFIGFILE, QSettings::IniFormat);
    setting.degiroCSV = settings.value("DeGiro/path", "").toString();
    setting.degiroCSVdelimeter = static_cast<eDELIMETER>(settings.value("DeGiro/delimeter", 0).toInt());
    setting.degiroAutoLoad = settings.value("DeGiro/Dautoload", false).toBool();

    setting.tastyworksCSV = settings.value("Tastyworks/path", "").toString();
    setting.tastyworksCSVdelimeter = static_cast<eDELIMETER>(settings.value("Tastyworks/delimeter", 0).toInt());
    setting.tastyworksAutoLoad = settings.value("Tastyworks/Dautoload", false).toBool();

    setting.lastScreenerIndex = settings.value("Screener/lastScreenerIndex", -1).toInt();
    setting.filterON = settings.value("Screener/filterON", false).toBool();
    setting.screenerAutoLoad = settings.value("Screener/Sautoload", false).toBool();

    setting.width = settings.value("General/width", 0).toInt();
    setting.height = settings.value("General/height", 0).toInt();
    setting.xPos = settings.value("General/xPos", 0).toInt();
    setting.yPos = settings.value("General/yPos", 0).toInt();
    setting.lastOpenedTab = settings.value("General/lastOpenedTab", 0).toInt();
    setting.currency = static_cast<eCURRENCY>(settings.value("General/currency", 0).toInt());

    setting.lastOverviewFrom = QDate::fromString(settings.value("Overview/lastOverviewFrom", QDate(QDate::currentDate().year(), 1, 1).toString("dd.MM.yyyy")).toString(), "dd.MM.yyyy");
    setting.lastOverviewTo = QDate::fromString(settings.value("Overview/lastOverviewTo", QDate(QDate::currentDate().year(), 12, 31).toString("dd.MM.yyyy")).toString(), "dd.MM.yyyy");
    setting.showSoldPositions = settings.value("Overview/soldPositions", false).toBool();

    QDate currentDate = QDate::currentDate();
    currentDate = currentDate.addDays(-1);
    setting.lastExchangeRatesUpdate = QDate::fromString(settings.value("Exchange/lastExchangeRatesUpdate", currentDate.toString("dd.MM.yyyy")).toString(), "dd.MM.yyyy");
    setting.CZK2USD = settings.value("Exchange/CZK2USD", 0.044).toDouble();
    setting.CZK2EUR = settings.value("Exchange/CZK2EUR", 0.040).toDouble();
    setting.CZK2GBP = settings.value("Exchange/CZK2GBP", 0.034).toDouble();
    setting.CZK2CAD = settings.value("Exchange/CZK2CAD", 0.057).toDouble();

    setting.EUR2USD = settings.value("Exchange/EUR2USD", 1.12).toDouble();
    setting.EUR2GBP = settings.value("Exchange/EUR2GBP", 0.85).toDouble();
    setting.EUR2CZK = settings.value("Exchange/EUR2CZK", 25.42).toDouble();
    setting.EUR2CAD = settings.value("Exchange/EUR2CAD", 1.48).toDouble();

    setting.GBP2CZK = settings.value("Exchange/GBP2CZK", 29.71).toDouble();
    setting.GBP2EUR = settings.value("Exchange/GBP2EUR", 1.18).toDouble();
    setting.GBP2USD = settings.value("Exchange/GBP2USD", 1.31).toDouble();
    setting.GBP2CAD = settings.value("Exchange/GBP2CAD", 1.70).toDouble();

    setting.USD2CZK = settings.value("Exchange/USD2CZK", 22.68).toDouble();
    setting.USD2EUR = settings.value("Exchange/USD2EUR", 0.89).toDouble();
    setting.USD2GBP = settings.value("Exchange/USD2GBP", 0.76).toDouble();
    setting.USD2CAD = settings.value("Exchange/USD2CAD", 1.23).toDouble();

    setting.CAD2CZK = settings.value("Exchange/CAD2CZK", 17.50).toDouble();
    setting.CAD2EUR = settings.value("Exchange/CAD2EUR", 0.68).toDouble();
    setting.CAD2GBP = settings.value("Exchange/CAD2GBP", 0.59).toDouble();
    setting.CAD2USD = settings.value("Exchange/CAD2USD", 0.81).toDouble();

    setting.EUR2CZKDAP = settings.value("Exchange/EUR2CZKDAP", 25.66).toDouble();
    setting.USD2CZKDAP = settings.value("Exchange/USD2CZKDAP", 22.93).toDouble();
    setting.GBP2CZKDAP = settings.value("Exchange/GBP2CZKDAP", 29.31).toDouble();
    setting.CAD2CZKDAP = settings.value("Exchange/CAD2CZKDAP", 17.50).toDouble();
}

void Database::saveConfig()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + CONFIGFILE, QSettings::IniFormat);
    settings.setValue("DeGiro/path", setting.degiroCSV);
    settings.setValue("DeGiro/delimeter", setting.degiroCSVdelimeter);
    settings.setValue("DeGiro/Dautoload", setting.degiroAutoLoad);

    settings.setValue("Tastyworks/path", setting.tastyworksCSV);
    settings.setValue("Tastyworks/delimeter", setting.tastyworksCSVdelimeter);
    settings.setValue("Tastyworks/Dautoload", setting.tastyworksAutoLoad);

    settings.setValue("Screener/lastScreenerIndex", setting.lastScreenerIndex);
    settings.setValue("Screener/filterON", setting.filterON);
    settings.setValue("Screener/Sautoload", setting.screenerAutoLoad);

    settings.setValue("General/width", setting.width);
    settings.setValue("General/height", setting.height);
    settings.setValue("General/xPos", setting.xPos);
    settings.setValue("General/yPos", setting.yPos);
    settings.setValue("General/lastOpenedTab", setting.lastOpenedTab);
    settings.setValue("General/currency", setting.currency);

    settings.setValue("Overview/lastOverviewFrom", setting.lastOverviewFrom.toString("dd.MM.yyyy"));
    settings.setValue("Overview/lastOverviewTo", setting.lastOverviewTo.toString("dd.MM.yyyy"));
    settings.setValue("Overview/soldPositions", setting.showSoldPositions);

    settings.setValue("Exchange/lastExchangeRatesUpdate", setting.lastExchangeRatesUpdate.toString("dd.MM.yyyy"));

    settings.setValue("Exchange/CZK2USD", setting.CZK2USD);
    settings.setValue("Exchange/CZK2EUR", setting.CZK2EUR);
    settings.setValue("Exchange/CZK2GBP", setting.CZK2GBP);
    settings.setValue("Exchange/CZK2CAD", setting.CZK2CAD);

    settings.setValue("Exchange/EUR2USD", setting.EUR2USD);
    settings.setValue("Exchange/EUR2GBP", setting.EUR2GBP);
    settings.setValue("Exchange/EUR2CZK", setting.EUR2CZK);
    settings.setValue("Exchange/EUR2CAD", setting.EUR2CAD);

    settings.setValue("Exchange/USD2CZK", setting.USD2CZK);
    settings.setValue("Exchange/USD2EUR", setting.USD2EUR);
    settings.setValue("Exchange/USD2GBP", setting.USD2GBP);
    settings.setValue("Exchange/USD2CAD", setting.USD2CAD);

    settings.setValue("Exchange/GBP2CZK", setting.GBP2CZK);
    settings.setValue("Exchange/GBP2EUR", setting.GBP2EUR);
    settings.setValue("Exchange/GBP2USD", setting.GBP2USD);
    settings.setValue("Exchange/GBP2CAD", setting.GBP2CAD);

    settings.setValue("Exchange/GBP2CZK", setting.CAD2CZK);
    settings.setValue("Exchange/GBP2EUR", setting.CAD2EUR);
    settings.setValue("Exchange/GBP2USD", setting.CAD2USD);
    settings.setValue("Exchange/GBP2CAD", setting.CAD2GBP);

    settings.setValue("Exchange/EUR2CZKDAP", setting.EUR2CZKDAP);
    settings.setValue("Exchange/USD2CZKDAP", setting.USD2CZKDAP);
    settings.setValue("Exchange/GBP2CZKDAP", setting.GBP2CZKDAP);
    settings.setValue("Exchange/CAD2CZKDAP", setting.CAD2CZKDAP);
}

void Database::loadScreenParams()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + SCREENERPARAMSFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> setting.screenerParams;
            qFile.close();
        }
    }

    setEnabledScreenerParams();
}

void Database::saveScreenerParams()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + SCREENERPARAMSFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << setting.screenerParams;
        qFile.close();
    }
}

QDataStream &operator<<(QDataStream &out, const sSCREENERPARAM &param)
{
    out << param.name;
    out << param.enabled;

    return out;
}

QDataStream &operator>>(QDataStream &in, sSCREENERPARAM &param)
{
    in >> param.name;
    in >> param.enabled;

    return in;
}

double Database::getExchangePrice(const eCURRENCY &currencyFrom, const double &price)
{
    QString rates;

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
        case CAD: rates = "CAD";
            break;
    }

    rates += "2";

    switch(setting.currency)
    {
        case USD: rates += "USD";
            break;
        case CZK: rates += "CZK";
            break;
        case EUR: rates += "EUR";
            break;
        case GBP: rates += "GBP";
            break;
        case CAD: rates += "CAD";
            break;
    }

    return exchangeRatesFuncMap[rates](price);
}

QString Database::getCurrencyText(eCURRENCY currency)
{
    switch (currency)
    {
        case CZK: return "CZK";
        case EUR: return "EUR";
        case USD: return "USD";
        case GBP: return "GBP";
        case CAD: return "CAD";
    }

    return "";
}

QString Database::getCurrencySign(eCURRENCY currency)
{
    switch (currency)
    {
        case CZK: return "Kč";
        case EUR: return "€";
        case USD: return "$";
        case GBP: return "Ł";
        case CAD: return "CAD";
    }

    return "";
}

// Getters and Setters
sSETTINGS Database::getSetting() const
{
    return setting;
}

void Database::setSettingSlot(const sSETTINGS &value)
{
    setting = value;
    saveConfig();
}

ExchangeRatesFunctions Database::getExchangeRatesFuncMap() const
{
    return exchangeRatesFuncMap;
}

QString Database::getDegiroCSV() const
{
    return setting.degiroCSV;
}

void Database::setDegiroCSV(const QString &value)
{
    setting.degiroCSV = value;
    saveConfig();
}

int Database::getLastScreenerIndex() const
{
    return setting.lastScreenerIndex;
}

void Database::setLastScreenerIndex(const int &value)
{
    setting.lastScreenerIndex = value;
    saveConfig();
}

QVector<sSCREENERPARAM> Database::getScreenerParams() const
{
    return setting.screenerParams;
}

void Database::setScreenerParams(const QVector<sSCREENERPARAM> &value)
{
    setting.screenerParams = value;
    saveScreenerParams();
    setEnabledScreenerParams();
}

void Database::setEnabledScreenerParams()
{
    enabledScreenerParams.clear();

    for(const sSCREENERPARAM &param : qAsConst(setting.screenerParams))
    {
        if(param.enabled)
        {
            enabledScreenerParams << param.name;
        }
    }
}

QStringList Database::getEnabledScreenerParams() const
{
    return enabledScreenerParams;
}


QVector<sFILTER> Database::getFilterList() const
{
    return filterList;
}

void Database::setFilterList(const QVector<sFILTER> &value)
{
    filterList = value;
    saveFilterList();
}

void Database::loadFilterList()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + FILTERLISTFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> filterList;
            qFile.close();
        }
    }
}

void Database::saveFilterList()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + FILTERLISTFILE);
    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << filterList;
        qFile.close();
    }
}

QDataStream &operator<<(QDataStream &out, const sFILTER &param)
{
    out << param.param;
    out << static_cast<int>(param.filter);
    out << param.color;
    out << param.val1;
    out << param.val2;

    return out;
}

QDataStream &operator>>(QDataStream &in, sFILTER &param)
{
    in >> param.param;

    int buffer;
    in >> buffer;
    param.filter = static_cast<eFILTER>(buffer);

    in >> param.color;
    in >> param.val1;
    in >> param.val2;

    return in;
}

QVector<sISINDATA> Database::getIsinList() const
{
    return isinList;
}

void Database::setIsinList(const QVector<sISINDATA> &value)
{
    isinList = value;
    saveIsinData();
}

bool Database::loadIsinData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + ISINFILE);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> isinList;
            qFile.close();
            return true;
        }
    }

    return false;
}

void Database::saveIsinData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + ISINFILE);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << isinList;
        qFile.close();
    }
}

QDataStream &operator<<(QDataStream &out, const sISINDATA &param)
{
    out << param.ISIN;
    out << param.ticker;
    out << param.name;
    out << param.sector;
    out << param.industry;
    out << param.lastUpdate;

    return out;
}

QDataStream &operator>>(QDataStream &in, sISINDATA &param)
{
    in >> param.ISIN;
    in >> param.ticker;
    in >> param.name;
    in >> param.sector;
    in >> param.industry;
    in >> param.lastUpdate;

    return in;
}

bool Database::copyDirectoryFiles(const QString &fromDir, const QString &toDir, const bool &coverFileIfExist, const bool &removeOldFiles)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists()){    /* if directory don't exists, build it */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    //foreach(QFileInfo fileInfo, fileInfoList)
    for(const QFileInfo &fileInfo : fileInfoList)
    {
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir())    /* if it is directory, copy recursively*/
        {
            if(!copyDirectoryFiles(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()),
                coverFileIfExist,
                removeOldFiles))
                return false;
        }
        else
        {
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName()))       /* if coverFileIfExist == true, remove old file in target dir */
            {
                targetDir.remove(fileInfo.fileName());
            }

            // files copy
            if(!QFile::copy(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName())))
            {
                    return false;
            }

            if(removeOldFiles && sourceDir.exists(fileInfo.fileName()))         /* if removeOldFiles == true, remove old file in source dir */
            {
                sourceDir.remove(fileInfo.fileName());
            }
        }
    }

    return true;
}
