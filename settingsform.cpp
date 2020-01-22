#include "settingsform.h"
#include "ui_settingsform.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QIntValidator>

SettingsForm::SettingsForm(sSETTINGS set, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsForm)
{
    ui->setupUi(this);

    setting = set;

    ui->leWidth->setValidator(new QIntValidator(0, 4096, this));
    ui->leHeight->setValidator(new QIntValidator(0, 2160, this));
    ui->lePosX->setValidator(new QIntValidator(0, 4096, this));
    ui->lePosY->setValidator(new QIntValidator(0, 2160, this));
    ui->cmCurrency->setCurrentIndex(static_cast<int>(setting.currency));

    ui->leWidth->setText(QString::number(setting.width));
    ui->leHeight->setText(QString::number(setting.height));
    ui->lePosX->setText(QString::number(setting.xPos));
    ui->lePosY->setText(QString::number(setting.yPos));

    ui->leDegiroCSV->setText(setting.degiroCSV);
    ui->cmDegiroCSV->setCurrentIndex(setting.degiroCSVdelimeter);
    ui->cbDegiroAutoLoad->setChecked(setting.degiroAutoLoad);

    ui->leTastyworksCSV->setText(setting.tastyworksCSV);
    ui->cmTastyworksCSV->setCurrentIndex(setting.tastyworksCSVdelimeter);
    ui->cbTastyworksAutoLoad->setChecked(setting.tastyworksAutoLoad);

    ui->cbFilterON->setChecked(setting.filterON);
    ui->cbStartReload->setChecked(setting.screenerAutoLoad);

    ui->leLastUpdate->setText(setting.lastExchangeRatesUpdate.toString("ddd dd.MM.yyyy"));
    ui->leCZK2USD->setText(QString::number(setting.CZK2USD, 'f', 2));
    ui->leCZK2EUR->setText(QString::number(setting.CZK2EUR, 'f', 2));
    ui->leCZK2GBP->setText(QString::number(setting.CZK2GBP, 'f', 2));

    ui->leEUR2USD->setText(QString::number(setting.EUR2USD, 'f', 2));
    ui->leEUR2CZK->setText(QString::number(setting.EUR2CZK, 'f', 2));
    ui->leEUR2GBP->setText(QString::number(setting.EUR2GBP, 'f', 2));

    ui->leUSD2CZK->setText(QString::number(setting.USD2CZK, 'f', 2));
    ui->leUSD2GBP->setText(QString::number(setting.USD2GBP, 'f', 2));
    ui->leUSD2EUR->setText(QString::number(setting.USD2EUR, 'f', 2));

    ui->leGBP2CZK->setText(QString::number(setting.GBP2CZK, 'f', 2));
    ui->leGBP2USD->setText(QString::number(setting.GBP2USD, 'f', 2));
    ui->leGBP2EUR->setText(QString::number(setting.GBP2EUR, 'f', 2));

    ui->leEUR2CZKDAP->setText(QString::number(setting.EUR2CZKDAP, 'f', 2));
    ui->leUSD2CZKDAP->setText(QString::number(setting.USD2CZKDAP, 'f', 2));
    ui->leGBP2CZKDAP->setText(QString::number(setting.GBP2CZKDAP, 'f', 2));

    ui->tabWidget->setCurrentIndex(0);
}

SettingsForm::~SettingsForm()
{
    delete ui;
}

void SettingsForm::on_buttonBox_accepted()
{
    setting.degiroCSVdelimeter = static_cast<eDELIMETER>(ui->cmDegiroCSV->currentIndex());
    setting.degiroAutoLoad = ui->cbDegiroAutoLoad->isChecked();

    setting.screenerAutoLoad = ui->cbStartReload->isChecked();

    setting.width = ui->leWidth->text().toInt();
    setting.height = ui->leHeight->text().toInt();
    setting.xPos = ui->lePosX->text().toInt();
    setting.yPos = ui->lePosY->text().toInt();

    setting.EUR2CZKDAP = ui->leEUR2CZKDAP->text().toDouble();
    setting.USD2CZKDAP = ui->leUSD2CZKDAP->text().toDouble();
    setting.GBP2CZKDAP = ui->leGBP2CZKDAP->text().toDouble();

    emit setSetting(setting);
}


void SettingsForm::on_pbLoadParameters_clicked()
{
    int ret = QMessageBox::warning(this,
                                   "Screener parameters",
                                   "This will overwrite your screener parameters setting!\nDo you want to continue?",
                                   QMessageBox::Yes, QMessageBox::No);

    if(ret == QMessageBox::Yes)
    {
        emit loadOnlineParameters();
    }
}

void SettingsForm::on_pbShowParameters_clicked()
{
    ScreenerForm *dlg = new ScreenerForm(setting.screenerParams, this);
    connect(dlg, SIGNAL(setScreenerParams(QVector<sSCREENERPARAM>)), this, SLOT(getScreenerParamsSlot(QVector<sSCREENERPARAM>)));
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void SettingsForm::getScreenerParamsSlot(QVector<sSCREENERPARAM> params)
{
    setting.screenerParams = params;
    emit setScreenerParams(params);
}

void SettingsForm::updateScreenerParamsSlot(QVector<sSCREENERPARAM> params)
{
    setting.screenerParams = params;
}

void SettingsForm::on_pbDegiroPath_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CSV File"),
                                                    QCoreApplication::applicationDirPath(),
                                                    tr("CSV files (*.csv)"));

    ui->leDegiroCSV->setText(fileName);
    setting.degiroCSV = fileName;
}

void SettingsForm::on_pbDegiroLoadCSV_clicked()
{
    emit loadDegiroCSV();
}

void SettingsForm::on_pbTastyworksPath_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open CSV File"),
                                                    QCoreApplication::applicationDirPath(),
                                                    tr("CSV files (*.csv)"));

    ui->leTastyworksCSV->setText(fileName);
    setting.tastyworksCSV = fileName;
}

void SettingsForm::on_pbTastyworksLoadCSV_clicked()
{
    emit loadTastyworksCSV();
}


void SettingsForm::on_cbFilterON_clicked(bool checked)
{
    setting.filterON = checked;
}

void SettingsForm::on_cmCurrency_currentIndexChanged(int index)
{
    setting.currency = static_cast<eCURRENCY>(index);
    emit setSetting(setting);
    emit fillOverview();
}
