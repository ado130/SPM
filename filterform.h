#ifndef FILTERFORM_H
#define FILTERFORM_H

#include <QDialog>
#include <QGridLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QCloseEvent>

#include "global.h"

namespace Ui {
class FilterForm;
}

class FilterForm : public QDialog
{
    Q_OBJECT

public:
    explicit FilterForm(QStringList params, QVector<sFILTER> list, QWidget *parent = nullptr);
    ~FilterForm();

signals:
    void setFilter(QVector<sFILTER> list);

private slots:
    void on_pbAddRow_clicked();

    void deleteLine();
    void colorChanged(const QString &text);
    void filterChanged(const QString &text);

    void on_pbSave_clicked();

private:
    Ui::FilterForm *ui;

    QGridLayout *gl;

    QVector<QComboBox*> paramList;
    QVector<QLineEdit*> expressionList;
    QVector<QLineEdit*> colorList;
    QVector<QPushButton*> delButtonList;

    QStringList enabledParams;
    QVector<sFILTER> filterList;

    QLayout *findParentLayout(QWidget *w, QLayout *topLevelLayout);
    QLayout *findParentLayout(QWidget *w);

    void clearLayout(QLayout *layout);

    void setFilterList();
};

#endif // FILTERFORM_H
