#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressDialog>
#include <QPointer>

#include "downloadmanager.h"
#include "database.h"
#include "global.h"
#include "degiro.h"
#include "screener.h"
#include "screenertab.h"
#include "stockdata.h"
#include "tastyworks.h"


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


public Q_SLOTS:
    void checkVersion(const QByteArray data, QString statusCode);
    void getData(const QByteArray data, QString statusCode);
    void parseOnlineParameters(const QByteArray data, QString statusCode);
    void loadOnlineParametersSlot();
    void setScreenerParamsSlot(QVector<sSCREENERPARAM> params);
    void loadDegiroCSVslot();
    void loadTastyworksCSVslot();
    void setStatus(QString text);
    void setFilterSlot(QVector<sFILTER> list);
    void updateExchangeRates(const QByteArray data, QString statusCode);
    void setDegiroDataSlot(StockDataType newStockData);
    void fillOverviewSlot();
    void addRecord(const QByteArray data, QString statusCode);
    void fillOverviewTable();

private slots:
    void on_actionAbout_triggered();
    void on_actionHelp_triggered();
    void on_actionSettings_triggered();
    void on_actionExit_triggered();
    void on_pbAddTicker_clicked();
    void on_pbNewScreener_clicked();
    void on_pbDeleteScreener_clicked();

    void on_pbFilter_clicked();
    void on_cbFilter_clicked(bool checked);
    void on_pbRefresh_clicked();

    void refreshTickersSlot(QString ticker = QString());
    void refreshTickersCanceled();
    void on_pbDeleteTickers_clicked();

    void clickedScreenerTabSlot(int index);
    void on_mainTab_currentChanged(int index);

    void on_actionAbout_Qt_triggered();
    void on_pbAlert_clicked();
    void on_pbShowGraph_clicked();
    void on_pbPDFExport_clicked();
    void on_deGraphYear_userDateChanged(const QDate &date);
    void deOverviewYearChanged(const QDate &date);
    void on_dePDFYear_userDateChanged(const QDate &date);
    void on_pbAddRecord_clicked();

    void on_pbISINAdd_clicked();

    void on_tableISIN_cellDoubleClicked(int row, int column);

    void on_deOverviewFrom_userDateChanged(const QDate &date);

    void on_deOverviewTo_userDateChanged(const QDate &date);

    void on_tableOverview_cellDoubleClicked(int row, int column);

    void on_actionCheck_version_triggered();

signals:
    void updateScreenerParams(QVector<sSCREENERPARAM> params);
    void refreshTickers(QString ticker);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;    

    std::shared_ptr<DownloadManager> manager;
    std::shared_ptr<Database> database;
    std::shared_ptr<DeGiro> degiro;
    std::shared_ptr<Tastyworks> tastyworks;
    std::shared_ptr<Screener> screener;
    std::shared_ptr<StockData> stockData;

    QVector<ScreenerTab*> screenerTabs;

    /**
     * @brief temporaryLoadedTable
     * @details sTABLE for last loaded ticker
     */
    sTABLE temporaryLoadedTable;

    int currentScreenerIndex;
    QStringList currentTickers;

    eSCREENSOURCE lastRequestSource;

    QVector<sFILTER> filterList;

    /**
     * @brief lastRecord
     * @details last manualy added record
     */
    sNEWRECORD lastRecord;

    QPointer<QProgressDialog> progressDialog;

    void centerAndResize();

    /*
     *  Overview tab
     */
    void setOverviewHeader();
    QVector<sPDFEXPORT> prepareDataToExport();

    /**
     * @brief updateStockDataVector - set new "vector" for the specified ISIN
     * @param ISIN -
     * @param vector - vector to be replaced
     * @return true if ISIN exists or false
     */
    bool updateStockDataVector(QString ISIN, QVector<sSTOCKDATA> vector);

    /*
     *  DeGiro tab
     */
    /**
     * @brief setDegiroHeader - set the header label for the DeGiro table
     */
    void setDegiroHeader();

    /**
     * @brief fillDegiroTable - fill the DeGiro table with the content (raw data)
     */
    void fillDegiroTable();

    /*
     *  Screener tab
     */
    void setScreenerHeader(ScreenerTab *st);
    void dataLoaded();
    int findScreenerTicker(QString ticker);
    void insertScreenerRow(TickerDataType tickerData);
    void fillScreenerTable(ScreenerTab *st);
    void applyFilter(ScreenerTab *st);
    int applyFilterOnItem(ScreenerTab *st, QTableWidgetItem *item, sFILTER filter);

    /*
     *  ISIN tab
     */
    /**
     * @brief setISINHeader - set the header label for the ISIN table
     */
    void setISINHeader();

    /**
     * @brief fillISINTable - fill the ISIN table with the content
     */
    void fillISINTable();

    /**
     * @brief eraseISIN - erase record from the ISIN list
     * @param ISIN - ISIN to be removed
     */
    void eraseISIN(QString ISIN);
    void createProgressDialog(int min, int max);
};

#endif // MAINWINDOW_H
