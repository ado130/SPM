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

    ui->table->horizontalHeader()->setVisible(true);
    ui->table->verticalHeader()->setVisible(true);

    ui->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table->setShowGrid(true);
}

CustomCSVImportForm::~CustomCSVImportForm()
{
    if(loadedFile != nullptr)
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

        if(loadedFile != nullptr)
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

        if(skipedLines < skipLines)
        {
            skipedLines++;
            continue;
        }

        // Set header
        if(ui->cbHeader->isChecked() && !isFirstLine)
        {
            columnCount = setTableHeader(line, chDelimeter);
        }
        else if(!ui->cbHeader->isChecked() && !isFirstLine)
        {
            columnCount = setTableHeader(line.split(chDelimeter).count());
        }

        // Set first row
        if(!isFirstLine)
        {
            ui->table->insertRow(rowCount);
            for(int col = 0; col<columnCount; col++)
            {
                QTableWidgetItem *item = new QTableWidgetItem;
                item->setFlags(Qt::ItemIsEnabled);
                item->setData(Qt::EditRole, "DoubleClick here");
                item->setForeground(QBrush(Qt::gray));
                ui->table->setItem(0, col, item);
            }

            isFirstLine = true;
            rowCount++;
            continue;
        }

        ui->table->insertRow(rowCount);

        QStringList items = line.trimmed().split(chDelimeter);

        for(int col = 0; col<items.count(); ++col)
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
            //ui->table->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->table->resizeColumnsToContents();
}

int CustomCSVImportForm::setTableHeader(const QString &line, const char &delimeter)
{
    ui->table->setRowCount(0);

    QStringList header = line.trimmed().split(delimeter);
    ui->table->setColumnCount(header.count());
    ui->table->setHorizontalHeaderLabels(header);

    return header.count();
}

int CustomCSVImportForm::setTableHeader(const int &columns)
{
    ui->table->setRowCount(0);

    QStringList header;

    for(int col = 1; col<=columns; ++col)
    {
        header << QString("Column %1").arg(col);
    }

    ui->table->setColumnCount(header.count());
    ui->table->setHorizontalHeaderLabels(header);

    return header.count();
}


void CustomCSVImportForm::on_table_cellDoubleClicked(int row, int column)
{
    if(row != 0)
    {
        return;
    }

    QStringList items;
    items << tr("-") << tr("Date") << tr("Time") << tr("Ticker") << tr("ISIN") << tr("Currency") << tr("Value") << tr("Type");

    bool ok;
    QString item = QInputDialog::getItem(this,
                                         tr("Input"),
                                         tr("Type:"),
                                         items,
                                         0,
                                         false,
                                         &ok);
    if (ok && !item.isEmpty())
    {
        QTableWidgetItem *it = ui->table->item(row, column);

        if(it != nullptr)
        {
            it->setText(item);
        }
    }
}



























