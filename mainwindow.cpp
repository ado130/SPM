#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsform.h"

#include "filterform.h"

#include <QDebug>
#include <QDesktopWidget>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QScreen>
#include <QTableWidgetItem>
#include <QTimer>
#include <QPageSize>
#include <QPrinter>
#include <QtCharts>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << QSslSocket::supportsSsl() << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::sslLibraryVersionString();

    if (!QSslSocket::supportsSsl())
    {
        QMessageBox::critical(this,
                              "SSL supports",
                              "The application does not support the SSL, the application might not work correct!",
                              QMessageBox::Ok);
    }

    manager = std::make_unique<DownloadManager> (this);
    database = std::make_unique<Database> (this);
    degiro = std::make_unique<DeGiro> (database->getSetting(), this);
    tastyworks = std::make_unique<Tastyworks> (this);
    screener = std::make_unique<Screener> (this);
    stockData = std::make_unique<StockData> (this);
    calculation = std::make_unique<Calculation> (database.get(), stockData.get(), this);
    refreshProgressDlg = nullptr;

    connect(degiro.get(), &DeGiro::setDegiroData, this, &MainWindow::setDegiroDataSlot);

    connect(stockData.get(), &StockData::updateStockData, this, &MainWindow::updateStockDataSlot);
    stockData->loadOnlineStockInfo();

    /********************************
     * Geometry
    ********************************/
    if (database->getSetting().width <= 0 || database->getSetting().height <= 0 || database->getSetting().xPos <= 0 || database->getSetting().yPos <= 0)
    {
        centerAndResize();

        sSETTINGS set = database->getSetting();
        set.width = this->geometry().width();
        set.height = this->geometry().height();
        database->setSettingSlot(set);
    }
    else
    {
        QSize newSize( database->getSetting().width, database->getSetting().height);

        QPoint point = this->mapFromGlobal(QPoint(database->getSetting().xPos, database->getSetting().yPos));

        setGeometry(point.x(),
                    point.y(),
                    database->getSetting().width,
                    database->getSetting().height);
    }

    // set status bar text
    QLabel *author = new QLabel("Author: Andrej © 2019-2020", this);
    QLabel *version = new QLabel(QString("Version: %1").arg(VERSION_STR), this);
    QLabel *built = new QLabel(QString("Built: %1 %2").arg(__DATE__).arg(__TIME__), this);
    ui->statusBar->addPermanentWidget(author, 1);
    ui->statusBar->addPermanentWidget(version, 0);
    ui->statusBar->addPermanentWidget(built, 0);


    // PDF export default data
    ui->lePDFEUR2CZK->setText(QString::number(database->getSetting().EUR2CZKDAP, 'f', 2));
    ui->lePDFUSD2CZK->setText(QString::number(database->getSetting().USD2CZKDAP, 'f', 2));
    ui->lePDFGBP2CZK->setText(QString::number(database->getSetting().GBP2CZKDAP, 'f', 2));

    // default graph date time values
    ui->deGraphTo->setDate(QDate::currentDate());
    ui->deGraphFrom->setDate(QDate(QDate::currentDate().year(), 1, 1));
    ui->deGraphYear->setDate(QDate::currentDate());

    ui->dePDFTo->setDate(QDate::currentDate());
    ui->dePDFFrom->setDate(QDate(QDate::currentDate().year(), 1, 1));
    ui->dePDFYear->setDate(QDate::currentDate());

    ui->deOverviewTo->setDate(database->getSetting().lastOverviewTo);
    ui->deOverviewFrom->setDate(database->getSetting().lastOverviewFrom);
    ui->deOverviewYear->setDate(QDate::currentDate());

    connect(
        ui->deOverviewYear, &QDateEdit::userDateChanged,
        [=]( ) { deOverviewYearChanged(ui->deOverviewYear->date()); }
        );

    connect(
        ui->deOverviewTo, &QDateEdit::userDateChanged,
        [=]( ) { fillOverviewTable(); fillOverviewSlot(); }
        );

    connect(
        ui->deOverviewFrom, &QDateEdit::userDateChanged,
        [=]( ) { fillOverviewTable(); fillOverviewSlot(); }
        );


    ui->mainTab->setCurrentIndex(database->getSetting().lastOpenedTab);

    // Update the exchange rates
    if (database->getSetting().lastExchangeRatesUpdate < QDate::currentDate())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::updateExchangeRates);
        manager.get()->execute("https://api.exchangeratesapi.io/latest?base=USD&symbols=EUR,CZK,GBP");
    }

    /********************************
     * Overview
    ********************************/
    setOverviewHeader();
    fillOverviewTable();
    fillOverviewSlot();

    /********************************
     * DeGiro table
    ********************************/
    setDegiroHeader();

    if (database->getSetting().degiroAutoLoad && degiro->getIsRAWFile())
    {
        fillDegiroTable();
    }


    /********************************
     * ISIN table
    ********************************/
    setISINHeader();
    fillISINTable();


    /********************************
     * Screener table
    ********************************/
    ui->cbFilter->setChecked(database->getSetting().filterON);
    ui->pbFilter->setEnabled(database->getSetting().filterON);

    filterList = database->getFilterList();

    currentScreenerIndex = database->getLastScreenerIndex();

    if (currentScreenerIndex > -1)
    {
        QVector<sSCREENER> allData = screener->getAllScreenerData();

        if (allData.count() > currentScreenerIndex)
        {
            for (int a = 0; a<allData.count(); ++a)
            {
                ScreenerTab *st = new ScreenerTab(this);
                st->setScreenerData(allData.at(a));
                screenerTabs.push_back(st);

                ui->tabScreener->addTab(st, allData.at(a).screenerName);

                setScreenerHeader(st);
                fillScreenerTable(st);
            }

            ui->tabScreener->setCurrentIndex(currentScreenerIndex);
        }
        else
        {
            currentScreenerIndex = allData.count() - 1;
        }
    }

    connect(ui->tabScreener, &QTabWidget::currentChanged, this, &MainWindow::clickedScreenerTabSlot);
    database->setLastScreenerIndex(currentScreenerIndex);


    if (database->getEnabledScreenerParams().count() == 0 || screenerTabs.count() == 0)
    {
        ui->pbAddTicker->setEnabled(false);
    }

    if (database->getSetting().screenerAutoLoad)
    {
        ui->pbRefresh->click();
    }


    ui->leTicker->installEventFilter(this);
    this->installEventFilter(this);
}

void MainWindow::centerAndResize()
{
    // get the dimension available on this screen
    QSize availableSize = QGuiApplication::screens().first()->size();

    int width = availableSize.width();
    int height = availableSize.height();

    qDebug() << "Available dimensions " << width << "x" << height;

    width = static_cast<int>(width*0.75);
    height = static_cast<int>(height*0.75);

    qDebug() << "Computed dimensions " << width << "x" << height;

    QSize newSize( width, height );

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            newSize,
            QGuiApplication::screens().first()->availableGeometry()
        )
    );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_mainTab_currentChanged(int index)
{
    sSETTINGS set = database->getSetting();
    set.lastOpenedTab = index;
    database->setSettingSlot(set);
}


void MainWindow::on_actionAbout_triggered()
{
    QString text;
    text =  "<html><body><center>";
    text += "<h2>Stock Portfolio Manager (SPM)</h2>";
    text += "<b>Author:</b> Andrej<br>";
    text += "<b>E-mail:</b> <a href=\"mailto:vlasaty.andrej@gmail.com?subject=SPM\">vlasaty.andrej@gmail.com</a><br>";
    text += "<b>Website:</b> <a href=\"http://ado.4fan.cz/SPM/web/\">http://ado.4fan.cz/SPM/web/</a><br>";
    text += "<b>Website:</b> <a href=\"https://www.investicnigramotnost.cz\">https://www.investicnigramotnost.cz</a><br>";
    text += "==================================<br>";
    text += "<b>Icons8:</b> <a href=\"https://icons8.com\">https://icons8.com</a><br>";
    text += "<b>Exchange rates source:</b> <a href=\"https://exchangeratesapi.io\">https://exchangeratesapi.io</a><br>";
    text += "Exchange rates are updated once per day<br>";
    text += "The PDF export file is valid only for Czech republic<br>";
    text += "<br><br>";
    text += "<a href=\"https://www.paypal.me/vandrej\"> <img border=\"0\" alt=\"Donate\" src=\":/images/donate.gif\" width=\"147\" height=\"47\"> </a><br>";
    text += "If you like this tool you can donate me and together we can make this tool better.<br>";
    text += "Any amount is greatly appreciated.<br>";
    text += "If you have any question or suggestion, feel free to contact me.<br>";
    text += "<br><br>";
    text += "<b>Copyright © 2019 Stock Portfolio Manager</b>";
    text += "</center></body></html>";

    QMessageBox::about(this,
                       "About",
                       text);
}

void MainWindow::on_actionHelp_triggered()
{
    QString text;
    text =  "<html><body>";
    text += "Set the CSV path to the DeGiro file and delimeter in the Settings.<br>";
    text += "Please load the parameters under the Settings window and select the order and the visibility.<br><br>";
    text += "The filter window allows to set one of the predefined filter:";
    text += "<ul>";
    text += "<li> f&lt; </li>";
    text += "<li> f&gt; </li>";
    text += "<li> &lt;f;f&gt; </li>";
    text += "</ul>";
    text += "where the \"f\" means float number.<br><br>";
    text += "The color column has to be either HEX color number or \"HIDE\".<br>";
    text += "The color HEX palette is available under the context menu (right click).<br>";
    text += "<br>";
    text += "Double click on the row in the Overview table opens selected stock details.";
    text += "</body></html>";

    QMessageBox::about(this,
                       "Help",
                       text);
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsForm *dlg = new SettingsForm(database->getSetting(), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, SIGNAL(setSetting(sSETTINGS)), database.get(), SLOT(setSettingSlot(sSETTINGS)));
    connect(dlg, &SettingsForm::setScreenerParams, this, &MainWindow::setScreenerParamsSlot);
    connect(dlg, &SettingsForm::loadOnlineParameters, this, &MainWindow::loadOnlineParametersSlot);
    connect(dlg, &SettingsForm::loadDegiroCSV, this, &MainWindow::loadDegiroCSVslot);
    connect(dlg, &SettingsForm::loadTastyworksCSV, this, &MainWindow::loadTastyworksCSVslot);
    connect(dlg, &SettingsForm::fillOverview, this, &MainWindow::fillOverviewSlot);
    connect(dlg, &SettingsForm::fillOverview, this, &MainWindow::fillOverviewTable);
    connect(this, &MainWindow::updateScreenerParams, dlg, &SettingsForm::updateScreenerParamsSlot);
    dlg->open();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, "SPM");
}


void MainWindow::on_actionExit_triggered()
{
    exit(0);
}

void MainWindow::setStatus(QString text)
{
    ui->leStatus->setText(text);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);

        if ( (key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return) )
        {
            if (obj == ui->leTicker)
            {
                if (!ui->leTicker->text().isEmpty())
                {
                    ui->pbAddTicker->click();
                }
                else
                {
                    setStatus("The ticker is empty!");
                }
            }
        }
        else
        {
            return QObject::eventFilter(obj, event);
        }
        return true;
    }
    else if (event->type() == QEvent::Resize)
    {
        sSETTINGS set = database->getSetting();
        set.width = this->geometry().width();
        set.height = this->geometry().height();
        database->setSettingSlot(set);

        if (refreshProgressDlg)
        {
            QPoint p = mapToGlobal(QPoint(size().width(), size().height())) -
                       QPoint(refreshProgressDlg->size().width(), refreshProgressDlg->size().height());
            refreshProgressDlg->move(p);
        }

        return true;
    }
    else if (event->type() == QEvent::Move)
    {
        sSETTINGS set = database->getSetting();
        QPoint point = this->mapToGlobal(QPoint(0, 0));
        set.xPos = point.x();
        set.yPos = point.y();
        database->setSettingSlot(set);

        if (refreshProgressDlg)
        {
            QPoint p = mapToGlobal(QPoint(size().width(), size().height())) -
                       QPoint(refreshProgressDlg->size().width(), refreshProgressDlg->size().height());
            refreshProgressDlg->move(p);
        }

        return true;
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

void MainWindow::updateExchangeRates(const QByteArray data, QString statusCode)
{
    disconnect(manager.get(), &DownloadManager::sendData, this, &MainWindow::updateExchangeRates);

    if (!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the update exchange rates request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the update exchange rates request! %1").arg(statusCode));
        QApplication::restoreOverrideCursor();
    }
    else
    {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error == QJsonParseError::NoError)
        {
            QJsonObject jsonObject = jsonDoc.object();

            if (jsonObject["base"].toString() == "USD")
            {
                QJsonObject rates = jsonObject["rates"].toObject();

                sSETTINGS set = database->getSetting();
                set.USD2CZK = rates["CZK"].toDouble();
                set.USD2EUR = rates["EUR"].toDouble();
                set.USD2GBP = rates["GBP"].toDouble();
                database->setSettingSlot(set);

                connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::updateExchangeRates);
                manager.get()->execute("https://api.exchangeratesapi.io/latest?base=EUR&symbols=USD,CZK,GBP");
            }
            else if (jsonObject["base"].toString() == "EUR")
            {
                QJsonObject rates = jsonObject["rates"].toObject();

                sSETTINGS set = database->getSetting();
                set.EUR2USD = rates["USD"].toDouble();
                set.EUR2CZK = rates["CZK"].toDouble();
                set.EUR2GBP = rates["GBP"].toDouble();
                database->setSettingSlot(set);


                connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::updateExchangeRates);
                manager.get()->execute("https://api.exchangeratesapi.io/latest?base=CZK&symbols=USD,EUR,GBP");
            }
            else if (jsonObject["base"].toString() == "CZK")
            {
                QJsonObject rates = jsonObject["rates"].toObject();

                sSETTINGS set = database->getSetting();
                set.CZK2USD = rates["USD"].toDouble();
                set.CZK2EUR = rates["EUR"].toDouble();
                set.CZK2GBP = rates["GBP"].toDouble();
                set.lastExchangeRatesUpdate = QDate::currentDate();
                database->setSettingSlot(set);

                setStatus("The exchange rates have been updated!");
                QApplication::restoreOverrideCursor();
            }
        }
        else
        {
            QApplication::restoreOverrideCursor();
            setStatus(QString("The exchange rates have not been updated because of the following error: %1: %2").arg(error.error).arg(error.errorString()));
        }
    }
}

void MainWindow::on_actionCheck_version_triggered()
{
    connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::checkVersion);
    manager.get()->execute("http://ado.4fan.cz/SPM/version.txt");
}

void MainWindow::checkVersion(const QByteArray data, QString statusCode)
{
    disconnect(manager.get(), &DownloadManager::sendData, this, &MainWindow::checkVersion);

    if (!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the check version request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the check version request! %1").arg(statusCode));
    }
    else
    {
        QString input = QString(data);

        if (input.startsWith("Version:"))
        {
            int start = input.indexOf(":");
            QString version = input.mid(start+1);

            setStatus(QString("Installed version: %1; Latest version: %2").arg(VERSION_STR).arg(version));
        }
    }
}

/********************************
*
*  OVERVIEW
*
********************************/

void MainWindow::setOverviewHeader()
{
    QStringList header;
    header << "ISIN" << "Ticker" << "Name" << "Sector" << "%" << "Count" << "Average price" << "Total price" << "Fees" << "Current price" << "Total current price" << "Netto dividend";
    ui->tableOverview->setColumnCount(header.count());

    ui->tableOverview->setRowCount(0);

    ui->tableOverview->horizontalHeader()->setVisible(true);
    ui->tableOverview->verticalHeader()->setVisible(true);

    ui->tableOverview->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableOverview->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableOverview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableOverview->setShowGrid(true);

    ui->tableOverview->setHorizontalHeaderLabels(header);
}

void MainWindow::fillOverviewTable()
{
    QDate from = ui->deOverviewFrom->date();
    QDate to = ui->deOverviewTo->date();

    QVector<sOVERVIEWTABLE> table = calculation->getOverviewTable(from, to);

    if (table.isEmpty())
    {
        return;
    }

    QString currencySign = database->getCurrencySign(database->getSetting().currency);


    int pos = 0;

    ui->tableOverview->setRowCount(0);
    ui->tableOverview->setSortingEnabled(false);

    for (const sOVERVIEWTABLE &row : qAsConst(table))
    {
        ui->tableOverview->insertRow(pos);

        ui->tableOverview->setItem(pos, 0, new QTableWidgetItem(row.ISIN));
        ui->tableOverview->setItem(pos, 1, new QTableWidgetItem(row.ticker));
        ui->tableOverview->setItem(pos, 2, new QTableWidgetItem(row.stockName));
        ui->tableOverview->setItem(pos, 3, new QTableWidgetItem(row.sector));
        ui->tableOverview->setItem(pos, 4, new QTableWidgetItem(QString("%L1 %").arg(row.percentage, 0, 'f', 2)));
        ui->tableOverview->setItem(pos, 5, new QTableWidgetItem(QString::number(row.totalCount)));
        ui->tableOverview->setItem(pos, 6, new QTableWidgetItem(QString("%L1").arg(row.averageBuyPrice, 0, 'f', 2) + " " + currencySign));
        ui->tableOverview->setItem(pos, 7, new QTableWidgetItem(QString("%L1").arg(row.totalStockPrice, 0, 'f', 2) + " " + currencySign));
        ui->tableOverview->setItem(pos, 8, new QTableWidgetItem(QString("%L1").arg(row.totalFee, 0, 'f', 2) + " " + currencySign));
        ui->tableOverview->setItem(pos, 9, new QTableWidgetItem(QString("%L1").arg(row.onlineStockPrice, 0, 'f', 2) + " " + currencySign));
        ui->tableOverview->setItem(pos, 10, new QTableWidgetItem(QString("%L1").arg(row.totalOnlinePrice, 0, 'f', 2) + " " + currencySign));
        ui->tableOverview->setItem(pos, 11, new QTableWidgetItem(QString("%L1").arg(row.dividend, 0, 'f', 2) + " " + currencySign));

        if (row.totalOnlinePrice > row.totalStockPrice)
        {
            ui->tableOverview->item(pos, 10)->setBackground(QColor(Qt::green));
        }
        else if (row.totalOnlinePrice < row.totalStockPrice)
        {
            ui->tableOverview->item(pos, 10)->setBackground(QColor(Qt::red));
        }

        if (row.averageBuyPrice < row.onlineStockPrice)
        {
            ui->tableOverview->item(pos, 6)->setBackground(QColor(Qt::green));
        }
        else if (row.averageBuyPrice > row.onlineStockPrice)
        {
            ui->tableOverview->item(pos, 6)->setBackground(QColor(Qt::red));
        }

        pos++;
    }
    ui->tableOverview->setSortingEnabled(true);


    for (int row = 0; row<ui->tableOverview->rowCount(); ++row)
    {
        for (int col = 0; col<ui->tableOverview->columnCount(); ++col)
        {
            ui->tableOverview->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->tableOverview->resizeColumnsToContents();

    for (int col = 0; col < ui->tableOverview->horizontalHeader()->count(); ++col)
    {
        if (col == 2)
        {
            ui->tableOverview->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
        }
    }
}

void MainWindow::on_tableOverview_cellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)

    QTableWidgetItem *rowItem = ui->tableOverview->item(row, 0);

    if (!rowItem) return;

    QString ISIN = rowItem->text();

    StockDataType stockList = stockData->getStockData();
    QVector<sSTOCKDATA> vector = stockList.value(ISIN);

    if (vector.count() == 0) return;

    QDialog *stockDlg = new QDialog(this);
    stockDlg->setAttribute(Qt::WA_DeleteOnClose);
    stockDlg->setWindowTitle("SPM " + vector.at(0).stockName);


    QTableWidget *table = new QTableWidget(stockDlg);
    table->setStyleSheet(""
                         "QTableWidget {"
                         "  border: 2px solid #8f8f91;"
                         "  border-radius: 6px;"
                         "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde);"
                         "  selection-background-color: gray;"
                         "  alternate-background-color: #dadbde;\n"
                         "}"
                         "");


    QStringList header;
    header << "Date" << "Type" << "Count" << "Price" << "Fee" << "Delete";
    table->setColumnCount(header.count());

    table->setRowCount(0);
    table->setColumnCount(header.count());

    table->horizontalHeader()->setVisible(true);
    table->verticalHeader()->setVisible(false);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(true);

    table->setHorizontalHeaderLabels(header);

    eCURRENCY selectedCurrency = database->getSetting().currency;
    QString currencySign = database->getCurrencySign(database->getSetting().currency);

    int pos = 0;
    table->setSortingEnabled(false);

    for (const sSTOCKDATA &stock : qAsConst(vector))
    {
        if (stock.type == CURRENCYEXCHANGE)
        {
            continue;
        }

        table->insertRow(pos);

        QTableWidgetItem *item = new QTableWidgetItem;
        item->setData(Qt::EditRole, stock.dateTime.date());
        table->setItem(pos, 0, item);


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
        double fee = 0.0;

        price = database->getExchangePrice(rates, stock.price) * stock.count;
        fee = database->getExchangePrice(rates, stock.fee);

        switch(stock.type)
        {
            case DEPOSIT: table->setItem(pos, 1, new QTableWidgetItem("Deposit"));
                break;
            case BUY: table->setItem(pos, 1, new QTableWidgetItem("Buy"));
                break;
            case SELL: table->setItem(pos, 1, new QTableWidgetItem("Sell"));
                break;
            case DIVIDEND: table->setItem(pos, 1, new QTableWidgetItem("Dividend"));
                break;
            case TAX: table->setItem(pos, 1, new QTableWidgetItem("Tax"));
                break;
            case FEE: table->setItem(pos, 1, new QTableWidgetItem("Fee"));
                break;
            case TRANSACTIONFEE: table->setItem(pos, 1, new QTableWidgetItem("Transaction fee"));
                break;
            case WITHDRAWAL: table->setItem(pos, 1, new QTableWidgetItem("Withdrawal"));
                break;
        }

        table->setItem(pos, 2, new QTableWidgetItem(QString::number(stock.count)));
        table->setItem(pos, 3, new QTableWidgetItem(QString("%L1").arg(abs(price), 0, 'f', 2) + " " + currencySign));
        table->setItem(pos, 4, new QTableWidgetItem(QString("%L1").arg(abs(fee), 0, 'f', 2) + " " + currencySign));


        QPushButton *pbDelete = new QPushButton(table);
        pbDelete->setStyleSheet("QPushButton {border-image:url(:/images/delete.png);}");

        connect(pbDelete, &QPushButton::clicked, [table, ISIN, stockList, this]()
                {
                    int ret = QMessageBox::warning(nullptr,
                                                   "Delete record",
                                                   "Do you really want to delete the selected record?",
                                                   QMessageBox::Yes, QMessageBox::No);

                    if (ret == QMessageBox::Yes)
                    {
                        int rowToDelete = table->currentRow();

                        auto it = stockList.find(ISIN);

                        if (it != stockList.end())
                        {
                            QVector<sSTOCKDATA> vec = it.value();
                            vec.remove(rowToDelete);

                            if (stockData->updateStockDataVector(ISIN, vec))
                            {
                                table->removeRow(rowToDelete);
                            }
                        }
                    }
                } );

        table->setItem(pos, 5, new QTableWidgetItem());
        table->setCellWidget(pos, 5, pbDelete);

        pos++;
    }
    table->setSortingEnabled(true);

    for (int rowTable = 0; rowTable<table->rowCount(); ++rowTable)
    {
        for (int colTable = 0; colTable<table->columnCount()-1; ++colTable)
        {
            table->item(rowTable, colTable)->setTextAlignment(Qt::AlignCenter);
        }
    }

    table->resizeColumnsToContents();

    //QHeaderView will automatically resize the section to fill the available space. The size cannot be changed by the user or programmatically.
    for (int col = 0; col < table->horizontalHeader()->count(); ++col)
    {
        if (col == 0 || col == 5)
        {
            continue;
        }

        table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
    }


    if (table->rowCount() > 5)
    {
        table->setMinimumHeight(table->rowHeight(0)*5);
    }
    else
    {
        table->setMinimumHeight(table->rowHeight(0)*table->rowCount()+30);
    }

    int tableWidth = 0;

    for (int colTable = 0; colTable<table->columnCount(); ++colTable)
    {
        tableWidth += table->columnWidth(colTable);
    }

    table->setMinimumWidth(tableWidth);

    QVBoxLayout *VB = new QVBoxLayout(stockDlg);

    QFont font("Helvetica", 10);
    font.setBold(true);

    QHBoxLayout *HBStockName = new QHBoxLayout();
    QLabel *labelName = new QLabel(QString("%1").arg(vector.first().stockName));
    labelName->setAlignment(Qt::AlignCenter);
    labelName->setFont(font);
    HBStockName->addWidget(labelName);
    VB->addLayout(HBStockName);

    QHBoxLayout *HBIsin = new QHBoxLayout();
    QLabel *labelIsin = new QLabel(QString("%1 (%2)").arg(vector.first().ticker).arg(vector.first().ISIN));
    labelIsin->setAlignment(Qt::AlignCenter);
    HBIsin->addWidget(labelIsin);
    VB->addLayout(HBIsin);

    QHBoxLayout *HBTable = new QHBoxLayout();
    HBTable->addWidget(table);
    VB->addLayout(HBTable);


    /************************
    * CHART
    ************************/
    QChartView *chartView = calculation->getChartView(ISINCHART, vector.last().dateTime.date(), vector.first().dateTime.date(), ISIN);

    if (chartView != nullptr)
    {
        chartView->setRubberBand(QChartView::NoRubberBand);

        chartView->chart()->axes(Qt::Horizontal).first()->setTitleText(QString("Years"));
        chartView->chart()->axes(Qt::Vertical).first()->setTitleText(QString("Dividends (%1)").arg(currencySign));

        QHBoxLayout *HBChart = new QHBoxLayout();
        HBChart->addWidget(chartView);
        VB->addLayout(HBChart);
    }

    stockDlg->setLayout(VB);

    if (chartView != nullptr)
    {
        stockDlg->resize(table->horizontalHeader()->length()+table->verticalScrollBar()->width(),
                        table->verticalHeader()->length()+table->horizontalHeader()->height()+table->horizontalScrollBar()->height()+chartView->size().height());
    }
    else
    {
        stockDlg->resize(table->horizontalHeader()->length()+table->verticalScrollBar()->width(),
                         table->verticalHeader()->length()+table->horizontalHeader()->height()+table->horizontalScrollBar()->height());
    }

    stockDlg->open();
}


void MainWindow::fillOverviewSlot()
{
    QDate from = ui->deOverviewFrom->date();
    QDate to = ui->deOverviewTo->date();
    sOVERVIEWINFO info = calculation->getOverviewInfo(from, to);

    QString currencySign = database->getCurrencySign(database->getSetting().currency);

    double deposit = abs(info.deposit);
    double invested = abs(info.invested);
    double dividends = abs(info.dividends);
    double divTax = abs(info.divTax);
    double fees = abs(info.fees);
    double transFees = abs(info.transFees);
    double withdrawal = abs(info.withdrawal);
    double sell = abs(info.sell);

    ui->leDeposit->setText(QString("%L1").arg(deposit, 0, 'f', 2) + " " + currencySign);
    ui->leInvested->setText(QString("%L1").arg(invested, 0, 'f', 2) + " " + currencySign);
    ui->leDividends->setText(QString("%L1").arg(dividends, 0, 'f', 2) + " " + currencySign);
    ui->leDivTax->setText(QString("%L1").arg(divTax, 0, 'f', 2) + " " + currencySign);
    ui->leDY->setText(QString("%L1").arg(info.DY, 0, 'f', 2) + " %");
    ui->leFees->setText(QString("%L1").arg(fees, 0, 'f', 2) + " " + currencySign);
    ui->leTransactionFee->setText(QString("%L1").arg(transFees, 0, 'f', 2) + " " + currencySign);
    ui->leWithdrawal->setText(QString("%L1").arg(withdrawal, 0, 'f', 2) + " " + currencySign);
    ui->leAccount->setText(QString("%L1").arg(info.account, 0, 'f', 2) + " " + currencySign);
    ui->leSell->setText(QString("%L1").arg(sell, 0, 'f', 2) + " " + currencySign);
    ui->lePortfolio->setText(QString("%L1").arg(info.portfolio, 0, 'f', 2) + " " + currencySign);
    ui->lePerformance->setText(QString("%L1 %").arg(info.performance, 0, 'f', 2));
}

void MainWindow::on_pbShowGraph_clicked()
{
    QDate from = ui->deGraphFrom->date();
    QDate to = ui->deGraphTo->date();
    eCHARTTYPE type = static_cast<eCHARTTYPE>(ui->cmGraphType->currentIndex());

    QChartView *chartView = calculation->getChartView(type, from, to);

    if (chartView == nullptr)
    {
        return;
    }



    QWidget *chartWidget = new QWidget(this, Qt::Tool);

    QVBoxLayout *VB = new QVBoxLayout(chartWidget);
    VB->addWidget(chartView);

    if (type != SECTORCHART && type != STOCKCHART)
    {
        QPushButton *zoomReset = new QPushButton("Zoom reset", chartWidget);
        connect(zoomReset, &QPushButton::clicked, [chartView]( )
                {
                    chartView->chart()->zoomReset();
                }
                );

        QHBoxLayout *HB = new QHBoxLayout();
        HB->addWidget(zoomReset);
        VB->addLayout(HB);
    }        

    QPushButton *makeImageBtn = new QPushButton("Save PNG", chartWidget);
    connect(makeImageBtn, &QPushButton::clicked, [chartView, chartWidget]()
            {
                QString fileName = QFileDialog::getSaveFileName(chartWidget,
                                                                "Save File",
                                                                QString(),
                                                                tr("PNG (*.png)"));
                if (!fileName.isEmpty())
                {
                    QPixmap p = chartView->grab();
                    QOpenGLWidget *glWidget  = chartView->findChild<QOpenGLWidget*>();

                    if (glWidget)
                    {
                        QPainter painter(&p);
                        QPoint d = glWidget->mapToGlobal(QPoint())-chartView->mapToGlobal(QPoint());
                        painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                        painter.drawImage(d, glWidget->grabFramebuffer());
                        painter.end();
                    }

                    p.save(fileName, "PNG");
                }
            }
            );

    QHBoxLayout *HB = new QHBoxLayout();
    HB->addWidget(makeImageBtn);
    VB->addLayout(HB);

    if (type == MONTHCOMPAREDIVDEND)
    {
        const MonthDividendDataType dividends = calculation->getMonthDividendData(from, to);

        QTableWidget *table = new QTableWidget(chartWidget);
        table->setStyleSheet(""
                             "QTableWidget {"
                             "  border: 2px solid #8f8f91;"
                             "  border-radius: 6px;"
                             "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde);"
                             "  selection-background-color: gray;"
                             "  alternate-background-color: #dadbde;\n"
                             "}"
                             "");


        QStringList header;
        header << "Month/Year";

        for(quint8 a = 0; a<dividends.keys().count(); ++a)
        {
            header << QString::number(dividends.keys().at(a));
        }

        table->setColumnCount(header.count());

        table->setRowCount(13);
        table->setColumnCount(header.count());

        table->horizontalHeader()->setVisible(true);
        table->verticalHeader()->setVisible(false);

        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setShowGrid(true);

        table->setHorizontalHeaderLabels(header);

        QString currencySign = database->getCurrencySign(database->getSetting().currency);

        table->setSortingEnabled(false);

        QList<int> divKeys = dividends.keys();

        QLocale locale;
        QDate tmpMonth = QDate(2020, 1, 1);

        for (quint8 row = 0; row<12; ++row)
        {
            table->setItem(row, 0, new QTableWidgetItem(locale.toString(tmpMonth, "MMMM")));
            tmpMonth = tmpMonth.addMonths(1);
        }

        table->setItem(12, 0, new QTableWidgetItem("Total"));

        int col = 1;

        for (const int &key : divKeys)
        {
            QVector<QPair<int, double>> vector = dividends.value(key);

            double total = 0.0;
            for (const auto &item : qAsConst(vector))
            {
                table->setItem(item.first-1, col, new QTableWidgetItem(currencySign + QString::number(item.second)));
                total += item.second;
            }

            table->setItem(12, col, new QTableWidgetItem(currencySign + QString::number(total)));
            col++;
        }
        table->setSortingEnabled(true);

        for (int rowTable = 0; rowTable<table->rowCount(); ++rowTable)
        {
            for (int colTable = 0; colTable<table->columnCount(); ++colTable)
            {
                table->item(rowTable, colTable)->setTextAlignment(Qt::AlignCenter);
            }
        }

        table->resizeColumnsToContents();

        table->setMinimumHeight(table->rowHeight(0)*14);

        int tableWidth = 0;

        for (int colTable = 0; colTable<table->columnCount(); ++colTable)
        {
            tableWidth += table->columnWidth(colTable);
        }

        table->setMinimumWidth(tableWidth);

        QHBoxLayout *HBTable = new QHBoxLayout();
        HBTable->addWidget(table);
        VB->addLayout(HBTable);
    }


    chartWidget->setLayout(VB);
    chartWidget->activateWindow();
    chartWidget->setParent(this);
    chartWidget->setAttribute(Qt::WA_DeleteOnClose);
    chartWidget->resize(1024, 768);
    chartWidget->setVisible(true);
}

void MainWindow::on_pbPDFExport_clicked()
{
    QString customText;
    bool okCustom = false;

    if (ui->cbPDFCustomText->isChecked())
    {
        customText = QInputDialog::getMultiLineText(this,
                                                    tr("PDF export"),
                                                    tr("Text:"),
                                                    "",
                                                    &okCustom);

        customText.replace("\n", "<br>");
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export PDF",
                                                    QString(),
                                                    "*.pdf");

    if (fileName.isEmpty()) return;

    if (QFileInfo(fileName).suffix().isEmpty())
    {
        fileName.append(".pdf");
    }

    const QDate from = ui->dePDFFrom->date();
    const QDate to = ui->dePDFTo->date();

    const double USD2CZK = ui->lePDFUSD2CZK->text().toDouble();
    const double EUR2CZK = ui->lePDFEUR2CZK->text().toDouble();
    const double GBP2CZK = ui->lePDFGBP2CZK->text().toDouble();
    QVector<sPDFEXPORTDATA> pdfData = stockData->prepareDataToExport(from, to, USD2CZK, EUR2CZK, GBP2CZK);

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setOutputFileName(fileName);

    QString text;
    text =  "<html>";
    text += "    <table align=\"center\" cellspacing=\"0\" cellpadding=\"2\" border=\"1\">";
    text += "       <tbody>";
    text += "           <tr>";
    text += "               <td colspan=\"4\" align=\"center\"><b>Export pro DAP</b></td>";
    text += "           </tr>";
    text += "           <tr>";
    text += "               <td align=\"left\"><b>Jméno:</b></td>";
    text += QString("       <td colspan=\"3\" align=\"center\">%1</td>").arg(ui->lePDFName->text());
    text += "           </tr>";
    text += "           <tr>";
    text += "               <td align=\"left\"><b>Datum:</b></td>";
    text += QString("       <td align=\"center\">%1</td>").arg(QDate::currentDate().toString("dd.MM.yyyy"));
    text += "               <td align=\"left\"><b>Počet:</b></td>";
    text += QString("       <td align=\"center\">%1</td>").arg(pdfData.count());
    text += "           </tr>";
    text += "       </tbody>";
    text += "   </table>";
    text += "<br><br>";
    text += "    <table width=\"100%\" align=\"center\" cellspacing=\"0\" cellpadding=\"1\" border=\"1\">";
    text += "       <tbody>";
    text += "           <tr>";
    text += "               <td align=\"center\" colspan=\"6\"><b>DAP tabulka dividenda</b></td>";
    text += "           </tr>";
    text += "           <tr>";
    text += "               <td align=\"center\"><b>Datum</b></td>";
    text += "               <td align=\"center\"><b>Zaplaceno<br>Měna výplaty</b></td>";
    text += "               <td align=\"center\"><b>Zaplaceno<br>CZK</b></td>";
    text += "               <td align=\"center\"><b>Daň<br>CZK</b></td>";
    text += "               <td align=\"center\"><b>Daň %</b></td>";
    text += "               <td align=\"center\"><b>Název</b></td>";
    text += "           </tr>";

    for (const sPDFEXPORTDATA &pdf : qAsConst(pdfData))
    {
        if (pdf.type != DIVIDEND)
        {
            continue;
        }

        text += "           <tr>";
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.date.toString("dd.MM.yyyy"));
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.priceInOriginal);
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.priceInCZK);
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.tax);
        text += QString("               <td align=\"center\">%1</td>").arg(QString::number( round((pdf.tax/pdf.priceInCZK) * 100.0), 'f', 0));
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.name);
        text += "           </tr>";
    }

    text += "       </tbody>";
    text += "   </table>";



    /*
    *       CP
    */
    text += "<br><br>";
    text += "    <table width=\"100%\" align=\"center\" cellspacing=\"0\" cellpadding=\"1\" border=\"1\">";
    text += "       <tbody>";
    text += "           <tr>";
    text += "               <td align=\"center\" colspan=\"4\"><b>DAP tabulka prodej CP</b></td>";
    text += "           </tr>";
    text += "           <tr>";
    text += "               <td align=\"center\"><b>Datum</b></td>";
    text += "               <td align=\"center\"><b>Zaplaceno<br>Měna výplaty</b></td>";
    text += "               <td align=\"center\"><b>Zaplaceno<br>CZK</b></td>";
    text += "               <td align=\"center\"><b>Název</b></td>";
    text += "           </tr>";

    double totalInCZK = 0.0;
    for (const sPDFEXPORTDATA &pdf : qAsConst(pdfData))
    {
        if (pdf.type != SELL)
        {
            continue;
        }

        totalInCZK += pdf.priceInCZK;

        text += "           <tr>";
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.date.toString("dd.MM.yyyy"));
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.priceInOriginal);
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.priceInCZK);
        text += QString("               <td align=\"center\">%1</td>").arg(pdf.name);
        text += "           </tr>";
    }

    text += "           <tr>";
    text += QString("               <td align=\"center\">Spolu: </td>");
    text += QString("               <td align=\"center\"> </td>");
    text += QString("               <td align=\"center\">%1</td>").arg(totalInCZK);
    text += QString("               <td align=\"center\"> </td>");
    text += "           </tr>";

    text += "       </tbody>";
    text += "   </table>";

    /*
    *       TEXT
    */
    text += "<br><br>";
    text += QString("Když je celková hodnota prodeje CP méně než 100 000, - Kč, neni třeba prodej uvádět do DP: §4 odst. 1 písm. w) Zákonu o dani.");
    text += "<br><br>";
    text += "<br><br>";
    text += QString("Přepočet měn z USD do CZK byl proveden jednotným kurzem: 1 USD = %1 CZK").arg(ui->lePDFUSD2CZK->text());
    text += "<br>";
    text += QString("Přepočet měn z EUR do CZK byl proveden jednotným kurzem: 1 EUR = %1 CZK").arg(ui->lePDFEUR2CZK->text());
    text += "<br>";
    text += QString("Přepočet měn z GBP do CZK byl proveden jednotným kurzem: 1 EUR = %1 CZK").arg(ui->lePDFGBP2CZK->text());
    text += "<br><br>";

    if (okCustom && !customText.isEmpty())
    {
        text += customText;
    }

    text += "</html>";

    QTextDocument doc;
    doc.setHtml(text);
    doc.setPageSize(printer.pageLayout().pageSize().size(QPageSize::Point)); // This is necessary if you want to hide the page number
    doc.print(&printer);
}

void MainWindow::deOverviewYearChanged(const QDate &date)
{
    disconnect(ui->deOverviewFrom, &QDateEdit::userDateChanged, nullptr, nullptr);

    int year = date.year();

    ui->deOverviewFrom->setDate(QDate(year, 1, 1));
    ui->deOverviewTo->setDate(QDate(year, 12, 31));

    sSETTINGS set = database->getSetting();
    set.lastOverviewFrom = ui->deOverviewFrom->date();
    set.lastOverviewTo = ui->deOverviewTo->date();
    database->setSettingSlot(set);

    connect(
        ui->deOverviewFrom, &QDateEdit::userDateChanged,
        [=]( ) { fillOverviewTable(); fillOverviewSlot(); }
        );
}

void MainWindow::on_deOverviewFrom_userDateChanged(const QDate &date)
{
    sSETTINGS set = database->getSetting();
    set.lastOverviewFrom = date;
    database->setSettingSlot(set);
}

void MainWindow::on_deOverviewTo_userDateChanged(const QDate &date)
{
    sSETTINGS set = database->getSetting();
    set.lastOverviewTo = date;
    database->setSettingSlot(set);
}

void MainWindow::on_deGraphYear_userDateChanged(const QDate &date)
{
    int year = date.year();

    ui->deGraphFrom->setDate(QDate(year, 1, 1));
    ui->deGraphTo->setDate(QDate(year, 12, 31));
}

void MainWindow::on_dePDFYear_userDateChanged(const QDate &date)
{
    int year = date.year();

    ui->dePDFFrom->setDate(QDate(year, 1, 1));
    ui->dePDFTo->setDate(QDate(year, 12, 31));
}


void MainWindow::on_pbAddRecord_clicked()
{
    QDialog *inputDlg = new QDialog(this);
    inputDlg->setAttribute(Qt::WA_DeleteOnClose);
    inputDlg->setWindowTitle("Add record");


    QStringList isinWords;
    QStringList tickerWords;
    QVector<sISINDATA> isinList = database->getIsinList();

    for (const sISINDATA &key : qAsConst(isinList))
    {
        tickerWords << key.ticker;
        isinWords << key.ISIN;
    }


    QVBoxLayout *VB = new QVBoxLayout(inputDlg);

    QHBoxLayout *HB1 = new QHBoxLayout();
    QLabel *label1 = new QLabel("Date", inputDlg);
    QDateEdit *date = new QDateEdit(QDate::currentDate(), inputDlg);
    HB1->addWidget(label1);
    HB1->addWidget(date);

    QHBoxLayout *HB2 = new QHBoxLayout();
    QLabel *label2 = new QLabel("Type", inputDlg);

    QComboBox *type = new QComboBox(inputDlg);
    QStringList typeList;
    typeList << "Deposit" << "Withdrawal" << "Buy" << "Sell" << "Fee" << "Dividend";
    type->addItems(typeList);

    HB2->addWidget(label2);
    HB2->addWidget(type);

    QHBoxLayout *HB3 = new QHBoxLayout();
    QLabel *label3 = new QLabel("Ticker", inputDlg);

    QLineEdit *leTicker = new QLineEdit(inputDlg);
    QCompleter *tickerCompleter = new QCompleter(tickerWords, this);
    tickerCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    leTicker->setCompleter(tickerCompleter);

    HB3->addWidget(label3);
    HB3->addWidget(leTicker);

    QHBoxLayout *HB4 = new QHBoxLayout();
    QLabel *label4 = new QLabel("ISIN", inputDlg);

    QLineEdit *leISIN = new QLineEdit(inputDlg);
    QCompleter *isinCompleter = new QCompleter(isinWords, this);
    isinCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    leISIN->setCompleter(isinCompleter);

    HB4->addWidget(label4);
    HB4->addWidget(leISIN);

    // Autocomplete ticker or isin if exists
    connect(leTicker, &QLineEdit::editingFinished, [leISIN, leTicker, this]()
            {
                if (leISIN->text().isEmpty())
                {
                    QVector<sISINDATA> isin = database->getIsinList();

                    auto it = std::find_if (isin.begin(), isin.end(), [leTicker](sISINDATA a)
                                           {
                                               return a.ticker == leTicker->text();
                                           }
                                           );

                    if (it != isin.end())
                    {
                        leISIN->setText(it->ISIN);
                    }
                }
            });


    connect(leISIN, &QLineEdit::editingFinished, [leISIN, leTicker, this]()
            {
                if (leTicker->text().isEmpty())
                {
                    QVector<sISINDATA> isin = database->getIsinList();

                    auto it = std::find_if (isin.begin(), isin.end(), [leISIN](sISINDATA a)
                                           {
                                               return a.ISIN == leISIN->text();
                                           }
                                           );

                    if (it != isin.end())
                    {
                        leTicker->setText(it->ticker);
                    }
                }
            });


    QHBoxLayout *HB5 = new QHBoxLayout();
    QLabel *label5 = new QLabel("Currency", inputDlg);
    QComboBox *cmCurrency = new QComboBox(inputDlg);
    cmCurrency->addItem("CZK");
    cmCurrency->addItem("EUR");
    cmCurrency->addItem("USD");
    HB5->addWidget(label5);
    HB5->addWidget(cmCurrency);

    QHBoxLayout *HB6 = new QHBoxLayout();
    QLabel *label6 = new QLabel("Count", inputDlg);
    QLineEdit *leCount = new QLineEdit(inputDlg);
    leCount->setValidator(new QDoubleValidator(0, 9999, 2, leCount));
    HB6->addWidget(label6);
    HB6->addWidget(leCount);

    QHBoxLayout *HB7 = new QHBoxLayout();
    QLabel *label7 = new QLabel("Price per pcs", inputDlg);
    QLineEdit *lePrice = new QLineEdit(inputDlg);
    lePrice->setValidator(new QDoubleValidator(0, 9999, 2, leCount));
    HB7->addWidget(label7);
    HB7->addWidget(lePrice);

    QHBoxLayout *HB8 = new QHBoxLayout();
    QLabel *label8 = new QLabel("Total fee/tax", inputDlg);
    QLineEdit *leFee = new QLineEdit(inputDlg);
    leFee->setValidator(new QDoubleValidator(0, 9999, 2, leCount));
    HB8->addWidget(label8);
    HB8->addWidget(leFee);

    QPushButton *pbSave = new QPushButton("Add", inputDlg);
    connect(pbSave, &QPushButton::clicked,
            [=]()
            {
                if ( leTicker->text().isEmpty() || leISIN->text().isEmpty() || leCount->text().isEmpty() || lePrice->text().isEmpty() || leFee->text().isEmpty() )
                {
                    QMessageBox::critical(inputDlg,
                                          "Add record",
                                          "All fields are required!",
                                          QMessageBox::Ok);
                }
                else
                {
                    lastRecord.dateTime = date->dateTime();
                    lastRecord.type = static_cast<eSTOCKEVENTTYPE>(type->currentIndex());
                    lastRecord.ticker = leTicker->text();
                    lastRecord.ISIN = leISIN->text();
                    lastRecord.currency = static_cast<eCURRENCY>(cmCurrency->currentIndex());
                    lastRecord.count = leCount->text().toInt();
                    lastRecord.price = lePrice->text().toDouble();
                    lastRecord.fee = leFee->text().toDouble();


                    QApplication::setOverrideCursor(Qt::WaitCursor);

                    connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::addRecord);
                    manager.get()->execute("https://finviz.com/quote.ashx?t=" + leTicker->text());
                }
            }
            );


    VB->addLayout(HB1);
    VB->addLayout(HB2);
    VB->addLayout(HB3);
    VB->addLayout(HB4);
    VB->addLayout(HB5);
    VB->addLayout(HB6);
    VB->addLayout(HB7);
    VB->addLayout(HB8);
    VB->addWidget(pbSave);

    inputDlg->setLayout(VB);

    inputDlg->open();
}

void MainWindow::addRecord(const QByteArray data, QString statusCode)
{
    disconnect(manager.get(), &DownloadManager::sendData, this, &MainWindow::addRecord);

    if (!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the request! %1\nPlease check the ticker %2").arg(statusCode).arg(lastRecord.ticker));
    }
    else
    {
        sSTOCKDATA row1;
        row1.dateTime = lastRecord.dateTime;
        row1.type = lastRecord.type;
        row1.ticker = lastRecord.ticker;
        row1.ISIN = lastRecord.ISIN;
        row1.currency = lastRecord.currency;
        row1.count = lastRecord.count;
        row1.price = lastRecord.price;
        row1.source = MANUALLY;

        sSTOCKDATA row2;

        if (!qFuzzyIsNull(lastRecord.fee))
        {
            row2.dateTime = lastRecord.dateTime;
            row2.type = FEE;
            row2.ticker = lastRecord.ticker;
            row2.ISIN = lastRecord.ISIN;
            row2.currency = lastRecord.currency;
            row2.price = lastRecord.fee;
            row1.source = MANUALLY;
        }

        sONLINEDATA table = screener->finvizParse(QString(data));
        row1.stockName = table.info.stockName;
        row2.stockName = table.info.stockName;


        StockDataType stockList = stockData->getStockData();

        QVector<sSTOCKDATA> vector = stockList[lastRecord.ISIN];
        vector.append(row1);

        if (!qFuzzyIsNull(lastRecord.fee))
        {
            vector.append(row2);
        }

        // The record is not in the ISIN list, so add it
        auto it = stockList.find(lastRecord.ISIN);

        if (it == stockList.end())
        {
            QVector<sISINDATA> isinList = database->getIsinList();

            sISINDATA record;
            record.ISIN = lastRecord.ISIN;
            record.name = table.info.stockName;
            record.sector = table.info.sector;
            record.industry = table.info.industry;
            record.lastUpdate = QDateTime(QDate(2000, 2, 31), QTime(0, 0, 0));      // should always return invalid date

            isinList.push_back(record);
            database->setIsinList(isinList);

            fillISINTable();
        }

        stockList[lastRecord.ISIN] = vector;

        stockData->setStockData(stockList);

        fillOverviewSlot();
        fillOverviewTable();
    }

    QApplication::restoreOverrideCursor();
}


/********************************
*
*  DEGIRO
*
********************************/

void MainWindow::loadDegiroCSVslot()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    degiro->loadCSV(database->getSetting().degiroCSV, database->getSetting().degiroCSVdelimeter);

    if (degiro->getIsRAWFile())
    {
        fillDegiroTable();
        fillOverviewSlot();
        fillOverviewTable();

        setStatus("The DeGiro csv file has been loaded!");
    }
    else
    {
        setStatus("The DeGiro data are corrupted or are not loaded!");
    }

    QApplication::restoreOverrideCursor();
}

void MainWindow::setDegiroDataSlot(StockDataType newStockData)
{
    StockDataType stockList = stockData->getStockData();

    // Delete old DeGiro data
    QMutableHashIterator<QString, QVector<sSTOCKDATA>> it(stockList);

    while (it.hasNext())
    {
        it.next();

        QMutableVectorIterator<sSTOCKDATA> i(it.value());

        while (i.hasNext())
        {
            if (i.next().source == DEGIRO)
            {
                i.remove();
            }
        }

        if (it.value().count() == 0)
        {
            it.remove();
        }
    }


    // Insert new DeGiro data
    QVector<sISINDATA> isinList = database->getIsinList();
    QList<QString> keys = newStockData.keys();

    for (const QString &key : qAsConst(keys))
    {
        auto i = stockList.find(key);

        if (i == stockList.end())
        {
            stockList.insert(key, newStockData.value(key));

            // Find ISIN, if does not exist add it
            QString ISIN = newStockData.value(key).first().ISIN;
            QString stockName = newStockData.value(key).first().stockName;

            if (ISIN.isEmpty() || stockName.isEmpty() || stockName.toLower().contains("fundshare")) continue;

            auto iter = std::find_if (isinList.begin(), isinList.end(), [ISIN](sISINDATA a)
                                  {
                                      return a.ISIN == ISIN;
                                  }
                                  );

            if (iter == isinList.end())
            {
                sISINDATA record;
                record.ISIN = ISIN;
                record.name = stockName;
                record.lastUpdate = QDateTime(QDate(2000, 2, 31), QTime(0, 0, 0));

                isinList.push_back(record);
            }
        }
        else
        {
            i->append(newStockData.value(key));
        }
    }


    // Assign tickers to ISIN
    QList<QString> isinKeys = stockList.keys();

    for (const QString &key : qAsConst(isinKeys))
    {
        auto isinIt = std::find_if (isinList.begin(), isinList.end(), [key](sISINDATA a)
                                   {
                                       return a.ISIN == key;
                                   }
                                   );

        QString ticker;

        if (isinIt != isinList.end())
        {
            ticker = isinIt->ticker;
        }

        QVector<sSTOCKDATA> vector = stockList.value(key);

        QMutableVectorIterator i(vector);

        while(i.hasNext())
        {
            i.next();
            i.value().ticker = ticker;
        }

        stockList[key] = vector;
    }



    database->setIsinList(isinList);
    fillISINTable();
    stockData->setStockData(stockList);
}

void MainWindow::setDegiroHeader()
{
    QStringList header;
    header << "Date" << "Product" << "ISIN" << "Description" << "Currency" << "Value" << "Balance";
    ui->tableDegiro->setColumnCount(header.count());


    ui->tableDegiro->horizontalHeader()->setVisible(true);
    ui->tableDegiro->verticalHeader()->setVisible(true);

    ui->tableDegiro->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableDegiro->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableDegiro->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableDegiro->setShowGrid(true);

    ui->tableDegiro->setHorizontalHeaderLabels(header);
}

void MainWindow::fillDegiroTable()
{
    QVector<sDEGIRORAW> degiroRawData = degiro->getRawData();

    if (degiroRawData.isEmpty())
    {
        return;
    }

    ui->tableDegiro->setRowCount(0);

    ui->tableDegiro->setSortingEnabled(false);
    for (int a = 0; a<degiroRawData.count(); ++a)
    {
        ui->tableDegiro->insertRow(a);

        QTableWidgetItem *item1 = new QTableWidgetItem;
        item1->setData(Qt::EditRole, degiroRawData.at(a).dateTime.date());
        ui->tableDegiro->setItem(a, 0, item1);

        ui->tableDegiro->setItem(a, 1, new QTableWidgetItem(degiroRawData.at(a).product));
        ui->tableDegiro->setItem(a, 2, new QTableWidgetItem(degiroRawData.at(a).ISIN));
        ui->tableDegiro->setItem(a, 3, new QTableWidgetItem(degiroRawData.at(a).description));
        ui->tableDegiro->setItem(a, 4, new QTableWidgetItem(database->getCurrencyText(degiroRawData.at(a).currency)));

        QTableWidgetItem *item2 = new QTableWidgetItem;
        item2->setData(Qt::EditRole, degiroRawData.at(a).price);
        ui->tableDegiro->setItem(a, 5, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem;
        item3->setData(Qt::EditRole, degiroRawData.at(a).balance);
        ui->tableDegiro->setItem(a, 6, item3);
    }
    ui->tableDegiro->setSortingEnabled(true);


    for (int row = 0; row<ui->tableDegiro->rowCount(); ++row)
    {
        for (int col = 0; col<ui->tableDegiro->columnCount(); ++col)
        {
            ui->tableDegiro->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->tableDegiro->resizeColumnsToContents();

    for (int col = 0; col < ui->tableDegiro->horizontalHeader()->count(); ++col)
    {
        if (col == 0 || col == 2 || col == 4 || col == 5 || col == 6)
        {
            continue;
        }

        ui->tableDegiro->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
    }
}


/********************************
*
*  Tastyworks
*
********************************/
void MainWindow::loadTastyworksCSVslot()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    tastyworks->loadCSV(database->getSetting().tastyworksCSV, database->getSetting().tastyworksCSVdelimeter);

    /*if (tastyworks->getIsRAWFile())
    {
        fillDegiroTable();
        fillOverviewSlot();
        fillOverviewTable();

        setStatus("The Tastyworks csv file has been loaded!");
    }
    else
    {
        setStatus("The Tastyworks data are corrupted or are not loaded!");
    }*/

    QApplication::restoreOverrideCursor();
}


/********************************
*
*  SCREENER
*
********************************/
void MainWindow::loadOnlineParametersSlot()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    lastRequestSource = FINVIZ;

    // Clean and save screener params
    QVector<sSCREENERPARAM> screenerParams = database->getScreenerParams();
    screenerParams.clear();
    database->setScreenerParams(screenerParams);

    connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::parseOnlineParameters);
    manager.get()->execute("https://finviz.com/quote.ashx?t=T");
}

void MainWindow::parseOnlineParameters(const QByteArray data, QString statusCode)
{
    disconnect(manager.get(), &DownloadManager::sendData, this, &MainWindow::parseOnlineParameters);

    if (!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the request! %1").arg(statusCode));
        QApplication::restoreOverrideCursor();
    }
    else
    {
        sONLINEDATA table;
        QVector<sSCREENERPARAM> screenerParams = database->getScreenerParams();

        sSCREENERPARAM param;

        if (lastRequestSource == FINVIZ)
        {
            QStringList infoData;
            infoData << "Ticker" << "Stock name" << "Sector" << "Industry" << "Country";

            for (const QString &par : qAsConst(infoData))
            {
                param.name = par;
                param.enabled = true;
                screenerParams.push_back(param);
            }
        }

        switch (lastRequestSource)
        {
            case FINVIZ:
                table = screener->finvizParse(QString(data));
                param.name = "FINVIZ";
                break;
            case YAHOO:
                table = screener->yahooParse(QString(data));
                param.name = "YAHOO";
                break;
        }

        param.enabled = false;
        screenerParams.push_back(param);

        for (const QString &key : table.row.keys())
        {
            param.name = key;
            param.enabled = false;
            screenerParams.push_back(param);
        }

        database->setScreenerParams(screenerParams);

        if (lastRequestSource == FINVIZ)
        {
            lastRequestSource = YAHOO;
            connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::parseOnlineParameters);
            manager.get()->execute("https://finance.yahoo.com/quote/T/key-statistics");
        }
        else        // end
        {       
            std::sort(screenerParams.begin(), screenerParams.end(), [](sSCREENERPARAM a, sSCREENERPARAM b) {return a.name < b.name; });

            QApplication::restoreOverrideCursor();
            database->setScreenerParams(screenerParams);
            emit updateScreenerParams(screenerParams);
            setStatus("Parameters have been loaded");

            if (screenerTabs.count() != 0)
            {
                ui->pbAddTicker->setEnabled(true);
            }
        }
    }
}

void MainWindow::setScreenerParamsSlot(QVector<sSCREENERPARAM> params)
{
    database->setScreenerParams(params);

    for (int a = 0; a<screenerTabs.count(); ++a)
    {
        setScreenerHeader(screenerTabs.at(a));
        fillScreenerTable(screenerTabs.at(a));
    }
}

void MainWindow::setScreenerHeader(ScreenerTab *st)
{
    if (!st) return;

    QTableWidget *tab = st->getScreenerTable();

    tab->setRowCount(0);

    QStringList header = database->getEnabledScreenerParams();

    header << "Delete";

    tab->setColumnCount(header.count());

    tab->horizontalHeader()->setVisible(true);
    tab->verticalHeader()->setVisible(true);

    tab->setSelectionBehavior(QAbstractItemView::SelectRows);
    tab->setSelectionMode(QAbstractItemView::SingleSelection);
    tab->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tab->setShowGrid(true);

    tab->setHorizontalHeaderLabels(header);
}

void MainWindow::on_pbAddTicker_clicked()
{
    if (currentScreenerIndex == -1)
    {
        QMessageBox::warning(this,
                             "Screener",
                             "Please create at least one screener!",
                             QMessageBox::Ok);

        setStatus("Please create at least one screener!");

        return;
    }

    if (ui->leTicker->text().trimmed().isEmpty())
    {
        setStatus("The ticker field is empty!");
        return;
    }

    QString ticker = ui->leTicker->text().trimmed();
    ui->leTicker->setText(ticker.toUpper());

    lastLoadedTable.row.clear();
    lastRequestSource = FINVIZ;
    connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::getData);
    QString request = QString("https://finviz.com/quote.ashx?t=%1").arg(ticker);
    manager->execute(request);
}

void MainWindow::getData(const QByteArray data, QString statusCode)
{
    if (!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the request! %1").arg(statusCode));
    }
    else
    {
        QString ticker = ui->leTicker->text().trimmed();

        if (ticker.isEmpty()) // when the update came from ISIN table, the leTicker is empty so we need to assign the ticker
        {
            ticker = ui->tableISIN->item(ui->tableISIN->currentRow(), 1)->text();
        }

        sONLINEDATA table;

        switch (lastRequestSource)
        {
            case FINVIZ:
                table = screener->finvizParse(QString(data));
                table.info.ticker = ticker;
                lastLoadedTable.info = table.info;
                break;
            case YAHOO:
                table = screener->yahooParse(QString(data));
                break;
        }


        for (const QString &key : table.row.keys())
        {
            lastLoadedTable.row.insert(key, table.row.value(key));
        }

        if (lastRequestSource == FINVIZ)
        {
            lastRequestSource = YAHOO;
            QString request = QString("https://finance.yahoo.com/quote/%1/key-statistics").arg(ticker);
            manager->execute(request);
        }
        else    // end
        {
            disconnect(manager.get(), &DownloadManager::sendData, this, &MainWindow::getData);

            QString ISIN;
            QVector<sISINDATA> isinList = database->getIsinList();

            auto it = std::find_if (isinList.begin(), isinList.end(),
                                   [ticker](sISINDATA a)
                                   {
                                       return a.ticker == ticker;
                                   } );

            if (it != isinList.end())
            {
                ISIN = it->ISIN;
            }

            stockData->saveOnlineStockInfo(ISIN, lastLoadedTable);
            updateStockDataSlot(ISIN, lastLoadedTable);
            dataLoaded();
            emit refreshTickers(ticker);
            setStatus(QString("Ticker %1 has been updated.").arg(ticker));
        }
    }
}

void MainWindow::dataLoaded()
{
    QStringList screenerParams = database->getEnabledScreenerParams();

    TickerDataType tickerLine;

    QString ticker = ui->leTicker->text().trimmed();

    if (ticker.isEmpty()) return;

    QStringList infoData;
    infoData << "Ticker" << "Stock name" << "Sector" << "Industry" << "Country";

    bool bParamFound = false;

    for (int param = 0; param<screenerParams.count(); ++param)
    {
        bParamFound = false;

        for (int table = 0; table<lastLoadedTable.row.count() && !bParamFound; ++table)
        {
            for (int info = 0; info<infoData.count() && !bParamFound; ++info)
            {
                if (screenerParams.at(param).toLower() == infoData.at(info).toLower())
                {
                    if (infoData.at(info) == "Industry")
                    {
                        tickerLine.push_back(qMakePair(infoData.at(info), lastLoadedTable.info.industry));
                    }
                    else if (infoData.at(info) == "Ticker")
                    {
                        tickerLine.push_back(qMakePair(infoData.at(info), lastLoadedTable.info.ticker));
                    }
                    else if (infoData.at(info) == "Stock name")
                    {
                        tickerLine.push_back(qMakePair(infoData.at(info), lastLoadedTable.info.stockName));
                    }
                    else if (infoData.at(info) == "Sector")
                    {
                        tickerLine.push_back(qMakePair(infoData.at(info), lastLoadedTable.info.sector));
                    }
                    else if (infoData.at(info) == "Country")
                    {
                        tickerLine.push_back(qMakePair(infoData.at(info), lastLoadedTable.info.country));
                    }

                    bParamFound = true;
                }
            }
        }
    }


    bParamFound = false;

    for (int param = 0; param<screenerParams.count(); ++param)
    {
        bParamFound = false;

        for (int table = 0; table<lastLoadedTable.row.count() && !bParamFound; ++table)
        {
            if (lastLoadedTable.row.contains(screenerParams.at(param)))
            {
                tickerLine.push_back(qMakePair(screenerParams.at(param), lastLoadedTable.row.value(screenerParams.at(param))));

                bParamFound = true;
            }
        }
    }

    if (tickerLine.isEmpty()) return;

    if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    int tickerOrder = findScreenerTicker(ticker);

    if (tickerOrder == -2)
    {
        return;
    }
    else if (tickerOrder == -1)       // Ticker does not exist, so add it
    {
        currentScreenerData.screenerData.push_back(tickerLine);

        QVector<sSCREENER> allScreenerData = screener->getAllScreenerData();

        if (currentScreenerIndex < allScreenerData.count())
        {
            allScreenerData[currentScreenerIndex] = currentScreenerData;
            screener->setAllScreenerData(allScreenerData);
        }

        screenerTabs.at(currentScreenerIndex)->setScreenerData(currentScreenerData);

        insertScreenerRow(tickerLine);

        setStatus(QString("Ticker %1 has been added").arg(ticker));
    }
    else    // Ticker already exists
    {
        currentScreenerData.screenerData[tickerOrder] = tickerLine;

        QVector<sSCREENER> allScreenerData = screener->getAllScreenerData();

        if (currentScreenerIndex < allScreenerData.count())
        {
            allScreenerData[currentScreenerIndex] = currentScreenerData;
            screener->setAllScreenerData(allScreenerData);

            screenerTabs.at(currentScreenerIndex)->setScreenerData(currentScreenerData);

            setStatus(QString("Ticker %1 has been updated").arg(ticker));
        }

        int currentRowInTable = -1;

        if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;
        ScreenerTab *tab = screenerTabs.at(currentScreenerIndex);

        // Check if the filter is enabled, if so, some tickers might be hidden, so find correct row in the table for our ticker
        if (ui->cbFilter->isChecked())
        {
            bool found = false;

            for (int row = 0; row<tab->getScreenerTable()->rowCount() && !found; ++row)
            {
                for (int col = 0; col<tab->getScreenerTable()->columnCount()-1 && !found; ++col)
                {
                    if (tab->getScreenerTable()->item(row, col) && tab->getScreenerTable()->item(row, col)->text() == ui->leTicker->text())
                    {
                        currentRowInTable = row;
                        found = true;
                        break;
                    }
                }
            }
        }
        else
        {
            currentRowInTable = tickerOrder;
        }

        if (currentRowInTable != -1)
        {
            int pos = 0;

            for (int col = 0; col<currentScreenerData.screenerData.at(tickerOrder).count(); ++col)
            {
                for (int param = 0; param<screenerParams.count(); ++param)
                {
                    if (currentScreenerData.screenerData.at(tickerOrder).at(col).first == screenerParams.at(param))
                    {
                        QString text = tickerLine.at(col).second;

                        if (!tab->getScreenerTable()->item(currentRowInTable, param))      // the item does not exist, so create it
                        {
                            QTableWidgetItem *item = new QTableWidgetItem;

                            bool ok;
                            double testNumber = text.toDouble(&ok);

                            if (ok)
                            {
                                item->setData(Qt::EditRole, testNumber);
                            }
                            else
                            {
                                item->setData(Qt::EditRole, text);
                            }

                            item->setTextAlignment(Qt::AlignCenter);                           
                            tab->getScreenerTable()->setSortingEnabled(false);
                            tab->getScreenerTable()->setItem(currentRowInTable, param, item);
                            tab->getScreenerTable()->setSortingEnabled(true);

                            for (const sFILTER &filter : qAsConst(filterList))
                            {
                                if (filter.param == screenerParams.at(param))
                                {
                                    applyFilterOnItem(screenerTabs.at(currentScreenerIndex), item, filter);
                                }
                            }
                        }
                        else            // update the data
                        {
                            QTableWidgetItem *item = tab->getScreenerTable()->item(currentRowInTable, param);

                            if (item)
                            {
                                item->setText(text);

                                for (const sFILTER &filter : qAsConst(filterList))
                                {
                                    if (filter.param == screenerParams.at(param))
                                    {
                                        applyFilterOnItem(screenerTabs.at(currentScreenerIndex), item, filter);
                                    }
                                }
                            }
                        }

                        pos++;
                        break;
                    }
                }
            }
        }
    }

    ui->leTicker->clear();
}

// Return a row for specific ticker
int MainWindow::findScreenerTicker(QString ticker)
{
    if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return -2;

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    for (int row = 0; row<currentScreenerData.screenerData.count(); ++row)
    {
        for (int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
        {
            if (currentScreenerData.screenerData.at(row).at(col).second == ticker)
            {
                return row;
            }
        }
    }

    return -1;
}

void MainWindow::insertScreenerRow(TickerDataType tickerData)
{
    if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    ScreenerTab *st = screenerTabs.at(currentScreenerIndex);

    int row = st->getScreenerTable()->rowCount();
    st->getScreenerTable()->insertRow(row);

    QStringList screenerParams = database->getEnabledScreenerParams();

    for (int col = 0; col<tickerData.count(); ++col)
    {
        for (int param = 0; param<screenerParams.count(); ++param)
        {
            if (tickerData.at(col).first == screenerParams.at(param))
            {
                QString text = tickerData.at(col).second;
                QTableWidgetItem *item = new QTableWidgetItem();

                bool ok;
                double testNumber = text.toDouble(&ok);

                if (ok)
                {
                    item->setData(Qt::EditRole, testNumber);
                }
                else
                {
                    item->setData(Qt::EditRole, text);
                }

                item->setTextAlignment(Qt::AlignCenter);                
                st->getScreenerTable()->setSortingEnabled(false);
                st->getScreenerTable()->setItem(row, param, item);
                st->getScreenerTable()->setSortingEnabled(true);

                for (const sFILTER &filter : qAsConst(filterList))
                {
                    if (filter.param == screenerParams.at(param))
                    {
                        applyFilterOnItem(st, item, filter);
                    }
                }

                break;
            }
        }
    }

    QTableWidgetItem *item = new QTableWidgetItem("Delete");
    item->setCheckState(Qt::Unchecked);
    item->setTextAlignment(Qt::AlignCenter);    
    st->getScreenerTable()->setSortingEnabled(false);
    st->getScreenerTable()->setItem(st->getScreenerTable()->rowCount()-1, st->getScreenerTable()->columnCount()-1, item);
    st->getScreenerTable()->setSortingEnabled(true);

    st->getScreenerTable()->resizeColumnsToContents();
}

void MainWindow::fillScreenerTable(ScreenerTab *st)
{
    if (!st) return;

    sSCREENER currentScreenerData = st->getScreenerData();

    if (currentScreenerData.screenerData.isEmpty()) return;

    st->getScreenerTable()->setRowCount(0);

    QStringList screenerParams = database->getEnabledScreenerParams();

    bool nextRow = false;
    int hiddenRows = 0;

    for (int row = 0; row<currentScreenerData.screenerData.count(); ++row)
    {
        st->getScreenerTable()->insertRow(row-hiddenRows);

        for (int param = 0; param<screenerParams.count(); ++param)
        {
            for (int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
            {
                if (currentScreenerData.screenerData.at(row).at(col).first == screenerParams.at(param))
                {
                    bool isFilter = false;
                    QString color;
                    double val1 = 0.0;
                    double val2 = 0.0;
                    eFILTER filterType = LOWER;

                    if (ui->cbFilter->isChecked())
                    {
                        for (const sFILTER &filter : qAsConst(filterList))
                        {
                            if (filter.param == screenerParams.at(param))
                            {
                                color = filter.color;
                                val1 = filter.val1;
                                val2 = filter.val2;
                                filterType = filter.filter;
                                isFilter = true;
                                break;
                            }
                        }
                    }

                    QString text = currentScreenerData.screenerData.at(row).at(col).second;
                    int percentSign = text.indexOf("%");

                    if (percentSign != -1)
                    {
                        text = text.mid(0, percentSign);
                    }

                    QTableWidgetItem *item = new QTableWidgetItem;

                    bool ok;
                    double testNumber = text.toDouble(&ok);

                    if (ok)
                    {
                        item->setData(Qt::EditRole, testNumber);
                    }
                    else
                    {
                        item->setData(Qt::EditRole, text);
                    }

                    item->setTextAlignment(Qt::AlignCenter);

                    if (isFilter)
                    {
                        switch (filterType)
                        {
                            case LOWER:
                                if (text.toDouble() < val1)
                                {
                                    if (color == "HIDE")
                                    {
                                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                            case HIGHER:
                                if (text.toDouble() > val1)
                                {
                                    if (color == "HIDE")
                                    {
                                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                            case BETWEEN:
                                if (text.toDouble() > val1 && text.toDouble() < val2)
                                {
                                    if (color == "HIDE")
                                    {
                                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                        }
                    }

                    st->getScreenerTable()->setSortingEnabled(false);
                    st->getScreenerTable()->setItem(row - hiddenRows, param, item);
                    st->getScreenerTable()->setSortingEnabled(true);

                    break;
                }
            }
        }

        {   // because of goto
            QTableWidgetItem *item = new QTableWidgetItem("Delete");
            item->setCheckState(Qt::Unchecked);
            item->setTextAlignment(Qt::AlignCenter);
            st->getScreenerTable()->setSortingEnabled(false);
            st->getScreenerTable()->setItem(st->getScreenerTable()->rowCount()-1, st->getScreenerTable()->columnCount()-1, item);
            st->getScreenerTable()->setSortingEnabled(true);
        }

        nextRow:
        if (nextRow)
        {
            hiddenRows++;
            nextRow = false;
        }
    }

    st->getHiddenRows()->setText(QString("Hidden rows: %1").arg(hiddenRows));

    st->getScreenerTable()->resizeColumnsToContents();

    for (int col = 0; col < st->getScreenerTable()->horizontalHeader()->count(); ++col)
    {
        if (st->getScreenerTable()->horizontalHeaderItem(col)->text() == "Stock name")
        {
            st->getScreenerTable()->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
        }
    }
}

void MainWindow::applyFilter(ScreenerTab *st)
{
    if (!st) return;

    QTableWidget *tab = st->getScreenerTable();
    QStringList screenerParams = database->getEnabledScreenerParams();
    sSCREENER currentScreenerData = st->getScreenerData();

    int hiddenRows = 0;

    for (int row = 0; row<tab->rowCount(); ++row)
    {
        for (int param = 0; param<screenerParams.count(); ++param)
        {
            for (int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
            {
                for (const sFILTER &filter : qAsConst(filterList))
                {
                    if (filter.param == screenerParams.at(param) &&
                            currentScreenerData.screenerData.at(row).at(col).first == screenerParams.at(param))
                    {
                        QTableWidgetItem *item = tab->item(row, param);

                        hiddenRows += applyFilterOnItem(st, item, filter);
                        break;

                    }
                }
            }
        }
    }

    st->getHiddenRows()->setText(QString("Hidden rows: %1").arg(hiddenRows));

    st->getScreenerTable()->resizeColumnsToContents();
}

int MainWindow::applyFilterOnItem(ScreenerTab *st, QTableWidgetItem *item, sFILTER filter)
{
    if (!item || !st)
    {
        return 0;
    }

    int hiddenRows = 0;

    QString color = filter.color;
    double val1 = filter.val1;
    double val2 = filter.val2;
    eFILTER filterType = filter.filter;

    QString text = item->text();
    int percentSign = text.indexOf("%");

    if (percentSign != -1)
    {
        text = text.mid(0, percentSign);
    }

    bool ok;
    double testNumber = text.toDouble(&ok);

    if (ok)
    {
        item->setData(Qt::EditRole, testNumber);
        switch (filterType)
        {
            case LOWER:
                if (text.toDouble() < val1)
                {
                    if (color == "HIDE")
                    {
                        hiddenRows++;
                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                    }
                    else
                    {
                        item->setBackground(QColor("#" + color));
                    }
                }
                else
                {
                    item->setBackground(QColor(item->row()%2 ? "#dadbde" : "#f6f7fa"));
                }
                break;
            case HIGHER:
                if (text.toDouble() > val1)
                {
                    if (color == "HIDE")
                    {
                        hiddenRows++;
                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                    }
                    else
                    {
                        item->setBackground(QColor("#" + color));
                    }
                }
                else
                {
                    item->setBackground(QColor(item->row()%2 ? "#dadbde" : "#f6f7fa"));
                }
                break;
            case BETWEEN:
                if (text.toDouble() > val1 && text.toDouble() < val2)
                {
                    if (color == "HIDE")
                    {
                        hiddenRows++;
                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                    }
                    else
                    {
                        item->setBackground(QColor("#" + color));
                    }
                }
                else
                {
                    item->setBackground(QColor(item->row()%2 ? "#dadbde" : "#f6f7fa"));
                }
                break;
        }
    }

    return hiddenRows;
}

void MainWindow::on_pbNewScreener_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this,
                                         "Please enter new screener name",
                                         tr("Screener name:"),
                                         QLineEdit::Normal,
                                         "",
                                         &ok);

    if (ok && !text.isEmpty())
    {
        currentScreenerIndex++;
        database->setLastScreenerIndex(currentScreenerIndex);

        sSCREENER currentScreenerData;
        currentScreenerData.screenerName = text;
        currentScreenerData.screenerData.clear();

        QVector<sSCREENER> allData = screener->getAllScreenerData();
        allData.push_back(currentScreenerData);
        screener->setAllScreenerData(allData);

        ScreenerTab *st = new ScreenerTab(this);
        screenerTabs.push_back(st);
        st->setScreenerData(currentScreenerData);

        ui->tabScreener->addTab(st, currentScreenerData.screenerName);
        setScreenerHeader(st);
        ui->tabScreener->setCurrentIndex(currentScreenerIndex);

        if (screenerTabs.count() != 0)
        {
            ui->pbAddTicker->setEnabled(true);
        }
    }
}

void MainWindow::on_pbDeleteScreener_clicked()
{
    int ret = QMessageBox::warning(this,
                         "Delete screener",
                         "Do you really want to delete the currect screener? This step cannot be undone!",
                         QMessageBox::Yes, QMessageBox::No);

    if (ret == QMessageBox::Yes)
    {
        if (currentScreenerIndex < 0) return;

        QVector<sSCREENER> allData = screener->getAllScreenerData();
        allData.removeAt(currentScreenerIndex);
        screener->setAllScreenerData(allData);

        screenerTabs.removeAt(currentScreenerIndex);
        ui->tabScreener->removeTab(currentScreenerIndex);

        currentScreenerIndex--;
        database->setLastScreenerIndex(currentScreenerIndex);

        if (currentScreenerIndex > -1)
        {
            sSCREENER currentScreenerData;
            currentScreenerData = allData.at(currentScreenerIndex);
            ui->tabScreener->setCurrentIndex(currentScreenerIndex);
        }
    }
}

void MainWindow::clickedScreenerTabSlot(int index)
{
    if (index < screenerTabs.count())
    {
        currentScreenerIndex = index;
        database->setLastScreenerIndex(currentScreenerIndex);
    }
    else
    {
        qCritical() << "This will never happen";
    }
}

void MainWindow::on_pbFilter_clicked()
{
    FilterForm *dlg = new FilterForm(database->getEnabledScreenerParams(), database->getFilterList(), this);
    connect(dlg, SIGNAL(setFilter(QVector<sFILTER>)), this, SLOT(setFilterSlot(QVector<sFILTER>)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModal(false);
    dlg->open();
}

void MainWindow::on_cbFilter_clicked(bool checked)
{
    ui->pbFilter->setEnabled(checked);
    filterList = database->getFilterList();

    for (int a = 0; a<screenerTabs.count(); ++a)
    {
        fillScreenerTable(screenerTabs.at(a));
    }
}

void MainWindow::setFilterSlot(QVector<sFILTER> list)
{
    database->setFilterList(list);
    filterList = list;

    for (int a = 0; a<screenerTabs.count(); ++a)
    {
        applyFilter(screenerTabs.at(a));
    }
}


void MainWindow::on_pbRefresh_clicked()
{
    if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    currentTickers.clear();

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    for (const TickerDataType &scr : qAsConst(currentScreenerData.screenerData))
    {
        for (int a = 0; a<scr.count(); ++a)
        {
            if (scr.at(a).first == "Ticker")
            {
                currentTickers << scr.at(a).second;
                break;
            }
        }
    }

    if (!currentTickers.isEmpty())
    {
        createProgressDialog(0, currentTickers.count());

        connect(this, &MainWindow::refreshTickers, this, &MainWindow::refreshTickersSlot);

        ui->leTicker->setText(currentTickers.first());
        ui->pbAddTicker->click();
    }
}

void MainWindow::refreshTickersSlot(QString ticker)
{
    int pos = currentTickers.indexOf(ticker);

    if (pos == -1)   // error
    {
        disconnect(this, &MainWindow::refreshTickers, this, &MainWindow::refreshTickersSlot);

        if (refreshProgressDlg)
        {
            disconnect(refreshProgressDlg, &QProgressDialog::canceled, this, &MainWindow::refreshTickersCanceled);
            updateProgressDialog(pos+1);
            refreshProgressDlg->close();
        }

        setStatus("An error appeard during the refresh");
    }
    else if (pos == (currentTickers.count() - 1))  // last
    {
        if (refreshProgressDlg)
        {
            disconnect(refreshProgressDlg, &QProgressDialog::canceled, this, &MainWindow::refreshTickersCanceled);
            updateProgressDialog(pos+1);
        }

        disconnect(this, &MainWindow::refreshTickers, this, &MainWindow::refreshTickersSlot);
        setStatus("All tickers have been refreshed");
    }
    else
    {
        if (refreshProgressDlg)
        {
            updateProgressDialog(pos+1);
        }

        QString next = currentTickers.at(pos + 1);

        ui->leTicker->setText(next);
        ui->pbAddTicker->click();
    }
}

void MainWindow::refreshTickersCanceled()
{
    disconnect(this, &MainWindow::refreshTickers, this, &MainWindow::refreshTickersSlot);

    if (refreshProgressDlg)
    {
        disconnect(refreshProgressDlg, &QProgressDialog::canceled, this, &MainWindow::refreshTickersCanceled);
        updateProgressDialog(currentTickers.count());
    }

    setStatus("The process has been canceled");
}

void MainWindow::createProgressDialog(int min, int max)
{
    if (refreshProgressDlg) return;

    refreshProgressDlg = new QProgressDialog("Operation in progress", "Cancel", min, max, this);
    connect(refreshProgressDlg, &QProgressDialog::canceled, this, &MainWindow::refreshTickersCanceled);

    refreshProgressDlg->setWindowFlags(Qt::WindowStaysOnTopHint);
    refreshProgressDlg->setAttribute(Qt::WA_DeleteOnClose);
    refreshProgressDlg->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    refreshProgressDlg->setParent(this);
    refreshProgressDlg->resize(400, 50);
    refreshProgressDlg->adjustSize();
    refreshProgressDlg->setMinimumDuration(0);
    refreshProgressDlg->setWindowModality(Qt::NonModal);
    refreshProgressDlg->setValue(0);


    QPropertyAnimation *animFade = new QPropertyAnimation(refreshProgressDlg, "windowOpacity");
    animFade->setDuration(1000);
    animFade->setEasingCurve(QEasingCurve::Linear);
    animFade->setStartValue(0.0);
    animFade->setEndValue(1.0);


    QPoint p = mapToGlobal(QPoint(size().width(), size().height())) -
               QPoint(refreshProgressDlg->size().width(), refreshProgressDlg->size().height());

    QPropertyAnimation *animMove = new QPropertyAnimation(refreshProgressDlg, "pos");
    animMove->setDuration(1000);
    animMove->setEasingCurve(QEasingCurve::OutQuad);
    animMove->setStartValue(QPointF(p.x(), p.y() + refreshProgressDlg->size().height()));
    animMove->setEndValue(p);

    animFade->start(QAbstractAnimation::DeleteWhenStopped);
    animMove->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::updateProgressDialog(int val)
{
    Q_ASSERT(refreshProgressDlg);

    refreshProgressDlg->setValue(val);
}

void MainWindow::on_pbDeleteTickers_clicked()
{    
    if (currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    currentTickers.clear();

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    ScreenerTab *tab = screenerTabs.at(currentScreenerIndex);
    QStringList screenerParams = database->getEnabledScreenerParams();
    QString ticker;

    for (int tableRow = 0; tableRow<tab->getScreenerTable()->rowCount(); ++tableRow)
    {
        if (tab->getScreenerTable()->item(tableRow, tab->getScreenerTable()->columnCount()-1)->checkState() == Qt::Checked)
        {
            for (int row = 0; row<currentScreenerData.screenerData.count(); ++row)
            {
                for (int param = 0; param<screenerParams.count(); ++param)
                {
                    if (screenerParams.at(param) == "Ticker")
                    {
                        for (int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
                        {
                            if (currentScreenerData.screenerData.at(row).at(col).second == tab->getScreenerTable()->item(tableRow, param)->text())
                            {
                                currentScreenerData.screenerData.removeAt(row);

                                QVector<sSCREENER> allScreenerData = screener->getAllScreenerData();

                                if (currentScreenerIndex < allScreenerData.count())
                                {
                                    allScreenerData[currentScreenerIndex] = currentScreenerData;
                                    screener->setAllScreenerData(allScreenerData);
                                    screenerTabs.at(currentScreenerIndex)->setScreenerData(currentScreenerData);
                                }

                                break;
                            }
                        }
                    }
                }
            }

            tab->getScreenerTable()->removeRow(tableRow);

            tableRow--;
        }
    }
}

void MainWindow::on_pbAlert_clicked()
{

}

/********************************
*
*  ISIN list
*
********************************/
void MainWindow::on_pbISINAdd_clicked()
{
    if ( ui->leISINISIN->text().isEmpty() || ui->leISINName->text().isEmpty() || ui->leISINSector->text().isEmpty() || ui->leISINTicker->text().isEmpty() || ui->leISINIndustry->text().isEmpty() ) return;

    sISINDATA record;
    record.ISIN = ui->leISINISIN->text();
    record.name = ui->leISINName->text();
    record.sector = ui->leISINSector->text();
    record.ticker = ui->leISINTicker->text();
    record.industry = ui->leISINIndustry->text();
    record.lastUpdate = QDateTime(QDate(2000, 2, 31), QTime(0, 0, 0));

    QVector<sISINDATA> isin = database->getIsinList();

    auto it = std::find_if (isin.begin(), isin.end(),
                           [record](sISINDATA a)
                           {
                               return a.ISIN == record.ISIN;
                           }
                           );

    if (it != isin.end())
    {
        isin.push_back(record);
        database->setIsinList(isin);
    }
    else
    {
        setStatus(QString("ISIN %1 already exists").arg(record.ISIN));
    }
}

void MainWindow::setISINHeader()
{
    QStringList header;
    header << "ISIN" << "Ticker" << "Name" << "Sector" << "Industry" << "Last update" << "Update" << "Delete";
    ui->tableISIN->setColumnCount(header.count());

    ui->tableISIN->setRowCount(0);
    ui->tableISIN->setColumnCount(header.count());

    ui->tableISIN->horizontalHeader()->setVisible(true);
    ui->tableISIN->verticalHeader()->setVisible(true);

    ui->tableISIN->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->tableISIN->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableISIN->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableISIN->setShowGrid(true);

    ui->tableISIN->setHorizontalHeaderLabels(header);
}

void MainWindow::fillISINTable()
{
    QVector<sISINDATA> isinList = database->getIsinList();

    if (isinList.isEmpty())
    {
        return;
    }

    ui->tableISIN->setRowCount(0);

    ui->tableISIN->setSortingEnabled(false);

    for (int row = 0; row<isinList.count(); ++row)
    {
        ui->tableISIN->insertRow(row);

        ui->tableISIN->setItem(row, 0, new QTableWidgetItem(isinList.at(row).ISIN));
        ui->tableISIN->setItem(row, 1, new QTableWidgetItem(isinList.at(row).ticker));
        ui->tableISIN->setItem(row, 2, new QTableWidgetItem(isinList.at(row).name));
        ui->tableISIN->setItem(row, 3, new QTableWidgetItem(isinList.at(row).sector));
        ui->tableISIN->setItem(row, 4, new QTableWidgetItem(isinList.at(row).industry));

        QTableWidgetItem *item = new QTableWidgetItem;
        item->setData(Qt::EditRole, isinList.at(row).lastUpdate.date());
        ui->tableISIN->setItem(row, 5, item);

        QPushButton *pbUpdate = new QPushButton(ui->tableISIN);
        pbUpdate->setStyleSheet("QPushButton {border-image:url(:/images/update.png);}");
        connect(pbUpdate, &QPushButton::clicked, [this, row]()
                {
                    QVector<sISINDATA> isinList = database->getIsinList();  // we need to load it again, since the ticker might be added in the meantime

                    if (isinList.count() < row)
                    {
                        return;
                    }

                    QString ticker = isinList.at(row).ticker;

                    if (ticker.isEmpty())
                    {
                        setStatus(QString("ISIN %1 does not have assigned the ticker!").arg(isinList.at(row).ISIN));
                    }
                    else
                    {
                        QDateTime today = QDateTime::currentDateTime();

                        if (today.date() == isinList.at(row).lastUpdate.date())
                        {
                            setStatus(QString("The ticker %1 was already updated today.").arg(ticker));
                        }
                        else
                        {
                            lastLoadedTable.row.clear();

                            connect(manager.get(), &DownloadManager::sendData, this, &MainWindow::getData);

                            QString request;
                            // ToDo: might not be correct, the current stock is ETF
                            if (isinList.at(row).sector == "ETF")
                            {
                                lastRequestSource = YAHOO;
                                request = QString("https://finance.yahoo.com/quote/%1/").arg(ticker);
                            }
                            else
                            {
                                lastRequestSource = FINVIZ;
                                request = QString("https://finviz.com/quote.ashx?t=%1").arg(ticker);
                            }

                            manager->execute(request);
                        }
                    }
                }

                );

        ui->tableISIN->setItem(row, 6, new QTableWidgetItem());
        ui->tableISIN->setCellWidget(row, 6, pbUpdate);


        QPushButton *pbDelete = new QPushButton(ui->tableISIN);
        pbDelete->setStyleSheet("QPushButton {border-image:url(:/images/delete.png);}");

        connect(pbDelete, &QPushButton::clicked, [this, row, isinList]()
                {
                    int ret = QMessageBox::warning(nullptr,
                                                   "Delete record",
                                                   QString("Do you really want to delete %1?").arg(isinList.at(row).ISIN),
                                                   QMessageBox::Yes, QMessageBox::No);

                    if (ret == QMessageBox::Yes)
                    {
                        QTableWidgetItem *isinItem = ui->tableISIN->item(row, 0);

                        if (isinItem)
                        {
                            QString ISIN = isinItem->text();
                            eraseISIN(ISIN);
                            ui->tableISIN->removeRow(row);
                        }
                    }
                }

                );


        ui->tableISIN->setItem(row, 7, new QTableWidgetItem());
        ui->tableISIN->setCellWidget(row, 7, pbDelete);
    }
    ui->tableISIN->setSortingEnabled(true);

    for (int row = 0; row<ui->tableISIN->rowCount(); ++row)
    {
        for (int col = 0; col<ui->tableISIN->columnCount(); ++col)
        {
            ui->tableISIN->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->tableISIN->resizeColumnsToContents();

    for (int col = 0; col < ui->tableISIN->horizontalHeader()->count(); ++col)
    {
        if (col == 0 || col == 1 || col == 3 || col == 5 || col == 6 || col == 7)
        {
            continue;
        }

        ui->tableISIN->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
    }
}

void MainWindow::eraseISIN(QString ISIN)
{
    QVector<sISINDATA> isinList = database->getIsinList();

    isinList.erase(std::remove_if (isinList.begin(), isinList.end(), [ISIN](sISINDATA x)
                                  {
                                      return x.ISIN == ISIN;
                                  }
                                  )
                       );

    database->setIsinList(isinList);
}

void MainWindow::on_tableISIN_cellDoubleClicked(int row, int column)
{
    if (column > 4) return;

    QTableWidgetItem *clickedItem = ui->tableISIN->item(row, column);

    if (!clickedItem) return;

    QString previousText = clickedItem->text();

    QString header = "Input";

    switch(column)
    {
        case 0: header = "ISIN:"; break;
        case 1: header = "Ticker:"; break;
        case 2: header = "Name:"; break;
        case 3: header = "Sector:"; break;
        case 4: header = "Industry:"; break;
    }

    bool ok;
    QString newText = QInputDialog::getText(this,
                                         "Change " + header,
                                         header,
                                         QLineEdit::Normal,
                                         previousText,
                                         &ok);

    if (ok && !newText.isEmpty())
    {
        clickedItem->setText(newText);

        QVector<sISINDATA> isinList = database->getIsinList();
        QString ISIN = ui->tableISIN->item(row, 0)->text();

        auto it = std::find_if (isinList.begin(), isinList.end(),
                               [ISIN](sISINDATA a)
                               {
                                   return a.ISIN == ISIN;
                               } );

        if (it != isinList.end())
        {
            switch(column)
            {
                case 0: it->ISIN = newText;
                    break;

                case 1:
                {
                    it->ticker = newText;

                    if (previousText != newText)             // ticker was changed so it should be possible to update the data and not wait to the next day
                    {
                        it->lastUpdate = QDateTime();

                        QTableWidgetItem *dateItem = ui->tableISIN->item(row, 5);

                        if (dateItem != nullptr)
                        {
                            dateItem->setText("");
                        }
                    }
                }
                break;

                case 2: it->name = newText;
                    break;

                case 3: it->sector = newText;
                    break;

                case 4: it->industry = newText;
                    break;
            }

            database->setIsinList(isinList);
        }


        // The ticker has been updated, so check all stockData and assign ticker to the ISIN value
        if (column == 1)
        {
            StockDataType stockList = stockData->getStockData();

            QVector<sSTOCKDATA> vector = stockList.value(ISIN);

            QMutableVectorIterator i(vector);

            while(i.hasNext())
            {
                i.next();
                i.value().ticker = newText;
            }

            stockList[ISIN] = vector;
            stockData->setStockData(stockList);

            fillOverviewTable();
        }
    }
}

void MainWindow::updateStockDataSlot(QString ISIN, sONLINEDATA table)
{
    QVector<sISINDATA> isinList = database->getIsinList();

    auto it = std::find_if (isinList.begin(), isinList.end(), [ISIN](sISINDATA a)
                        {
                            return ISIN == a.ISIN;
                        }
                        );

    if (it != isinList.end())
    {
        it->lastUpdate = QDateTime::currentDateTime();

        it->sector = table.info.sector;
        it->industry = table.info.industry;

        database->setIsinList(isinList);

        fillISINTable();

        // First time
        /*if (it->sector.isEmpty() || it->industry.isEmpty())
        {
            it->sector = table.info.sector;
            it->industry = table.info.industry;

            database->setIsinList(isinList);

            fillISINTable();
        }
        else
        {
            database->setIsinList(isinList);
            fillISINTable();
        }*/

        fillOverviewTable();
    }
}


void MainWindow::on_pbUpdate_clicked()
{

}
