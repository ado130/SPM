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

    manager = std::make_shared<DownloadManager> (this);
    database = std::make_shared<Database> (this);
    degiro = std::make_shared<DeGiro> (this);
    screener = std::make_shared<Screener> (this);

    /********************************
     * Geometry
    ********************************/
    if(database->getSetting().width == 0 || database->getSetting().height == 0)
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

        setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                newSize,
                QGuiApplication::screens().first()->availableGeometry()
            )
        );
    }

    QString status = QString("Autor: Andrej Copyright © 2019; Version: %1").arg(VERSION_STR);
    ui->statusBar->showMessage(status);

    // Enter keyPress
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

    setScreenerHeader();
    currentScreenerIndex = database->getLastScreenerIndex();

    if(currentScreenerIndex != -1)
    {
        auto allData = screener->getAllScreenerData();

        if(allData.count() > currentScreenerIndex)
        {
            currentScreenerData = screener->getAllScreenerData().at(currentScreenerIndex);
            ui->lbScreenerName->setText(currentScreenerData.screenerName);
            fillScreener();
        }
        else
        {
            currentScreenerIndex = allData.count() - 1;
        }
    }

    ui->leScreenerIndex->setText(QString::number(currentScreenerIndex));
    database->setLastScreenerIndex(currentScreenerIndex);
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


void MainWindow::on_actionAbout_triggered()
{
    QString text;
    text =  "========================================\n";
    text += "........................Stock Portfolio Manager (SPM)...........................\n";
    text += "..................Copyright © 2019 Stock Portfolio Manager..........\n";
    text += ".......................https://www.investicnigramotnost.cz.................\n";
    text += "...............................................Andrej...............................................\n";
    text += "................................vlasaty.andrej@gmail.com..........................\n";
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

    QApplication::restoreOverrideCursor();
}

void MainWindow::on_pbDegiroLoad_clicked()
{
    if(degiro->getIsRAWFile())
    {
        fillDegiro();
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

    ui->tableScreener->setRowCount(0);

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

            foreach (QString par, infoData)
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

        foreach (QString key, table.row.keys())
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
            emit updateScreenerParams(screenerParams);
            setStatus("Parameters have been loaded");
        }
    }
}

void MainWindow::setScreenerParamsSlot(QList<sSCREENERPARAM> params)
{
    database->setScreenerParams(params);

    setScreenerHeader();
    fillScreener();
}

void MainWindow::setScreenerHeader()
{
    ui->tableScreener->setRowCount(0);

    QStringList header = database->getEnabledScreenerParams();

    header << "Delete";

    ui->tableScreener->setColumnCount(header.count());

    ui->tableScreener->horizontalHeader()->setVisible(true);
    ui->tableScreener->verticalHeader()->setVisible(true);

    ui->tableScreener->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableScreener->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableScreener->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableScreener->setShowGrid(true);

    ui->tableScreener->setHorizontalHeaderLabels(header);
}

void MainWindow::on_pbAddTicker_clicked()
{
    if(currentScreenerIndex == -1)
    {
        QMessageBox::warning(this,
                             "Screener",
                             "Please create at least one screener!",
                             QMessageBox::Ok);

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


        foreach (QString key, table.row.keys())
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
    int tickerOrder = findScreenerTicker(ticker);

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


    if(tickerOrder == -1)       // Ticker does not exist, so add it
    {
        currentScreenerData.screenerData.append(tickerLine);

        auto allScreenerData = screener->getAllScreenerData();

        if(currentScreenerIndex < allScreenerData.count())
        {
            allScreenerData[currentScreenerIndex] = currentScreenerData;
            screener->setAllScreenerData(allScreenerData);
        }

        insertScreenerRow(tickerLine);

        setStatus(QString("Ticker %1 has been added").arg(ticker));
    }
    else    // Ticker already exists
    {
        currentScreenerData.screenerData[tickerOrder] = tickerLine;

        auto allScreenerData = screener->getAllScreenerData();

        if(currentScreenerIndex < allScreenerData.count())
        {
            allScreenerData[currentScreenerIndex] = currentScreenerData;
            screener->setAllScreenerData(allScreenerData);

            setStatus(QString("Ticker %1 has been updated").arg(ticker));
        }

        QStringList screenerParams = database->getEnabledScreenerParams();

        int currentRowInTable = -1;

        // Check if the filter is enabled, if so, some tickers might be hidden, so find correct row in the table for our ticker
        if(ui->cbFilter->isChecked())
        {
            bool found = false;

            for(int row = 0; row<ui->tableScreener->rowCount() && !found; ++row)
            {
                for(int col = 0; col<ui->tableScreener->columnCount()-1 && !found; ++col)
                {
                    if(ui->tableScreener->item(row, col) && ui->tableScreener->item(row, col)->text() == ui->leTicker->text())
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

                        if(!ui->tableScreener->item(currentRowInTable, param))      // the item does not exist, so create it
                        {
                            QTableWidgetItem *item = new QTableWidgetItem(text);
                            item->setTextAlignment(Qt::AlignCenter);
                            ui->tableScreener->setItem(currentRowInTable, param, item);
                        }
                        else            // update the data
                        {
                            ui->tableScreener->item(currentRowInTable, param)->setText(text);
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
    auto allData = screener->getAllScreenerData();

    if(allData.count() > currentScreenerIndex)
    {
        for(int row = 0; row<allData.at(currentScreenerIndex).screenerData.count(); ++row)
        {
            for(int col = 0; col<allData.at(currentScreenerIndex).screenerData.at(row).count(); ++col)
            {
                if(allData.at(currentScreenerIndex).screenerData.at(row).at(col).second == ticker)
                {
                    return row;
                }
            }
        }
    }

    return -1;
}

void MainWindow::insertScreenerRow(tickerDataType ticerData)
{
    int row = ui->tableScreener->rowCount();
    ui->tableScreener->insertRow(row);

    if(currentScreenerData.screenerData.isEmpty() || ticerData.isEmpty()) return;

    QStringList screenerParams = database->getEnabledScreenerParams();

    int pos = 0;
    for(int col = 0; col<ticerData.count(); ++col)
    {
        for(int param = 0; param<screenerParams.count(); ++param)
        {
            if(ticerData.at(col).first == screenerParams.at(param))
            {
                QString text = ticerData.at(col).second;
                QTableWidgetItem *item = new QTableWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                ui->tableScreener->setItem(row, pos, item);
                pos++;
                break;
            }
        }
    }

    QTableWidgetItem *item = new QTableWidgetItem("Delete");
    item->setCheckState(Qt::Unchecked);
    item->setTextAlignment(Qt::AlignCenter);
    ui->tableScreener->setItem(ui->tableScreener->rowCount()-1, ui->tableScreener->columnCount()-1, item);

    ui->tableScreener->resizeColumnsToContents();
}

void MainWindow::fillScreener()
{
    ui->tableScreener->setRowCount(0);

    if(currentScreenerData.screenerData.isEmpty()) return;

    QStringList screenerParams = database->getEnabledScreenerParams();

    bool nextRow = false;
    int hiddenRows = 0;

    for(int row = 0; row<currentScreenerData.screenerData.count(); ++row)
    {
        ui->tableScreener->insertRow(row-hiddenRows);

        for(int col = 0; col<currentScreenerData.screenerData.at(row).count(); ++col)
        {
            for(int param = 0; param<screenerParams.count(); ++param)
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
                        Q_FOREACH(sFILTER filter, filterList)
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
                    QTableWidgetItem *item = new QTableWidgetItem(text);
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
                                        qDebug() << ui->tableScreener->rowCount();
                                        ui->tableScreener->setRowCount(ui->tableScreener->rowCount()-1);
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
                                        ui->tableScreener->setRowCount(ui->tableScreener->rowCount()-1);
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
                                        qDebug() << ui->tableScreener->rowCount();
                                        ui->tableScreener->setRowCount(ui->tableScreener->rowCount()-1);
                                        nextRow = true;
                                        goto nextRow;
                                    }

                                    item->setBackground(QColor("#" + color));
                                }
                                break;
                        }
                    }

                    ui->tableScreener->setItem(row - hiddenRows, param, item);

                    break;
                }
            }
        }

        {   // because of goto
            QTableWidgetItem *item = new QTableWidgetItem("Delete");
            item->setCheckState(Qt::Unchecked);
            item->setTextAlignment(Qt::AlignCenter);
            ui->tableScreener->setItem(ui->tableScreener->rowCount()-1, ui->tableScreener->columnCount()-1, item);
        }

        nextRow:
        if(nextRow)
        {
            qDebug() << "Row hidden";
            hiddenRows++;
            nextRow = false;
        }
    }

    ui->lbHidden->setText(QString("Hidden rows: %1").arg(hiddenRows));

    ui->tableScreener->resizeColumnsToContents();
}

void MainWindow::on_pbLeftScreener_clicked()
{
    if(currentScreenerIndex > 0)
    {
        currentScreenerIndex--;
        database->setLastScreenerIndex(currentScreenerIndex);

        auto allData = screener->getAllScreenerData();
        currentScreenerData = allData.at(currentScreenerIndex);
        ui->lbScreenerName->setText(currentScreenerData.screenerName);
        ui->leScreenerIndex->setText(QString::number(currentScreenerIndex));

        fillScreener();
    }
}

void MainWindow::on_pbRightScreener_clicked()
{
    auto allData = screener->getAllScreenerData();

    if(currentScreenerIndex < allData.count() - 1)
    {
        currentScreenerIndex++;
        database->setLastScreenerIndex(currentScreenerIndex);

        currentScreenerData = allData.at(currentScreenerIndex);
        ui->lbScreenerName->setText(currentScreenerData.screenerName);
        ui->leScreenerIndex->setText(QString::number(currentScreenerIndex));

        fillScreener();
    }
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
        ui->leScreenerIndex->setText(QString::number(currentScreenerIndex));

        currentScreenerData.screenerName = text;
        currentScreenerData.screenerData.clear();
        ui->lbScreenerName->setText(text);

        auto allData = screener->getAllScreenerData();
        allData.push_back(currentScreenerData);
        screener->setAllScreenerData(allData);

        fillScreener();
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

        auto allData = screener->getAllScreenerData();
        allData.removeAt(currentScreenerIndex);
        screener->setAllScreenerData(allData);

        currentScreenerIndex--;
        database->setLastScreenerIndex(currentScreenerIndex);
        ui->leScreenerIndex->setText(QString::number(currentScreenerIndex));

        if(currentScreenerIndex != -1)
        {
            currentScreenerData = allData.at(currentScreenerIndex);
            ui->lbScreenerName->setText(currentScreenerData.screenerName);
        }
        else
        {
            currentScreenerData.screenerData.clear();
            ui->lbScreenerName->setText("No Screener");
        }

        fillScreener();
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
    fillScreener();
}

void MainWindow::setFilterSlot(QVector<sFILTER> list)
{
    database->setFilterList(list);
    filterList = list;
    fillScreener();
}

void MainWindow::on_pbRefresh_clicked()
{
    currentTickers.clear();

    Q_FOREACH(tickerDataType scr, currentScreenerData.screenerData)
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
        progressDialog = new QProgressDialog("Operation in progress.", "Cancel", 0, currentTickers.count(), this);
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
    QString ticker;

    for(int tableRow = 0; tableRow<ui->tableScreener->rowCount(); ++tableRow)
    {
        if(ui->tableScreener->item(tableRow, ui->tableScreener->columnCount()-1)->checkState() == Qt::Checked)
        {
            Q_FOREACH(tickerDataType scr, currentScreenerData.screenerData)
            for(int row = 0; row<currentScreenerData.screenerData.count(); ++row)
            {
                for(int col = 0; col<scr.count(); ++col)
                {
                    if(scr.at(col).first == "Ticker" && currentScreenerData.screenerData.at(row).at(col).second == ui->tableScreener->item(tableRow, col)->text())
                    {
                        currentScreenerData.screenerData.removeAt(row);

                        auto allScreenerData = screener->getAllScreenerData();

                        if(currentScreenerIndex < allScreenerData.count())
                        {
                            allScreenerData[currentScreenerIndex] = currentScreenerData;
                            screener->setAllScreenerData(allScreenerData);
                        }

                        break;
                    }
                }
            }

            ui->tableScreener->removeRow(tableRow);

            if(tableRow > 0)
            {
                tableRow--;
            }
        }
    }
}
