#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressDialog>

#include "downloadmanager.h"
#include "database.h"
#include "global.h"
#include "degiro.h"
#include "screener.h"


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
    void setScreenerParamsSlot(QList<sSCREENERPARAM> params);
    void loadDegiroCSVslot();
    void setStatus(QString text);
    void setFilterSlot(QVector<sFILTER> list);

private slots:
    void on_actionAbout_triggered();
    void on_actionHelp_triggered();
    void on_actionSettings_triggered();
    void on_actionExit_triggered();
    void on_pbAddTicker_clicked();
    void on_pbLeftScreener_clicked();
    void on_pbRightScreener_clicked();
    void on_pbNewScreener_clicked();
    void on_pbDeleteScreener_clicked();
    void on_pbDegiroLoad_clicked();

    void on_pbFilter_clicked();
    void on_cbFilter_clicked(bool checked);
    void on_pbRefresh_clicked();

    void refreshTickersSlot(QString ticker = QString());
    void refreshTickersCanceled();
    void on_pbDeleteTickers_clicked();

signals:
    void updateScreenerParams(QList<sSCREENERPARAM> params);
    void refreshTickers(QString ticker);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    Ui::MainWindow *ui;    

    std::shared_ptr<DownloadManager> manager;
    std::shared_ptr<Database> database;
    std::shared_ptr<DeGiro> degiro;
    std::shared_ptr<Screener> screener;

    QProgressDialog *progressDialog;

    /**
     * @brief temporaryLoadedTable
     * @details sTABLE for last loaded ticker
     */
    sTABLE temporaryLoadedTable;

    sSCREENER currentScreenerData;
    int currentScreenerIndex;
    QStringList currentTickers;

    eSCREENSOURCE lastRequestSource;

    QVector<sFILTER> filterList;


    void centerAndResize();

    void dataLoaded();
    int findScreenerTicker(QString ticker);
    void setScreenerHeader();
    void fillScreener();
    void fillDegiro();
    void insertScreenerRow(tickerDataType ticerData);
    void setDegiroHeader();
};

#endif // MAINWINDOW_H
