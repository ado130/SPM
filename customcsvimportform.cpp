#include "customcsvimportform.h"
#include "ui_customcsvimportform.h"

CustomCSVImportForm::CustomCSVImportForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomCSVImportForm)
{
    ui->setupUi(this);

    loadedFile = nullptr;

    connect(ui->cmDelimeter, &QComboBox::currentIndexChanged, this, &CustomCSVImportForm::loadCSV);
    connect(ui->sbSkipLines, &QSpinBox::valueChanged, this, &CustomCSVImportForm::loadCSV);
    connect(ui->cbHeader, &QCheckBox::stateChanged, this, &CustomCSVImportForm::loadCSV);
    connect(ui->cmDateType, &QComboBox::currentIndexChanged, this, &CustomCSVImportForm::checkTableColumns);

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
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                        tr("Open CSV File"),
                                                        "",
                                                        tr("CSV files (*.csv)"));

    if(!filePath.isEmpty())
    {
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

void CustomCSVImportForm::loadCSV()
{
    if(loadedFile == nullptr)
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

            selectedColumnType.append(qMakePair(type, column));

            checkTableColumns();
        }
    }
}

void CustomCSVImportForm::checkTableColumns()
{
    for (int col = 0; col<ui->table->columnCount(); ++col)
    {
        auto it = std::find_if(selectedColumnType.begin(), selectedColumnType.end(), [col](QPair<eITEMTYPE, int> val)
                                            {
                                                return val.second == col;
                                            });
        eITEMTYPE columnType;

        if(it != selectedColumnType.end())
        {
            columnType = it->first;
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
                case NO:
                    break;
                case DATE:
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
                case TIME:
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
                case TICKER:
                    item->setBackground(Qt::green);
                    break;
                case ISIN:
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
                case CURRENCY:
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
                case VALUE:
                case FEE:
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
                case TYPE:
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



void CustomCSVImportForm::on_cmDateType_currentIndexChanged(int index)
{
    selectedDateType = index;
}
