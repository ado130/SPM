#include "filterform.h"
#include "ui_filterform.h"

#include <QDebug>
#include <QRegExp>
#include <QRegExpValidator>
#include <QMenu>
#include <QColorDialog>

FilterForm::FilterForm(QStringList params, QVector<sFILTER> list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilterForm)
{
    ui->setupUi(this);

    enabledParams = params;
    filterList = list;


    ui->scrollArea->setWidgetResizable(true);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    gl = new QGridLayout(ui->scrollArea);

    ui->scrollAreaWidgetContents->setLayout(gl);

    QHBoxLayout *hb = new QHBoxLayout();
    QLabel *lbParam = new QLabel("Parameter", this);
    lbParam->setAlignment(Qt::AlignCenter);
    QLabel *lbFilter = new QLabel("Filter", this);
    lbFilter->setAlignment(Qt::AlignCenter);
    QLabel *lbColor = new QLabel("Color", this);
    lbColor->setAlignment(Qt::AlignCenter);
    lbColor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(lbColor, &QWidget::customContextMenuRequested, this, &FilterForm::colorContextMenuSlot);
    QLabel *lbShow = new QLabel("Show", this);
    lbShow->setAlignment(Qt::AlignCenter);
    QLabel *lbDelete = new QLabel("Delete", this);
    lbDelete->setAlignment(Qt::AlignCenter);

    hb->addWidget(lbParam);
    hb->addWidget(lbFilter);
    hb->addWidget(lbColor);
    hb->addWidget(lbShow);
    hb->addWidget(lbDelete);

    gl->addLayout(hb, 0, 0, Qt::AlignTop);

    setFilterList();
}

FilterForm::~FilterForm()
{
    delete ui;
}

void FilterForm::setFilterList()
{
    for(const sFILTER &item : filterList)
    {
        QHBoxLayout *hb = new QHBoxLayout();

        QComboBox *cmParam = new QComboBox(this);
        cmParam->addItems(enabledParams);

        if(cmParam->findText(item.param) != -1)
        {
            cmParam->setCurrentText(item.param);
        }
        else
        {
            continue;
        }

        QLineEdit *leFilter = new QLineEdit(this);
        leFilter->setToolTip("f< or f> or <f;f>");
        connect(leFilter, SIGNAL(textChanged(const QString)), this, SLOT(filterChanged(const QString)));

        switch(item.filter)
        {
            case LOWER:
                leFilter->setText(QString::number(item.val1, 'f', 2) + "<");
                break;
            case HIGHER:
                leFilter->setText(QString::number(item.val1, 'f', 2) + "<");
                break;
            case BETWEEN:
                leFilter->setText("<" + QString::number(item.val1, 'f', 2) + ";" + QString::number(item.val2, 'f', 2) + ">");
                break;
        }


        QLineEdit *leColor = new QLineEdit(this);
        leColor->setMaxLength(6);
        leColor->setToolTip("HEX color");
        connect(leColor, SIGNAL(textChanged(const QString)), this, SLOT(colorChanged(const QString)));
        leColor->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(leColor, &QWidget::customContextMenuRequested, this, &FilterForm::colorContextMenuSlot);

        QLabel *lbColor = new QLabel(this);
        lbColor->setMinimumSize(20, 20);

        QPushButton *pbDel = new QPushButton("Delete", this);
        connect(pbDel, SIGNAL(clicked()), this, SLOT(deleteLine()));

        paramList.push_back(cmParam);
        expressionList.push_back(leFilter);
        colorList.push_back(leColor);
        delButtonList.push_back(pbDel);

        hb->addWidget(cmParam);
        hb->addWidget(leFilter);
        hb->addWidget(leColor);
        hb->addWidget(lbColor);
        hb->addWidget(pbDel);

        int cnt = gl->count();

        gl->addLayout(hb, cnt, 0, Qt::AlignTop);

        leColor->setText(item.color);
    }
}

void FilterForm::colorContextMenuSlot(const QPoint &pos)
{
    Q_UNUSED(pos)

    QObject *obj = sender();

    if(!obj) return;

    QLineEdit *le = qobject_cast<QLineEdit*>(obj);

    QMenu *menu = new QMenu(this);

    QAction *color = new QAction("HEX colors", this);
    connect(color,&QAction::triggered,
            this,[le](){
                       QString clr = QColorDialog::getColor().name(QColor::HexRgb).mid(1);      // skip #
                       le->setText(clr);
                     }
            );

    menu->addAction(color);
    menu->popup(QCursor::pos());
}

void FilterForm::on_pbAddRow_clicked()
{
    QHBoxLayout *hb = new QHBoxLayout();

    QComboBox *cmParam = new QComboBox(this);
    cmParam->addItems(enabledParams);

    QLineEdit *leFilter = new QLineEdit(this);
    leFilter->setToolTip("f< or f> or <f;f>");
    connect(leFilter, SIGNAL(textChanged(const QString)), this, SLOT(filterChanged(const QString)));

    QLineEdit *leColor = new QLineEdit(this);
    leColor->setMaxLength(6);
    leColor->setToolTip("HEX color");
    connect(leColor, SIGNAL(textChanged(const QString)), this, SLOT(colorChanged(const QString)));
    leColor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(leColor, &QWidget::customContextMenuRequested, this, &FilterForm::colorContextMenuSlot);

    QLabel *lbColor = new QLabel(this);
    lbColor->setMinimumSize(20, 20);

    QPushButton *pbDel = new QPushButton("Delete", this);
    connect(pbDel, SIGNAL(clicked()), this, SLOT(deleteLine()));

    paramList.push_back(cmParam);
    expressionList.push_back(leFilter);
    colorList.push_back(leColor);
    delButtonList.push_back(pbDel);

    hb->addWidget(cmParam);
    hb->addWidget(leFilter);
    hb->addWidget(leColor);
    hb->addWidget(lbColor);
    hb->addWidget(pbDel);

    int cnt = gl->count();

    gl->addLayout(hb, cnt, 0, Qt::AlignTop);
}

void FilterForm::filterChanged(const QString &text)
{
    QObject *obj = sender();

    QLineEdit *le = qobject_cast<QLineEdit *>(obj);

    if(le)
    {
        int pos;
        QString valid = text;
        bool isValid = false;

        // f <
        QRegExp rx1("[-]?(([0]{1}[.]{1}[0-9]+)|([1-9]+[0-9]*[.]{1}[0-9]+)|([0-9]+))[<]{1}");
        QRegExpValidator v1(rx1, nullptr);

        // f >
        QRegExp rx2("[-]?(([0]{1}[.]{1}[0-9]+)|([1-9]+[0-9]*[.]{1}[0-9]+)|([0-9]+))[>]{1}");
        QRegExpValidator v2(rx2, nullptr);

        // <f;f>
        QRegExp rx3("([<]{1})(([0-9]{1}[.]{1}[0-9]+)|([1-9]+[.]{1}[0-9]+)|([1-9]+))([;]{1})(([0-9]{1}[.]{1}[0-9]+)|([1-9]+[.]{1}[0-9]+)|([1-9]+))([>]{1})");
        QRegExpValidator v3(rx3, nullptr);

        if(v1.validate(valid, pos) == QValidator::Acceptable)
        {
            isValid = true;
        }
        else if(v2.validate(valid, pos) == QValidator::Acceptable)
        {
            isValid = true;
        }
        else if(v3.validate(valid, pos) == QValidator::Acceptable)
        {
            isValid = true;
        }

        if(isValid)
        {
            le->setStyleSheet("QLineEdit { background: green }");
        }
        else
        {
            le->setStyleSheet("QLineEdit { background: red }");
        }
    }
}

void FilterForm::colorChanged(const QString &text)
{
    QObject *obj = sender();

    QLineEdit *le = qobject_cast<QLineEdit *>(obj);

    QLabel *lb = nullptr;

    if(le)
    {
        QString valid = text;

        if(valid == "HIDE")
        {
            le->setStyleSheet("QLineEdit { background: white }");
        }
        else
        {
            int pos;
            bool isHEX = false;
            QRegExp rx("[0-9a-fA-F]{6}");
            QRegExpValidator v(rx, nullptr);

            if(v.validate(valid, pos) == QValidator::Acceptable)
            {
                isHEX = true;
                le->setStyleSheet("QLineEdit { background: white }");
            }
            else
            {
                le->setStyleSheet("QLineEdit { background: red }");
            }

            if(isHEX)
            {
                QLayout *layout = findParentLayout(le);

                for (int i = 0; i < layout->count(); ++i)
                {
                    QWidget *widget = layout->itemAt(i)->widget();

                    if (widget != nullptr)
                    {
                        if(QLabel *l = qobject_cast<QLabel*>(widget))
                        {
                            lb = l;
                        }
                    }
                }
            }
            else
            {
                return;
            }
        }
    }

    if(lb != nullptr && text.size() == 6)
    {
        QImage image(20, 20, QImage::Format_RGBA8888);

        bool ok;

        int R = text.mid(0, 2).toInt(&ok, 16);
        int G = text.mid(2, 2).toInt(&ok, 16);
        int B = text.mid(4, 2).toInt(&ok, 16);

        for (int x = 0; x < 20; x++)
        {
            for (int y = 0; y < 20; y++)
            {
                QRgb value = qRgb(R, G, B);
                image.setPixel(x, y, value);
            }
        }

        lb->setPixmap(QPixmap::fromImage(image));
        lb->adjustSize();
    }
}

void FilterForm::deleteLine()
{
    QObject *obj = sender();

    QPushButton *pb = qobject_cast<QPushButton *>(obj);

    if(pb)
    {
        qDebug() << pb->layout();
        QLayout *layout = findParentLayout(pb);

        if(layout)
        {
            int pos = -1;

            for(int a = 0; a<delButtonList.count(); ++a)
            {
                if(delButtonList.at(a) == pb)
                {
                    pos = a;
                    break;
                }
            }

            if(pos != -1)
            {
                paramList.removeAt(pos);
                expressionList.removeAt(pos);
                colorList.removeAt(pos);
                delButtonList.removeAt(pos);
            }
        }

        clearLayout(layout);

    }
}

void FilterForm::clearLayout(QLayout *layout)
{
    if (layout)
    {
        QLayoutItem *item;

        //the key point here is that the layout items are stored inside the layout in a stack
        while((item = layout->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                layout->removeWidget(item->widget());
                delete item->widget();
            }

            delete item;
        }
    }
}

QLayout* FilterForm::findParentLayout(QWidget* w, QLayout* topLevelLayout)
{
  for (QObject* qo: topLevelLayout->children())
  {
     QLayout* layout = qobject_cast<QLayout*>(qo);
     if (layout != nullptr)
     {
        if (layout->indexOf(w) > -1)
          return layout;
        else if (!layout->children().isEmpty())
        {
          layout = findParentLayout(w, layout);
          if (layout != nullptr)
            return layout;
        }
     }
  }
  return nullptr;
};


QLayout* FilterForm::findParentLayout(QWidget* w)
{
    if (w->parentWidget() != nullptr)
        if (w->parentWidget()->layout() != nullptr)
            return findParentLayout(w, w->parentWidget()->layout());
    return nullptr;
}

void FilterForm::on_pbSave_clicked()
{
    int cnt = paramList.count();
    filterList.clear();

    bool isValid = true;

    for(int a = 0; a<cnt; ++a)
    {
        isValid = true;

        sFILTER filter;

        int pos;
        QString filterValid = expressionList.at(a)->text();

        // f <
        QRegExp rx1("[-]?(([0]{1}[.]{1}[0-9]+)|([1-9]+[0-9]*[.]{1}[0-9]+)|([0-9]+))[<]{1}");
        QRegExpValidator v1(rx1, nullptr);

        // f >
        QRegExp rx2("[-]?(([0]{1}[.]{1}[0-9]+)|([1-9]+[0-9]*[.]{1}[0-9]+)|([0-9]+))[>]{1}");
        QRegExpValidator v2(rx2, nullptr);

        // <f;f>
        QRegExp rx3("([<]{1})(([0-9]{1}[.]{1}[0-9]+)|([1-9]+[.]{1}[0-9]+)|([1-9]+))([;]{1})(([0-9]{1}[.]{1}[0-9]+)|([1-9]+[.]{1}[0-9]+)|([1-9]+))([>]{1})");
        QRegExpValidator v3(rx3, nullptr);

        QString colorValid = colorList.at(a)->text();

        if(colorValid == "HIDE")
        {
            // keep text, do nothing
        }
        else
        {
            int pos;
            QRegExp rx("[0-9a-fA-F]{6}");
            QRegExpValidator v(rx, nullptr);

            if(v.validate(colorValid, pos) != QValidator::Acceptable)
            {
                colorList.at(a)->setStyleSheet("QLineEdit { background: red }");
                isValid = false;
            }
        }

        if(v1.validate(filterValid, pos) == QValidator::Acceptable)
        {
            filter.filter = LOWER;
            filter.val1 = filterValid.mid(0, filterValid.indexOf("<")).toDouble();
            filter.val2 = 0.0;
        }
        else if(v2.validate(filterValid, pos) == QValidator::Acceptable)
        {
            filter.filter = HIGHER;
            filter.val1 = filterValid.mid(0, filterValid.indexOf(">")).toDouble();
            filter.val2 = 0.0;
        }
        else if(v3.validate(filterValid, pos) == QValidator::Acceptable)
        {
            filter.filter = BETWEEN;

            int semicolon = filterValid.indexOf(";");

            filter.val1 = filterValid.mid(1, semicolon-1).toDouble();
            filter.val2 = filterValid.mid(semicolon+1, filterValid.indexOf(">")-semicolon-1).toDouble();
        }
        else
        {
            isValid = false;
        }

        if(isValid)
        {
            filter.param = paramList.at(a)->currentText();
            filter.color = colorList.at(a)->text();

            filterList.push_back(filter);
        }
        else
        {
            expressionList.at(a)->setStyleSheet("QLineEdit { background: red }");
        }
    }


    if (isValid)
    {
        emit setFilter(filterList);
        this->accept();
    }
}
