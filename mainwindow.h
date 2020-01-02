#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressDialog>

#include "downloadmanager.h"
#include "database.h"
#include "global.h"
#include "degiro.h"
#include "screener.h"
#include "screenertab.h"


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
    void getData(const QByteArray data, QString statusCode);
    void parseOnlineParameters(const QByteArray data, QString statusCode);
    void loadOnlineParametersSlot();
    void setScreenerParamsSlot(QVector<sSCREENERPARAM> params);
    void loadDegiroCSVslot();
    void setStatus(QString text);
    void setFilterSlot(QVector<sFILTER> list);
    void updateExchangeRates(const QByteArray data, QString statusCode);
    void fillOverview();

private slots:   
    void on_actionAbout_triggered();
    void on_actionHelp_triggered();
    void on_actionSettings_triggered();
    void on_actionExit_triggered();
    void on_pbAddTicker_clicked();
    void on_pbNewScreener_clicked();
    void on_pbDeleteScreener_clicked();
    void on_pbDegiroLoad_clicked();

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
    std::shared_ptr<Screener> screener;

    QVector<ScreenerTab*> screenerTabs;

    QProgressDialog *progressDialog;

    /**
     * @brief temporaryLoadedTable
     * @details sTABLE for last loaded ticker
     */
    sTABLE temporaryLoadedTable;

    int currentScreenerIndex;
    QStringList currentTickers;

    eSCREENSOURCE lastRequestSource;

    QVector<sFILTER> filterList;


    void centerAndResize();

    void setDegiroHeader();
    void fillDegiroCSV();

    void setScreenerHeader(ScreenerTab *st);
    void dataLoaded();
    int findScreenerTicker(QString ticker);
    void insertScreenerRow(TickerDataType tickerData);
    void fillScreener(ScreenerTab *st);
    void applyFilter(ScreenerTab *st);
    int applyFilterOnItem(ScreenerTab *st, QTableWidgetItem *item, sFILTER filter);
    QVector<sPDFEXPORT> prepareDataToExport();
};

#endif // MAINWINDOW_H
