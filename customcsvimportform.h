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

public:
    explicit CustomCSVImportForm(QWidget *parent = nullptr);
    ~CustomCSVImportForm();

private slots:
    void on_pbLoad_clicked();

    void loadCSV();
    void on_table_cellDoubleClicked(int row, int column);

private:
    Ui::CustomCSVImportForm *ui;

    QFile *loadedFile;

    int setTableHeader(const QString &line, const char &delimeter);
    int setTableHeader(const int &columns);
};

#endif // CUSTOMCSVIMPORTFORM_H
