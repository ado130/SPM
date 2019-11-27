#include "screenerform.h"
#include "ui_screenerform.h"



ScreenerForm::ScreenerForm(QVector<sSCREENERPARAM> params, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScreenerForm)
{
    ui->setupUi(this);

    screenerParams = params;

    fillList();
}

ScreenerForm::~ScreenerForm()
{
    delete ui;
}

void ScreenerForm::fillList()
{
    for(const sSCREENERPARAM &row : screenerParams)
    {
        if(row.name.isEmpty() || row.name.isNull() || row.name == "FINVIZ" || row.name == "YAHOO") continue;

        QListWidgetItem* item = new QListWidgetItem(row.name, ui->listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
        item->setCheckState(row.enabled ? Qt::Checked : Qt::Unchecked); // AND initialize check state

        if(item->text() == "Ticker")
        {
            item->setCheckState(Qt::Checked);
        }
    }
}

void ScreenerForm::on_buttonBox_accepted()
{
    screenerParams.clear();

    for(int a = 0; a<ui->listWidget->count(); ++a)
    {
        sSCREENERPARAM param;
        param.name = ui->listWidget->item(a)->text();
        param.enabled = ui->listWidget->item(a)->checkState() == Qt::Checked ? true : false;

        screenerParams.push_back(param);
    }

    emit setScreenerParams(screenerParams);
}

void ScreenerForm::on_listWidget_itemChanged(QListWidgetItem *item)
{
    QString text = item->text();

    if(text == "Ticker")
    {
        item->setCheckState(Qt::Checked);
    }
}
