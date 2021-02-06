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

    enum eITEMTYPE { ITEMNO = 0, ITEMDATE, ITEMTIME, ITEMTICKER, ITEMISIN, ITEMCURRENCY, ITEMVALUE, ITEMFEE, ITEMTYPE };

    typedef struct sCOLUMNTYPE
    {
        int column;
        eITEMTYPE type;
    }sCOLUMNTYPE;

public:
    explicit CustomCSVImportForm(eCUSTOMCSVACTION action, QWidget *parent = nullptr, StockDataType *data = nullptr);
    ~CustomCSVImportForm();

private slots:
    void on_pbLoad_clicked();
    void on_table_cellDoubleClicked(int row, int column);

    void loadCSV();   
    void validateTableColumns();

private:
    Ui::CustomCSVImportForm *ui;

    eCUSTOMCSVACTION action;

    QFile *loadedFile;
    QVector<sSTOCKDATA> exportTableData;

    QStringList itemTypes;
    QStringList dateTypes;
    QStringList typeTypes;

    QVector<sCOLUMNTYPE> selectedColumnType;
    int selectedDateType;

    int setTableHeader(const QString &line, const char &delimeter);
    int setTableHeader(const int &columns);
    QStringList parseLine(QString line, char delimeter);
    void saveCSV(const QString &fileName);
    void fillTable(StockDataType *data);
};

#endif // CUSTOMCSVIMPORTFORM_H
