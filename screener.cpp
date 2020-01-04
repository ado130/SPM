#include "screener.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>

Screener::Screener(QObject *parent) : QObject(parent)
{
    loadAllScreenerData();
}

void Screener::saveAllScreenerData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + SCREENERALLDATA);

    if (qFile.open(QIODevice::WriteOnly))
    {
        QDataStream out(&qFile);
        out << allScreenerData;
        qFile.close();
    }
}

void Screener::loadAllScreenerData()
{
    QFile qFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + SCREENERALLDATA);

    if(qFile.exists())
    {
        if (qFile.open(QIODevice::ReadOnly))
        {
            QDataStream in(&qFile);
            in >> allScreenerData;
            qFile.close();
        }
    }
}

QVector<sSCREENER> Screener::getAllScreenerData() const
{
    return allScreenerData;
}

void Screener::setAllScreenerData(const QVector<sSCREENER> &value)
{
    allScreenerData = value;
    saveAllScreenerData();
}

sTABLE Screener::finvizParse(QString data)
{
    sTABLE table;
    sTICKERINFO info;

    int startB = data.indexOf("<body");
    int endB = data.indexOf("</body");
    QString body = data.mid(startB, endB-startB);


    int startT = body.indexOf("<table", 0);

    int startNextTable = body.indexOf("<table", startT+1);

    int start1TR = body.indexOf("<tr", startNextTable);
    int start2TR = body.indexOf("<tr", start1TR+1);
    int start3TR = body.indexOf("<tr", start2TR+1);
    int start4TR = body.indexOf("<tr", start3TR+1);
    int start5TR = body.indexOf("<tr", start4TR+1);
    int start6TR = body.indexOf("<tr", start5TR+1);
    int start7TR = body.indexOf("<tr", start6TR+1);
    int start8TR = body.indexOf("<tr", start7TR+1);
    int start9TR = body.indexOf("<tr", start8TR+1);
    int start10TR = body.indexOf("<tr", start9TR+1);
    int start11TR = body.indexOf("<tr", start10TR+1);
    int start12TR = body.indexOf("<tr", start11TR+1);
    int start13TR = body.indexOf("<tr", start12TR+1);
    int start14TR = body.indexOf("<tr", start13TR+1);
    int start15TR = body.indexOf("<tr", start14TR+1);
    int start16TR = body.indexOf("<tr", start15TR+1);
    int start17TR = body.indexOf("<tr", start16TR+1);
    int start18TR = body.indexOf("<tr", start17TR+1);
    int start19TR = body.indexOf("<tr", start18TR+1);

    int st = body.indexOf("<b>", start18TR);
    st = body.indexOf(">", st);
    int en = body.indexOf("</b>", st);
    QString tmp = body.mid( st+1, en-st-1);
    info.stockName = tmp.replace("&","&amp;");

    int start20TR = body.indexOf("<tr", start19TR+1);

    st = body.indexOf("<a href", start19TR);
    st = body.indexOf(">", st);
    en = body.indexOf("</a>", st);
    tmp = body.mid( st+1, en-st-1);
    info.sector = tmp;

    st = body.indexOf("<a href", en);
    st = body.indexOf(">", st);
    en = body.indexOf("</a>", st);
    tmp = body.mid( st+1, en-st-1);
    info.industry = tmp;

    st = body.indexOf("<a href", en);
    st = body.indexOf(">", st);
    en = body.indexOf("</a>", st);
    tmp = body.mid( st+1, en-st-1);
    info.country = tmp;

    table.info = info;

    int start21TR = body.indexOf("<tr", start20TR+1);

    int startInnerTable = body.indexOf("<table", start20TR);
    int endInnderTable = body.indexOf("</table>", start21TR);

    QStringList TRvalues = body.mid(startInnerTable, endInnderTable-startInnerTable).split("<tr");

    for(const QString &TR : TRvalues)
    {
        if(!TR.startsWith(" class")) continue;

        QStringList TDvalues = TR.split("<td");
        TDvalues.removeFirst();

        for(int a = 0; a<TDvalues.count(); a+=2)
        {
            if(!TDvalues.at(a).startsWith(" width")) continue;

            QString name = TDvalues.at(a);
            QString value = TDvalues.at(a+1);

            int startName = name.indexOf("delay", 0);
            startName = name.indexOf(">", startName);
            int endName = name.indexOf("</td>");

            name = name.mid(startName+1, endName-(startName+1));

            if(value.contains("<span"))
            {
                int startValue = value.indexOf("<span", 0);
                startValue = value.indexOf(">", startValue);
                int endValue = value.indexOf("</span>", startValue);

                value = value.mid(startValue+1, endValue-(startValue+1));
            }
            else if(value.contains("<small>"))
            {
                int startValue = value.indexOf("<small>", 0);
                int endValue = value.indexOf("</small>", startValue);

                value = value.mid(startValue+7, endValue-(startValue+7));
            }
            else if(value.contains("<b>"))
            {
                int startValue = value.indexOf("<b>", 0);
                int endValue = value.indexOf("</b>", startValue);

                value = value.mid(startValue+3, endValue-(startValue+3));
            }

            table.row.insert(name, value);
        }
    }

    return table;

}

sTABLE Screener::yahooParse(QString data)
{
    sTABLE table;

    int startB = data.indexOf("<body");
    int endB = data.indexOf("</body");
    QString body = data.mid(startB, endB-startB);

    int lastEndT = body.lastIndexOf("</table");

    int startT = 0, endT = 0;

    do
    {
        startT = body.indexOf("<table", startT+1);

        int startTB = body.indexOf("<tbody", startT);
        startTB = body.indexOf(">", startTB);

        int endTB = body.indexOf("</tbody>", startTB);

        QStringList TRvalues = body.mid(startTB, endTB-startTB).split("<tr");

        sTABLE table;
        for(const QString &TR : TRvalues)
        {
            if(!TR.startsWith(" class")) continue;

            QString name;
            QString value;

            int TD = TR.indexOf("<td");
            TD = TR.indexOf(">", TD);
            TD = TR.indexOf("<span", TD);
            TD = TR.indexOf(">", TD);
            name = TR.mid(TD+1, TR.indexOf("</span>", TD)-TD-1);

            TD = TR.indexOf("<td", TD);
            TD = TR.indexOf(">", TD);
            value = TR.mid(TD+1, TR.indexOf("</td>", TD)-TD-1);

            table.row.insert(name, value);
        }

        endT = body.indexOf("</table>", endT+1);

    }while(endT != lastEndT);

    return table;
}

QDataStream &operator<<(QDataStream &out, const sSCREENER &param)
{
    out << param.screenerData;
    out << param.screenerName;

    return out;
}

QDataStream &operator>>(QDataStream &in, sSCREENER &param)
{
    in >> param.screenerData;
    in >> param.screenerName;

    return in;
}
