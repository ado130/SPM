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
    void on_pbDegiroPath_clicked();
    void on_buttonBox_accepted();
    void on_cmCSV_currentIndexChanged(int index);


    void on_cbAutoLoad_clicked(bool checked);
    void on_pbLoadParameters_clicked();
    void on_pbShowParameters_clicked();
    void on_pbLoadDegiroCSV_clicked();
    void on_cbFilterON_clicked(bool checked);
    void on_cbStartReload_clicked(bool checked);

signals:
    void setSetting(sSETTINGS);
    void setScreenerParams(QVector<sSCREENERPARAM> params);
    void loadOnlineParameters();
    void loadDegiroCSV();

private:
    Ui::SettingsForm *ui;

    sSETTINGS setting;
};

#endif // SETTINGSFORM_H
