#include "mapposprofile.h"
#include <QFile>

namespace qt {
MapPosProfile::MapPosProfile(QString fileName, QObject *parent)
    : QObject(parent)
{
    loadConfig(fileName);
}

QString MapPosProfile::getMapPos(double lon, double lat)
{
    QMap<QString, MapBounds>::iterator it = m_configMap.begin();
    for (; it != m_configMap.end(); ++it)
    {
        MapBounds bounds = it.value();
        if (lon >= bounds.min_x &&
            lon <= bounds.max_x &&
            lat >= bounds.min_y &&
            lat <= bounds.max_y)
        {
            return it.key();
        }
    }

    return "";
}

bool MapPosProfile::loadConfig(QString fileName)
{
    QFile file(QString("mapdata/") + fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QString key = "";
    double min_x = 0;
    double min_y = 0;
    double max_x = 0;
    double max_y = 0;

    while (!file.atEnd())
    {
        QByteArray line = file.readLine();
        QString strLine(line);
        strLine = strLine.trimmed();
        strLine = strLine.replace(QRegExp("[ ]+"), " ");
        if (strLine.isEmpty())
        {
            continue;
        }

        if (strLine.contains("[") && strLine.contains("]"))
        {
            key = strLine.replace("[", "");
            key = key.replace("]", "");
        }
        else if (strLine.contains(" "))
        {
            QStringList valueList = strLine.split(" ");
            if (valueList.size() == 2)
            {
                if (min_x < DOUBLE_EPS && min_y < DOUBLE_EPS)
                {
                    min_x = valueList.at(0).toDouble();
                    min_y = valueList.at(1).toDouble();
                }
                else
                {
                    max_x = valueList.at(0).toDouble();
                    max_y = valueList.at(1).toDouble();

                    MapBounds bounds{min_x, min_y, max_x, max_y};
                    m_configMap.insert(key, bounds);

                    key = "";
                    min_x = 0;
                    min_y = 0;
                    max_x = 0;
                    max_y = 0;
                }
            }
        }
    }

    return true;
}
}
