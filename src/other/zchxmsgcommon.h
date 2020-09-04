#ifndef ZCHXMSGCOMMON_H
#define ZCHXMSGCOMMON_H

enum MsgCmd{
    Msg_Undefined = 0,
    Msg_Edit_FilterArea = 1,
    Msg_Delete_FilterArea,
    Msg_Heart,
};

#define     JSON_CMD            "cmd"
#define     JSON_CMD_STR        "cmd_str"
#define     JSON_VAL            "content"
#define     JSON_STATUS         "status"
#define     JSON_STATUS_STR     "status_str"
#define     JSON_OK             1
#define     JSON_ERROR          0


#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QPolygon>
#include <QpolygonF>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QVariant>

//添加数据结构
namespace zchxMsg{
struct Latlon{
    double lat;
    double lon;
    Latlon() {lat =0.0; lon = 0.0;}
    Latlon(const QJsonArray& array)
    {
        lat = array[1].toDouble();
        lon = array[0].toDouble();
    }
    QJsonValue toJson() const
    {
        QJsonArray array;
        array.append(lon);
        array.append(lat);
        return array;
    }
};

typedef         QList<Latlon>       Area;

struct filterArea{
    int         type;           //目标在区域内是过滤还是保留。0；过滤；1保留
    qint64         id;
    int         site;
    Area        area;
    qint64      time;
    QString     name;

    filterArea() {id = 0; type = 0; site = 0;}
    filterArea(const QJsonObject& obj)
    {
        id = obj.value("id").toVariant().toLongLong();
        type = obj.value("type").toInt();
        site = obj.value("site").toInt();
        QJsonArray array = obj.value("area").toArray();
        for(int i=0; i<array.size(); i++)
        {
            area.append(Latlon(array[i].toArray()));
        }
        time = obj.value("time").toVariant().toLongLong();
        name = obj.value("name").toString();
    }
    QJsonValue toJson() const
    {
        QJsonObject obj;
        obj.insert("id", id);
        obj.insert("type", type);
        obj.insert("site", site);
        QJsonArray array;
        for(int i=0; i<area.size(); i++)
        {
            array.append(area[i].toJson());
        }
        obj.insert("area", array);
        obj.insert("time", time);
        obj.insert("name", name);
        return obj;
    }
};
}

#endif // ZCHXMSGCOMMON_H
