#include "customcsvimportform.h"
#include "ui_customcsvimportform.h"

CustomCSVImportForm::CustomCSVImportForm(eCUSTOMCSVACTION action, QWidget *parent, StockDataType *data) :
    QDialog(parent),
    ui(new Ui::CustomCSVImportForm)
{
    ui->setupUi(this);

    this->action = action;

    switch (action)
    {
        case IMPORTCSV:
        {
            ui->pbLoad->setText(tr("Load"));
        }
        break;

        case EXPORTCSV:
        {
            ui->pbLoad->setText(tr("Export"));
            ui->label_2->setVisible(false);
            ui->sbSkipLines->setVisible(false);
            ui->label_3->setVisible(false);
            ui->cmDateType->setVisible(false);
            fillTable(data);
        }
        break;
    }

    loadedFile = nullptr;

    connect(ui->cmDelimeter, &QComboBox::currentIndexChanged, this, &CustomCSVImportForm::loadCSV);
    connect(ui->sbSkipLines, &QSpinBox::valueChanged, this, &CustomCSVImportForm::loadCSV);
    connect(ui->cbHeader, &QCheckBox::stateChanged, this, &CustomCSVImportForm::loadCSV);

    connect(ui->cmDateType,
            &QComboBox::currentIndexChanged,
            [this](const int &index)
            {
                selectedDateType = index;
                validateTableColumns();
            });


    ui->table->horizontalHeader()->setVisible(true);
    ui->table->verticalHeader()->setVisible(true);

    ui->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table->setShowGrid(true);

    itemTypes.append("---");
    itemTypes.append("Date");
    itemTypes.append("Time");
    itemTypes.append("Ticker");
    itemTypes.append("ISIN");
    itemTypes.append("Currency");
    itemTypes.append("Value");
    itemTypes.append("Fee");
    itemTypes.append("Type");

    dateTypes.append("yyyy?MM?dd");
    dateTypes.append("yy?MM?dd");
    dateTypes.append("yyyy?M?dd");
    dateTypes.append("yy?M?dd");
    dateTypes.append("yyyy?MM?d");
    dateTypes.append("yy?MM?d");
    dateTypes.append("MM?dd?yyyy");
    dateTypes.append("MM?dd?yy");
    dateTypes.append("M?dd?yyyy");
    dateTypes.append("M?dd?yy");
    dateTypes.append("MM?d?yyyy");
    dateTypes.append("MM?d?yy");
    dateTypes.append("dd?MM?yyyy");
    dateTypes.append("dd?MM?yy");
    dateTypes.append("dd?M?yyyy");
    dateTypes.append("dd?M?yy");
    dateTypes.append("d?MM?yyyy");
    dateTypes.append("d?MM?yy");
    dateTypes.append("yyyyMMdd");
    dateTypes.append("yyyyMdd");
    dateTypes.append("yyyyMMd");
    dateTypes.append("yyMMdd");
    dateTypes.append("yyMdd");
    dateTypes.append("yyMMd");

    typeTypes.append("Buy (B)");
    typeTypes.append("Sell (S)");
    typeTypes.append("Dividend (D)");
    typeTypes.append("Deposit (De)");
    typeTypes.append("Withdrawal (W)");
    typeTypes.append("Fee (F)");
    typeTypes.append("Dividend (Di)");
    typeTypes.append("Tax (T)");

    ui->cmDateType->addItems(dateTypes);
}

CustomCSVImportForm::~CustomCSVImportForm()
{
    if (loadedFile != nullptr)
    {
        delete loadedFile;
    }

    delete ui;
}

void CustomCSVImportForm::on_pbLoad_clicked()
{
    switch (action)
    {
        case IMPORTCSV:
        {
            const QString filePath = QFileDialog::getOpenFileName(this,
                                                                  tr("Open CSV File"),
                                                                  "",
                                                                  tr("CSV files (*.csv)"));

            if (!filePath.isEmpty()) {
                ui->lbFilePath->setText(filePath);

                if (loadedFile != nullptr)
                {
                    delete loadedFile;
                }

                loadedFile = new QFile(filePath);

                if (!loadedFile->open(QIODevice::ReadOnly))
                {
                    qDebug() << loadedFile->errorString();
                    return;
                }

                loadCSV();
            }
        }
        break;

    case EXPORTCSV:
        {
            QString fileName = QFileDialog::getSaveFileName(this,
                                                            tr("Save File"),
                                                            "",
                                                            tr("Excel (*.csv)"));

            if(!fileName.isEmpty())
            {
                saveCSV(fileName);
            }
        }
        break;

    }


}

void CustomCSVImportForm::saveCSV(const QString &fileName)
{
    const eDELIMETER delimeter = static_cast<eDELIMETER>(ui->cmDelimeter->currentIndex());

    char chDelimeter = ',';
    switch (delimeter)
    {
        case COMMA_SEPARATED:
            chDelimeter = ','; break;
        case SEMICOLON_SEPARATED:
            chDelimeter = ';'; break;
        case POINT_SEPARATED:
            chDelimeter = '.'; break;
    }

    QFile data(fileName);

    if(data.open(QFile::WriteOnly |QFile::Truncate))
    {
        QTextStream output(&data);

        if(ui->cbHeader->isChecked())
        {
            const QStringList header( {tr("Date"), tr("Time"), tr("Ticker"), tr("ISIN"), tr("Name"), tr("Currency"), tr("Value"), tr("Fee"), tr("Type")} );

            for (const QString &str : header)
            {
                output << str << chDelimeter;
            }
        }

        output << '\n';

        for (const sSTOCKDATA &stock : qAsConst(exportTableData))
        {
            QString curr;
            switch (stock.currency)
            {
                case CZK: curr = "CZK"; break;
                case EUR: curr = "EUR"; break;
                case USD: curr = "USD"; break;
                case GBP: curr = "GBP"; break;
            }

            QString type;
            switch (stock.type)
            {
                case DEPOSIT:
                    type = "Deposit";
                    break;
                case WITHDRAWAL:
                    type = "Withdrawal";
                    break;
                case BUY:
                    type = "Buy";
                    break;
                case SELL:
                    type = "Sell";
                    break;
                case FEE:
                    type = "Fee";
                    break;
                case DIVIDEND:
                    type = "Dividend";
                    break;
                case TAX:
                    type = "Tax";
                    break;
                case TRANSACTIONFEE:
                    type = "Transactionfee";
                    break;
                case CURRENCYEXCHANGE:
                    type = "Currency exchange";
                    break;
            }

            output << stock.dateTime.date().toString("dd.MM.yyyy") << chDelimeter;
            output << stock.dateTime.time().toString("hh:mm") << chDelimeter;
            output << stock.ticker << chDelimeter;
            output << stock.ISIN << chDelimeter;
            output << stock.stockName << chDelimeter;
            output << curr << chDelimeter;
            output << stock.price << chDelimeter;
            output << stock.fee << chDelimeter;
            output << type << chDelimeter;
            output << '\n';
        }
    }

    data.close();
}

void CustomCSVImportForm::loadCSV()
{
    if (loadedFile == nullptr)
    {
        return;
    }

    const eDELIMETER delimeter = static_cast<eDELIMETER>(ui->cmDelimeter->currentIndex());

    char chDelimeter = ',';
    switch (delimeter)
    {
        case COMMA_SEPARATED:
            chDelimeter = ','; break;
        case SEMICOLON_SEPARATED:
            chDelimeter = ';'; break;
        case POINT_SEPARATED:
            chDelimeter = '.'; break;
    }

    bool isFirstLine = false;
    int skipedLines = 0;
    const int skipLines = ui->sbSkipLines->value();

    int columnCount = 0;
    int rowCount = 0;

    loadedFile->seek(0);

    ui->table->setRowCount(0);
    ui->table->setSortingEnabled(false);

    while (!loadedFile->atEnd())
    {
        QString line = loadedFile->readLine();

        if (skipedLines < skipLines)
        {
            skipedLines++;
            continue;
        }

        // Set header
        if (ui->cbHeader->isChecked() && !isFirstLine)
        {
            columnCount = setTableHeader(line, chDelimeter);
        }
        else if (!ui->cbHeader->isChecked() && !isFirstLine)
        {
            columnCount = setTableHeader(line.split(chDelimeter).count());
        }

        // Set first row
        if (!isFirstLine)
        {
            ui->table->insertRow(rowCount);
            for (int col = 0; col<columnCount; col++)
            {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setFlags(Qt::ItemIsEnabled);
                item->setData(Qt::EditRole, "Double click here");
                item->setForeground(QBrush(Qt::gray));
                ui->table->setItem(0, col, item);
            }

            isFirstLine = true;
            rowCount++;
            continue;
        }

        ui->table->insertRow(rowCount);

        //QStringList items = line.trimmed().split(chDelimeter);
        QStringList items = parseLine(line, chDelimeter);

        for (int col = 0; col<items.count(); ++col)
        {
            ui->table->setItem(rowCount, col, new QTableWidgetItem(items.at(col)));
        }



        rowCount++;
    }
    ui->table->setSortingEnabled(true);

    for (int row = 0; row<ui->table->rowCount(); ++row)
    {
        for (int col = 0; col<ui->table->columnCount(); ++col)
        {
            ui->table->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->table->resizeColumnsToContents();
}

QStringList CustomCSVImportForm::parseLine(QString line, char delimeter)
{
    QStringList list;

    int pos = 0;
    int index = line.indexOf(delimeter, pos);
    list << line.mid(pos, index-pos);

    do
    {
        if (line.at(index+1) == ("\""))
        {
            pos = index + 2;
            index = line.indexOf("\"", pos);
            list << line.mid(pos, index-pos);
            index += 1;
        }
        else
        {
            pos = index + 1;
            index = line.indexOf(delimeter, pos);
            list << line.mid(pos, index-pos);
        }
    }while(index != -1);

    return list;
}

int CustomCSVImportForm::setTableHeader(const QString &line, const char &delimeter)
{
    ui->table->setRowCount(0);

    const QStringList header = line.trimmed().split(delimeter);
    ui->table->setColumnCount(header.count());
    ui->table->setHorizontalHeaderLabels(header);

    return header.count();
}

int CustomCSVImportForm::setTableHeader(const int &columns)
{
    ui->table->setRowCount(0);

    QStringList header;

    for (int col = 1; col<=columns; ++col)
    {
        header << QString("Column %1").arg(col);
    }

    ui->table->setColumnCount(header.count());
    ui->table->setHorizontalHeaderLabels(header);

    return header.count();
}

void CustomCSVImportForm::on_table_cellDoubleClicked(int row, int column)
{
    if (row != 0)
    {
        return;
    }

    bool ok;
    QString item = QInputDialog::getItem(this,
                                         tr("Input"),
                                         tr("Type:"),
                                         itemTypes,
                                         0,
                                         false,
                                         &ok);

    if (ok && !item.isEmpty())
    {
        QTableWidgetItem *it = ui->table->item(row, column);

        if (it != nullptr)
        {
            it->setText(item);
            it->setForeground(QBrush(Qt::black));

            const int pos = itemTypes.indexOf(item);
            const eITEMTYPE type = static_cast<eITEMTYPE>(pos);

            selectedColumnType.append( {column, type} );

            validateTableColumns();
        }
    }
}

void CustomCSVImportForm::validateTableColumns()
{
    for (int col = 0; col<ui->table->columnCount(); ++col)
    {
        auto it = std::find_if(selectedColumnType.begin(), selectedColumnType.end(), [col](sCOLUMNTYPE val)
                                            {
                                                return val.column == col;
                                            });
        eITEMTYPE columnType;

        if(it != selectedColumnType.end())
        {
            columnType = it->type;
        }
        else
        {
            continue;
        }

        for (int row = 1; row<ui->table->rowCount(); ++row)
        {
            QTableWidgetItem *item = ui->table->item(row, col);

            if(item == nullptr)
            {
                continue;
            }

            QString text = item->text().trimmed();
            int pos = 0;

            switch (columnType)
            {
                case ITEMNO:
                    break;
                case ITEMDATE:
                    {
                        if (QDate::fromString(text, dateTypes[selectedDateType].replace('?', '.')).isValid() ||
                            QDate::fromString(text, dateTypes[selectedDateType].replace('?', '/')).isValid() ||
                            QDate::fromString(text, dateTypes[selectedDateType].replace('?', '-')).isValid())
                        {
                            item->setBackground(Qt::green);
                        }
                        else
                        {
                            item->setBackground(Qt::red);
                        }
                    }
                    break;
                case ITEMTIME:
                    {
                        if (QTime::fromString(text, "hh:mm").isValid() || QTime::fromString(text, "hh:mm:ss").isValid())
                        {
                            item->setBackground(Qt::green);
                        }
                        else
                        {
                            item->setBackground(Qt::red);
                        }
                    }
                    break;
                case ITEMTICKER:
                    item->setBackground(Qt::green);
                    break;
                case ITEMISIN:
                    {
                        QRegExp rx("[a-zA-Z]{2}[a-zA-Z0-9]{1,10}");
                        QRegExpValidator v(rx, 0);

                        if(v.validate(text, pos) == QValidator::Acceptable)
                        {
                            item->setBackground(Qt::green);
                        }
                        else
                        {
                            item->setBackground(Qt::red);
                        }
                    }
                    break;
                case ITEMCURRENCY:
                    {
                        QRegExp rx("[a-zA-Z]{3}");
                        QRegExpValidator v(rx, 0);

                        if(v.validate(text, pos) == QValidator::Acceptable)
                        {
                            item->setBackground(Qt::green);
                        }
                        else
                        {
                            item->setBackground(Qt::red);
                        }
                    }
                    break;
                case ITEMVALUE:
                case ITEMFEE:
                    {
                        QRegExp rx("[+-]?[0-9]+[.]?[0-9]*[,]?[0-9]*");
                        QRegExpValidator v(rx, 0);

                        if(v.validate(text, pos) == QValidator::Acceptable)
                        {
                            item->setBackground(Qt::green);
                        }
                        else
                        {
                            item->setBackground(Qt::red);
                        }
                    }
                    break;
                case ITEMTYPE:
                    {
                        if(typeTypes.contains(text, Qt::CaseInsensitive))
                        {
                           item->setBackground(Qt::green);
                        }
                    }
                    break;
            }
        }
    }
}

void CustomCSVImportForm::fillTable(StockDataType *data)
{
    exportTableData.clear();

    for (const auto &it : *qAsConst(data))
    {
        exportTableData.append(it);
    }

    std::sort(exportTableData.begin(), exportTableData.end(), [](sSTOCKDATA &a, sSTOCKDATA &b)
              {
                  return a.dateTime > b.dateTime;
              });

    exportTableData.erase(std::remove_if(exportTableData.begin(), exportTableData.end(),
                          [](const sSTOCKDATA &x)
                          {
                            return x.stockName.toLower().contains("fundshare");
                          }),
                          exportTableData.end());

    ui->table->setRowCount(0);

    const QStringList header( {tr("Date"), tr("Time"), tr("Ticker"), tr("ISIN"), tr("Name"), tr("Currency"), tr("Value"), tr("Fee"), tr("Type")} );
    ui->table->setColumnCount(header.count());
    ui->table->setHorizontalHeaderLabels(header);

    int pos = 0;

    for (const sSTOCKDATA &stock : qAsConst(exportTableData))
    {
        ui->table->insertRow(pos);

        QString curr;
        switch (stock.currency)
        {
            case CZK: curr = "CZK"; break;
            case EUR: curr = "EUR"; break;
            case USD: curr = "USD"; break;
            case GBP: curr = "GBP"; break;
        }

        QString type;
        switch (stock.type)
        {
            case DEPOSIT:
                type = "Deposit";
                break;
            case WITHDRAWAL:
                type = "Withdrawal";
                break;
            case BUY:
                type = "Buy";
                break;
            case SELL:
                type = "Sell";
                break;
            case FEE:
                type = "Fee";
                break;
            case DIVIDEND:
                type = "Dividend";
                break;
            case TAX:
                type = "Tax";
                break;
            case TRANSACTIONFEE:
                type = "Transactionfee";
                break;
            case CURRENCYEXCHANGE:
                type = "Currency exchange";
                break;
        }

        ui->table->setItem(pos, 0, new QTableWidgetItem(stock.dateTime.date().toString("dd.MM.yyyy")));
        ui->table->setItem(pos, 1, new QTableWidgetItem(stock.dateTime.time().toString("hh:mm")));
        ui->table->setItem(pos, 2, new QTableWidgetItem(stock.ticker));
        ui->table->setItem(pos, 3, new QTableWidgetItem(stock.ISIN));
        ui->table->setItem(pos, 4, new QTableWidgetItem(stock.stockName));
        ui->table->setItem(pos, 5, new QTableWidgetItem(curr));
        ui->table->setItem(pos, 6, new QTableWidgetItem(QString::number(stock.price)));
        ui->table->setItem(pos, 7, new QTableWidgetItem(QString::number(stock.fee)));
        ui->table->setItem(pos, 8, new QTableWidgetItem(type));

        pos++;
    }

    ui->table->setSortingEnabled(false);

    for (int row = 0; row<ui->table->rowCount(); ++row)
    {
        for (int col = 0; col<ui->table->columnCount(); ++col)
        {
            ui->table->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->table->resizeColumnsToContents();
    ui->table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

}
