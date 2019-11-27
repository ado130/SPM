#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsform.h"
#include "filterform.h"

#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QTableWidgetItem>
#include <QScreen>
#include <QDesktopWidget>
#include <QHash>
#include <QInputDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << QSslSocket::supportsSsl() << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::sslLibraryVersionString();

    if(!QSslSocket::supportsSsl())
    {
        QMessageBox::critical(this,
                              "SSL supports",
                              "The platform does not support the SSL, the application might not work correct!",
                              QMessageBox::Ok);
    }

    manager = std::make_shared<DownloadManager> (this);
    database = std::make_shared<Database> (this);
    degiro = std::make_shared<DeGiro> (this);
    screener = std::make_shared<Screener> (this);

    /********************************
     * Geometry
    ********************************/
    if(database->getSetting().width <= 0 || database->getSetting().height <= 0 || database->getSetting().xPos <= 0 || database->getSetting().yPos <= 0)
    {
        centerAndResize();

        sSETTINGS set = database->getSetting();
        set.width = this->geometry().width();
        set.height = this->geometry().height();
        database->setSetting(set);
    }
    else
    {
        QSize newSize( database->getSetting().width, database->getSetting().height);

        QPoint point = this->mapFromGlobal(QPoint(database->getSetting().xPos, database->getSetting().yPos));

        setGeometry(point.x(),
                    point.y(),
                    database->getSetting().width,
                    database->getSetting().height);
        /*setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                newSize,
                QGuiApplication::screens().first()->availableGeometry()
            )
        );*/

        //this->mapFromGlobal(QPoint(database->getSetting().xPos, database->getSetting().yPos));
    }

    ui->mainTab->setCurrentIndex(database->getSetting().lastOpenedTab);

    QString status = QString("Autor: Andrej Copyright © 2019; Version: %1").arg(VERSION_STR);
    ui->statusBar->showMessage(status);


    installEventFilter(this);


    /********************************
     * DeGiro table
    ********************************/
    setDegiroHeader();

    if(database->getSetting().autoload && degiro->getIsRAWFile())
    {
        fillDegiro();
    }


    /********************************
     * Screener table
    ********************************/
    ui->cbFilter->setChecked(database->getSetting().filterON);
    ui->pbFilter->setEnabled(database->getSetting().filterON);

    filterList = database->getFilterList();

    currentScreenerIndex = database->getLastScreenerIndex();

    if(currentScreenerIndex > -1)
    {
        QVector<sSCREENER> allData = screener->getAllScreenerData();

        if(allData.count() > currentScreenerIndex)
        {
            for(int a = 0; a<allData.count(); ++a)
            {
                ScreenerTab *st = new ScreenerTab(this);
                st->setScreenerData(allData.at(a));
                screenerTabs.append(st);

                ui->tabScreener->addTab(st, allData.at(a).screenerName);

                setScreenerHeader(st);
                fillScreener(st);
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


    if(database->getEnabledScreenerParams().count() == 0 || screenerTabs.count() == 0)
    {
        ui->pbAddTicker->setEnabled(false);
    }
}

void MainWindow::centerAndResize()
{
    // get the dimension available on this screen
    QSize availableSize = QGuiApplication::screens().first()->size();

    int width = availableSize.width();
    int height = availableSize.height();

    qDebug() << "Available dimensions " << width << "x" << height;

    width = static_cast<int>(width*0.75); // 90% of the screen size
    height = static_cast<int>(height*0.75); // 90% of the screen size

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
    database->setSetting(set);
}


void MainWindow::on_actionAbout_triggered()
{
    QString text;
    text =  "========================================\n";
    text += "Stock Portfolio Manager (SPM)\n\n";
    text += "Copyright © 2019 Stock Portfolio Manager\n\n";
    text += "Author: Andrej\n";
    text += "E-mail: vlasaty.andrej@gmail.com\n";
    text += "Website: https://www.investicnigramotnost.cz\n";
    text += "========================================\n";

    QMessageBox::about(this,
                       "About",
                       text);
}

void MainWindow::on_actionHelp_triggered()
{
    QString text;
    text = "Set the CSV path to the DeGiro file and delimeter in the Settings.\n";
    text += "Please load the parameters under the Settings window and select the order and the visibility.\n\n";
    text += "The filter windows allows to set one of the predefined filter:\n";
    text += "   1) f<\n";
    text += "   2) f>\n";
    text += "   3) <f;f>\n";
    text += "where the \"f\" means float number.\n\n";
    text += "The color column has to be either HEX color number or \"HIDE\".\n";
    text += "The color HEX palette as available under the context menu (right click).\n";


    QMessageBox::about(this,
                       "About",
                       text);
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsForm *dlg = new SettingsForm(database->getSetting(), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, SIGNAL(setSetting(sSETTINGS)), database.get(), SLOT(setSetting(sSETTINGS)));
    connect(dlg, &SettingsForm::setScreenerParams, this, &MainWindow::setScreenerParamsSlot);
    connect(dlg, &SettingsForm::loadOnlineParameters, this, &MainWindow::loadOnlineParametersSlot);
    connect(dlg, &SettingsForm::loadDegiroCSV, this, &MainWindow::loadDegiroCSVslot);
    connect(this, &MainWindow::updateScreenerParams, dlg, &SettingsForm::updateScreenerParamsSlot);
    dlg->show();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this,
                         "SPM");
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

        if ( (key->key()==Qt::Key_Enter) || (key->key()==Qt::Key_Return) )
        {
            if(!ui->leTicker->text().isEmpty())
            {
                ui->pbAddTicker->click();
            }
            else
            {
                setStatus("The ticker is empty!");
            }
        }
        else
        {
            return QObject::eventFilter(obj, event);
        }
        return true;
    }
    else if(event->type() == QEvent::Resize)
    {
        sSETTINGS set = database->getSetting();
        set.width = this->geometry().width();
        set.height = this->geometry().height();
        database->setSetting(set);

        return true;
    }
    else if(event->type() == QEvent::Move)
    {
        sSETTINGS set = database->getSetting();
        QPoint point = this->mapToGlobal(QPoint(0, 0));
        set.xPos = point.x();
        set.yPos = point.y();
        database->setSetting(set);

        return true;
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}


/********************************
*
*  DEGIRO
*
********************************/

void MainWindow::loadDegiroCSVslot()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    degiro->loadDegiroCSV(database->getSetting().degiroCSV, database->getSetting().CSVdelimeter);

    if(degiro->getIsRAWFile())
    {
        fillDegiro();
    }
    else
    {
        setStatus("The CSV DeGiro path is not set!");
    }

    QApplication::restoreOverrideCursor();
}

void MainWindow::on_pbDegiroLoad_clicked()
{
    if(degiro->getIsRAWFile())
    {
        fillDegiro();
    }
    else
    {
        setStatus("The CSV DeGiro path is not set!");
    }
}

void MainWindow::setDegiroHeader()
{
    QStringList header;
    header << "Date" << "Product" << "ISIN" << "Description" << "Currency" << "Value";
    ui->tableDegiro->setColumnCount(header.count());


    ui->tableDegiro->horizontalHeader()->setVisible(true);
    ui->tableDegiro->verticalHeader()->setVisible(true);

    ui->tableDegiro->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableDegiro->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableDegiro->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableDegiro->setShowGrid(true);

    ui->tableDegiro->setHorizontalHeaderLabels(header);
}

void MainWindow::fillDegiro()
{
    QVector<sDEGIRORAW> data = degiro->getDegiroRawData();

    if(data.isEmpty())
    {
        return;
    }

    ui->tableDegiro->setRowCount(0);

    ui->tableDegiro->setSortingEnabled(false);
    for(int a = 0; a<data.count(); ++a)
    {
        ui->tableDegiro->insertRow(a);
        ui->tableDegiro->setItem(a, 0, new QTableWidgetItem(data.at(a).dateTime.toString("dd.MM.yyyy hh:mm")));
        ui->tableDegiro->setItem(a, 1, new QTableWidgetItem(data.at(a).product));
        ui->tableDegiro->setItem(a, 2, new QTableWidgetItem(data.at(a).ISIN));
        ui->tableDegiro->setItem(a, 3, new QTableWidgetItem(data.at(a).description));
        ui->tableDegiro->setItem(a, 4, new QTableWidgetItem(database->getCurrencyText(data.at(a).currency)));
        ui->tableDegiro->setItem(a, 5, new QTableWidgetItem(QString::number(data.at(a).money, 'f', 2)));
    }
    ui->tableDegiro->setSortingEnabled(true);

    ui->tableDegiro->resizeColumnsToContents();

    for (int row = 0; row<ui->tableDegiro->rowCount(); ++row)
    {
        for(int col = 0; col<ui->tableDegiro->columnCount(); ++col)
        {
            ui->tableDegiro->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }
}


/********************************
*
*  SCREENER
*
********************************/
void MainWindow::loadOnlineParametersSlot()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    connect(manager.get(), SIGNAL(sendData(QByteArray, QString)), this, SLOT(parseOnlineParameters(QByteArray, QString)));
    lastRequestSource = FINVIZ;

    // Clean and save screener params
    QList<sSCREENERPARAM> screenerParams = database->getScreenerParams();
    screenerParams.clear();
    database->setScreenerParams(screenerParams);

    manager.get()->execute("https://finviz.com/quote.ashx?t=T");
}

void MainWindow::parseOnlineParameters(const QByteArray data, QString statusCode)
{
    disconnect(manager.get(), SIGNAL(sendData(QByteArray, QString)), this, SLOT(parseOnlineParameters(QByteArray, QString)));

    if(!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the request! %1").arg(statusCode));
        QApplication::restoreOverrideCursor();
    }
    else
    {
        sTABLE table;
        QList<sSCREENERPARAM> screenerParams = database->getScreenerParams();

        sSCREENERPARAM param;

        if(lastRequestSource == FINVIZ)
        {
            QStringList infoData;
            infoData << "Ticker" << "Stock name" << "Sector" << "Industry" << "Country";

            for(const QString &par : infoData)
            {
                param.name = par;
                param.enabled = true;
                screenerParams.append(param);
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
        screenerParams.append(param);

        for(const QString &key : table.row.keys())
        {
            param.name = key;
            param.enabled = false;
            screenerParams.append(param);
        }

        database->setScreenerParams(screenerParams);

        if(lastRequestSource == FINVIZ)
        {
            connect(manager.get(), SIGNAL(sendData(QByteArray, QString)), this, SLOT(parseOnlineParameters(QByteArray, QString)));
            lastRequestSource = YAHOO;
            manager.get()->execute("https://finance.yahoo.com/quote/T/key-statistics");
        }
        else        // end
        {       
            std::sort(screenerParams.begin(), screenerParams.end(), [](sSCREENERPARAM a, sSCREENERPARAM b) {return a.name < b.name; });

            QApplication::restoreOverrideCursor();
            database->setScreenerParams(screenerParams);
            emit updateScreenerParams(screenerParams);
            setStatus("Parameters have been loaded");

            if(screenerTabs.count() != 0)
            {
                ui->pbAddTicker->setEnabled(true);
            }
        }
    }
}

void MainWindow::setScreenerParamsSlot(QList<sSCREENERPARAM> params)
{
    database->setScreenerParams(params);

    for(int a = 0; a<screenerTabs.count(); ++a)
    {
        setScreenerHeader(screenerTabs.at(a));
        fillScreener(screenerTabs.at(a));
    }
}

void MainWindow::setScreenerHeader(ScreenerTab *st)
{
    if(!st) return;

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
    if(currentScreenerIndex == -1)
    {
        QMessageBox::warning(this,
                             "Screener",
                             "Please create at least one screener!",
                             QMessageBox::Ok);

        setStatus("Please create at least one screener!");

        return;
    }

    if(ui->leTicker->text().trimmed().isEmpty())
    {
        setStatus("The ticker field is empty!");
        return;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QString ticker = ui->leTicker->text().trimmed();
    ui->leTicker->setText(ticker.toUpper());

    temporaryLoadedTable.row.clear();
    lastRequestSource = FINVIZ;
    connect(manager.get(), SIGNAL(sendData(QByteArray, QString)), this, SLOT(getData(QByteArray, QString)));
    QString request = QString("https://finviz.com/quote.ashx?t=%1").arg(ticker);
    manager->execute(request);
}

void MainWindow::getData(const QByteArray data, QString statusCode)
{
    if(!statusCode.contains("200"))
    {
        qDebug() << QString("There is something wrong with the request! %1").arg(statusCode);
        setStatus(QString("There is something wrong with the request! %1").arg(statusCode));
        QApplication::restoreOverrideCursor();
    }
    else
    {
        QString ticker = ui->leTicker->text().trimmed();

        sTABLE table;

        switch (lastRequestSource)
        {
            case FINVIZ:
                table = screener->finvizParse(QString(data));
                table.info.ticker = ticker;
                temporaryLoadedTable.info = table.info;
                break;
            case YAHOO:
                table = screener->yahooParse(QString(data));
                break;
        }


        for(const QString &key : table.row.keys())
        {
            temporaryLoadedTable.row.insert(key, table.row.value(key));
        }

        if(lastRequestSource == FINVIZ)
        {
            lastRequestSource = YAHOO;
            QString request = QString("https://finance.yahoo.com/quote/%1/key-statistics").arg(ticker);
            manager->execute(request);
        }
        else    // end
        {
            QApplication::restoreOverrideCursor();
            disconnect(manager.get(), SIGNAL(sendData(QByteArray, QString)), this, SLOT(getData(QByteArray, QString)));
            dataLoaded();
            emit refreshTickers(ticker);
        }
    }
}

void MainWindow::dataLoaded()
{
    QStringList screenerParams = database->getEnabledScreenerParams();

    tickerDataType tickerLine;

    QString ticker = ui->leTicker->text().trimmed();

    QStringList infoData;
    infoData << "Ticker" << "Stock name" << "Sector" << "Industry" << "Country";

    bool bParamFound = false;

    for(int param = 0; param<screenerParams.count(); ++param)
    {
        bParamFound = false;

        for(int table = 0; table<temporaryLoadedTable.row.count() && !bParamFound; ++table)
        {
            for(int info = 0; info<infoData.count() && !bParamFound; ++info)
            {
                if(screenerParams.at(param).toLower() == infoData.at(info).toLower())
                {
                    if(infoData.at(info) == "Industry")
                    {
                        tickerLine.append(qMakePair(infoData.at(info), temporaryLoadedTable.info.industry));
                    }
                    else if(infoData.at(info) == "Ticker")
                    {
                        tickerLine.append(qMakePair(infoData.at(info), temporaryLoadedTable.info.ticker));
                    }
                    else if(infoData.at(info) == "Stock name")
                    {
                        tickerLine.append(qMakePair(infoData.at(info), temporaryLoadedTable.info.stockName));
                    }
                    else if(infoData.at(info) == "Sector")
                    {
                        tickerLine.append(qMakePair(infoData.at(info), temporaryLoadedTable.info.sector));
                    }
                    else if(infoData.at(info) == "Country")
                    {
                        tickerLine.append(qMakePair(infoData.at(info), temporaryLoadedTable.info.country));
                    }

                    bParamFound = true;
                }
            }
        }
    }


    bParamFound = false;

    for(int param = 0; param<screenerParams.count(); ++param)
    {
        bParamFound = false;

        for(int table = 0; table<temporaryLoadedTable.row.count() && !bParamFound; ++table)
        {
            if(temporaryLoadedTable.row.contains(screenerParams.at(param)))
            {
                tickerLine.append(qMakePair(screenerParams.at(param), temporaryLoadedTable.row.value(screenerParams.at(param))));

                bParamFound = true;
            }
        }
    }

    if(tickerLine.isEmpty()) return;

    if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    int tickerOrder = findScreenerTicker(ticker);

    if(tickerOrder == -2)
    {
        return;
    }
    else if(tickerOrder == -1)       // Ticker does not exist, so add it
    {
        currentScreenerData.screenerData.append(tickerLine);

        QVector<sSCREENER> allScreenerData = screener->getAllScreenerData();

        if(currentScreenerIndex < allScreenerData.count())
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

        if(currentScreenerIndex < allScreenerData.count())
        {
            allScreenerData[currentScreenerIndex] = currentScreenerData;
            screener->setAllScreenerData(allScreenerData);

            screenerTabs.at(currentScreenerIndex)->setScreenerData(currentScreenerData);

            setStatus(QString("Ticker %1 has been updated").arg(ticker));
        }

        int currentRowInTable = -1;

        if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;
        ScreenerTab *tab = screenerTabs.at(currentScreenerIndex);

        // Check if the filter is enabled, if so, some tickers might be hidden, so find correct row in the table for our ticker
        if(ui->cbFilter->isChecked())
        {
            bool found = false;

            for(int row = 0; row<tab->getScreenerTable()->rowCount() && !found; ++row)
            {
                for(int col = 0; col<tab->getScreenerTable()->columnCount()-1 && !found; ++col)
                {
                    if(tab->getScreenerTable()->item(row, col) && tab->getScreenerTable()->item(row, col)->text() == ui->leTicker->text())
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

        if(currentRowInTable != -1)
        {
            int pos = 0;

            for(int col = 0; col<currentScreenerData.screenerData.at(tickerOrder).count(); ++col)
            {
                for(int param = 0; param<screenerParams.count(); ++param)
                {
                    if(currentScreenerData.screenerData.at(tickerOrder).at(col).first == screenerParams.at(param))
                    {
                        QString text = tickerLine.at(col).second;

                        if(!tab->getScreenerTable()->item(currentRowInTable, param))      // the item does not exist, so create it
                        {
                            QTableWidgetItem *item = new QTableWidgetItem;

                            bool ok;
                            double testNumber = text.toDouble(&ok);

                            if(ok)
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
                            tab->getScreenerTable()->setSortingEnabled(false);

                            for(const sFILTER &filter : filterList)
                            {
                                if(filter.param == screenerParams.at(param))
                                {
                                    applyFilterOnItem(screenerTabs.at(currentScreenerIndex), item, filter);
                                }
                            }
                        }
                        else            // update the data
                        {
                            QTableWidgetItem *item = tab->getScreenerTable()->item(currentRowInTable, param);

                            if(item)
                            {
                                item->setText(text);

                                for(const sFILTER &filter : filterList)
                                {
                                    if(filter.param == screenerParams.at(param))
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
    if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return -2;

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    for(int row = 0; row<currentScreenerData.screenerData.count(); ++row)
    {
        for(int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
        {
            if(currentScreenerData.screenerData.at(row).at(col).second == ticker)
            {
                return row;
            }
        }
    }

    return -1;
}

void MainWindow::insertScreenerRow(tickerDataType tickerData)
{
    if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;
    ScreenerTab *st = screenerTabs.at(currentScreenerIndex);

    int row = st->getScreenerTable()->rowCount();
    st->getScreenerTable()->insertRow(row);

    QStringList screenerParams = database->getEnabledScreenerParams();

    for(int col = 0; col<tickerData.count(); ++col)
    {
        for(int param = 0; param<screenerParams.count(); ++param)
        {
            if(tickerData.at(col).first == screenerParams.at(param))
            {
                QString text = tickerData.at(col).second;
                QTableWidgetItem *item = new QTableWidgetItem();

                bool ok;
                double testNumber = text.toDouble(&ok);

                if(ok)
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

                for(const sFILTER &filter : filterList)
                {
                    if(filter.param == screenerParams.at(param))
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

void MainWindow::fillScreener(ScreenerTab *st)
{
    if(!st) return;

    sSCREENER currentScreenerData = st->getScreenerData();

    if(currentScreenerData.screenerData.isEmpty()) return;

    st->getScreenerTable()->setRowCount(0);

    QStringList screenerParams = database->getEnabledScreenerParams();

    bool nextRow = false;
    int hiddenRows = 0;

    for(int row = 0; row<currentScreenerData.screenerData.count(); ++row)
    {
        st->getScreenerTable()->insertRow(row-hiddenRows);

        for(int param = 0; param<screenerParams.count(); ++param)
        {
            for(int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
            {
                if(currentScreenerData.screenerData.at(row).at(col).first == screenerParams.at(param))
                {
                    bool isFilter = false;
                    QString color;
                    double val1 = 0.0;
                    double val2 = 0.0;
                    eFILTER filterType = LOWER;

                    if(ui->cbFilter->isChecked())
                    {
                        for(const sFILTER &filter : filterList)
                        {
                            if(filter.param == screenerParams.at(param))
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

                    if(percentSign != -1)
                    {
                        text = text.mid(0, percentSign);
                    }

                    QTableWidgetItem *item = new QTableWidgetItem;

                    bool ok;
                    double testNumber = text.toDouble(&ok);

                    if(ok)
                    {
                        item->setData(Qt::EditRole, testNumber);
                    }
                    else
                    {
                        item->setData(Qt::EditRole, text);
                    }

                    item->setTextAlignment(Qt::AlignCenter);

                    if(isFilter)
                    {
                        switch (filterType)
                        {
                            case LOWER:
                                if(text.toDouble() < val1)
                                {
                                    if(color == "HIDE")
                                    {
                                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                            case HIGHER:
                                if(text.toDouble() > val1)
                                {
                                    if(color == "HIDE")
                                    {
                                        st->getScreenerTable()->setRowCount(st->getScreenerTable()->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                            case BETWEEN:
                                if(text.toDouble() > val1 && text.toDouble() < val2)
                                {
                                    if(color == "HIDE")
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
        if(nextRow)
        {
            qDebug() << "Row hidden";
            hiddenRows++;
            nextRow = false;
        }
    }

    st->getHiddenRows()->setText(QString("Hidden rows: %1").arg(hiddenRows));

    st->getScreenerTable()->resizeColumnsToContents();
}

void MainWindow::applyFilter(ScreenerTab *st)
{
    if(!st) return;

    QTableWidget *tab = st->getScreenerTable();
    QStringList screenerParams = database->getEnabledScreenerParams();
    sSCREENER currentScreenerData = st->getScreenerData();

    int hiddenRows = 0;

    for(int row = 0; row<tab->rowCount(); ++row)
    {
        for(int param = 0; param<screenerParams.count(); ++param)
        {
            for(int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
            {
                for(const sFILTER &filter : filterList)
                {
                    if(filter.param == screenerParams.at(param) &&
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
    if(!item || !st)
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

    if(percentSign != -1)
    {
        text = text.mid(0, percentSign);
    }

    bool ok;
    double testNumber = text.toDouble(&ok);

    if(ok)
    {
        item->setData(Qt::EditRole, testNumber);
        switch (filterType)
        {
            case LOWER:
                if(text.toDouble() < val1)
                {
                    if(color == "HIDE")
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
                if(text.toDouble() > val1)
                {
                    if(color == "HIDE")
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
                if(text.toDouble() > val1 && text.toDouble() < val2)
                {
                    if(color == "HIDE")
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
        screenerTabs.append(st);
        st->setScreenerData(currentScreenerData);

        ui->tabScreener->addTab(st, currentScreenerData.screenerName);
        setScreenerHeader(st);
        ui->tabScreener->setCurrentIndex(currentScreenerIndex);

        if(screenerTabs.count() != 0)
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

    if(ret == QMessageBox::Yes)
    {
        if(currentScreenerIndex < 0) return;

        QVector<sSCREENER> allData = screener->getAllScreenerData();
        allData.removeAt(currentScreenerIndex);
        screener->setAllScreenerData(allData);

        screenerTabs.removeAt(currentScreenerIndex);
        ui->tabScreener->removeTab(currentScreenerIndex);

        currentScreenerIndex--;
        database->setLastScreenerIndex(currentScreenerIndex);

        if(currentScreenerIndex > -1)
        {
            sSCREENER currentScreenerData;
            currentScreenerData = allData.at(currentScreenerIndex);
            ui->tabScreener->setCurrentIndex(currentScreenerIndex);
        }
    }
}

void MainWindow::clickedScreenerTabSlot(int index)
{
    if(index < screenerTabs.count())
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
    dlg->show();
}

void MainWindow::on_cbFilter_clicked(bool checked)
{
    ui->pbFilter->setEnabled(checked);
    filterList = database->getFilterList();

    for(int a = 0; a<screenerTabs.count(); ++a)
    {
        fillScreener(screenerTabs.at(a));
    }
}

void MainWindow::setFilterSlot(QVector<sFILTER> list)
{
    database->setFilterList(list);
    filterList = list;

    for(int a = 0; a<screenerTabs.count(); ++a)
    {
        applyFilter(screenerTabs.at(a));
    }
}


void MainWindow::on_pbRefresh_clicked()
{
    if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    currentTickers.clear();

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    for(const tickerDataType &scr : currentScreenerData.screenerData)
    {
        for(int a = 0; a<scr.count(); ++a)
        {
            if(scr.at(a).first == "Ticker")
            {
                currentTickers << scr.at(a).second;
                break;
            }
        }
    }

    if(!currentTickers.isEmpty())
    {
        progressDialog = new QProgressDialog("Operation in progress", "Cancel", 0, currentTickers.count(), this);
        connect(progressDialog, &QProgressDialog::canceled, this, &MainWindow::refreshTickersCanceled);
        progressDialog->setMinimumDuration(0);
        progressDialog->setWindowModality(Qt::WindowModal);
        progressDialog->setValue(0);

        connect(this, SIGNAL(refreshTickers(QString)), this, SLOT(refreshTickersSlot(QString)));
        ui->leTicker->setText(currentTickers.first());

        ui->pbAddTicker->click();
    }
}

void MainWindow::refreshTickersSlot(QString ticker)
{
    int pos = currentTickers.indexOf(ticker);

    if(pos == -1)   // error
    {
        delete progressDialog;
        progressDialog = nullptr;

        disconnect(this, SIGNAL(refreshTickers(QString)), this, SLOT(refreshTickersSlot(QString)));
        setStatus("An error appeard during the refresh");
    }
    else if(pos == (currentTickers.count()-1))  // last
    {
        progressDialog->setValue(pos+1);

        delete progressDialog;
        progressDialog = nullptr;

        disconnect(this, SIGNAL(refreshTickers(QString)), this, SLOT(refreshTickersSlot(QString)));
        setStatus("All tickers have been refreshed");
    }
    else
    {
        progressDialog->setValue(pos+1);

        QString next = currentTickers.at(pos+1);

        ui->leTicker->setText(next);
        ui->pbAddTicker->click();
    }
}

void MainWindow::refreshTickersCanceled()
{
    disconnect(this, SIGNAL(refreshTickers(QString)), this, SLOT(refreshTickersSlot(QString)));
    progressDialog->setValue(currentTickers.count());

    delete progressDialog;
    progressDialog = nullptr;

    setStatus("The process has been canceled");
}

void MainWindow::on_pbDeleteTickers_clicked()
{    
    if(currentScreenerIndex >= screenerTabs.count() || currentScreenerIndex < 0) return;

    currentTickers.clear();

    sSCREENER currentScreenerData = screenerTabs.at(currentScreenerIndex)->getScreenerData();

    ScreenerTab *tab = screenerTabs.at(currentScreenerIndex);
    QStringList screenerParams = database->getEnabledScreenerParams();
    QString ticker;

    for(int tableRow = 0; tableRow<tab->getScreenerTable()->rowCount(); ++tableRow)
    {
        if(tab->getScreenerTable()->item(tableRow, tab->getScreenerTable()->columnCount()-1)->checkState() == Qt::Checked)
        {
            for(int row = 0; row<currentScreenerData.screenerData.count(); ++row)
            {
                for(int param = 0; param<screenerParams.count(); ++param)
                {
                    if(screenerParams.at(param) == "Ticker")
                    {
                        for(int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
                        {
                            if(currentScreenerData.screenerData.at(row).at(col).second == tab->getScreenerTable()->item(tableRow, param)->text())
                            {
                                currentScreenerData.screenerData.removeAt(row);

                                QVector<sSCREENER> allScreenerData = screener->getAllScreenerData();

                                if(currentScreenerIndex < allScreenerData.count())
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
