#ifndef SCREENERFORM_H
#define SCREENERFORM_H

#include <QDialog>
#include <QListWidgetItem>

#include "global.h"

namespace Ui {
class ScreenerForm;
}

class ScreenerForm : public QDialog
{
    Q_OBJECT

public:
    explicit ScreenerForm(QVector<sSCREENERPARAM> params, QWidget *parent = nullptr);
    ~ScreenerForm();

signals:
    void setScreenerParams(QVector<sSCREENERPARAM>);

private slots:
    void on_buttonBox_accepted();

    void on_listWidget_itemChanged(QListWidgetItem *item);

private:
    Ui::ScreenerForm *ui;

    QVector<sSCREENERPARAM> screenerParams;
    void fillList();
};

#endif // SCREENERFORM_H
