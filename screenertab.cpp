#include "screenertab.h"
#include "ui_screenertab.h"

ScreenerTab::ScreenerTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScreenerTab)
{
    ui->setupUi(this);
}

ScreenerTab::~ScreenerTab()
{
    delete ui;
}

QLabel *ScreenerTab::getHiddenRows()
{
    return ui->lbHidden;
}

QTableWidget *ScreenerTab::getScreenerTable()
{
    return ui->tableScreener;
}

sSCREENER ScreenerTab::getScreenerData() const
{
    return screenerData;
}

void ScreenerTab::setScreenerData(const sSCREENER &value)
{
    screenerData = value;
}
