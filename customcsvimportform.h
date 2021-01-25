#ifndef CUSTOMCSVIMPORTFORM_H
#define CUSTOMCSVIMPORTFORM_H

#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>

#include "global.h"

namespace Ui {
    class CustomCSVImportForm;
}

class CustomCSVImportForm : public QDialog
{
    Q_OBJECT

    enum eITEMTYPE
    {
        NO = 0,
        DATE,
        TIME,
        TICKER,
        ISIN,
        CURRENCY,
        VALUE,
        FEE,
        TYPE
    };

public:
    explicit CustomCSVImportForm(QWidget *parent = nullptr);
    ~CustomCSVImportForm();

private slots:
    void on_pbLoad_clicked();
    void on_table_cellDoubleClicked(int row, int column);

    void loadCSV();   
    void checkTableColumns();
    void on_cmDateType_currentIndexChanged(int index);

private:
    Ui::CustomCSVImportForm *ui;

    QFile *loadedFile;

    int setTableHeader(const QString &line, const char &delimeter);
    int setTableHeader(const int &columns);
    QStringList parseLine(QString line, char delimeter);

    QStringList itemTypes;
    QStringList dateTypes;
    QStringList typeTypes;

    QVector<QPair<eITEMTYPE, int>> selectedColumnType;
    int selectedDateType;
};

#endif // CUSTOMCSVIMPORTFORM_H
