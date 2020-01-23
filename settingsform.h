#ifndef SETTINGSFORM_H
#define SETTINGSFORM_H

#include <QDialog>

#include "global.h"
#include "screenerform.h"

namespace Ui {
class SettingsForm;
}

class SettingsForm : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsForm(sSETTINGS set, QWidget *parent = nullptr);
    ~SettingsForm();



public slots:
    void getScreenerParamsSlot(QVector<sSCREENERPARAM> params);
    void updateScreenerParamsSlot(QVector<sSCREENERPARAM> params);

private slots:
    void on_buttonBox_accepted();

    void on_pbDegiroPath_clicked();
    void on_pbDegiroLoadCSV_clicked();

    void on_pbLoadParameters_clicked();
    void on_pbShowParameters_clicked();
    void on_cbFilterON_clicked(bool checked);

    void on_cmCurrency_currentIndexChanged(int index);

    void on_pbTastyworksPath_clicked();

    void on_pbTastyworksLoadCSV_clicked();



    void on_cbSoldPositions_clicked(bool checked);

signals:
    void setSetting(sSETTINGS);
    void setScreenerParams(QVector<sSCREENERPARAM> params);
    void loadOnlineParameters();

    void loadDegiroCSV();
    void loadTastyworksCSV();

    void fillOverview();

private:
    Ui::SettingsForm *ui;

    sSETTINGS setting;
};

#endif // SETTINGSFORM_H
