#ifndef SCREENERTAB_H
#define SCREENERTAB_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>

#include "global.h"

namespace Ui {
class ScreenerTab;
}

class ScreenerTab : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenerTab(QWidget *parent = nullptr);
    ~ScreenerTab();

    QLabel* getHiddenRows();
    QTableWidget* getScreenerTable();

    sSCREENER getScreenerData() const;
    void setScreenerData(const sSCREENER &value);

private:
    Ui::ScreenerTab *ui;

    sSCREENER screenerData;
};

#endif // SCREENERTAB_H
