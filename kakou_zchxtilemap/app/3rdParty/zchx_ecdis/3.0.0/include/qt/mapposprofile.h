#ifndef MAPPOSPROFILE_H
#define MAPPOSPROFILE_H

#include <QObject>
#include <QMap>
#include "zchxutils.hpp"

namespace qt {
class MapPosProfile : public QObject
{
    Q_OBJECT
public:
    struct MapBounds{
        double     min_x;
        double     min_y;
        double     max_x;
        double     max_y;
    };
    explicit MapPosProfile(QString fileName, QObject *parent = 0);
    QString getMapPos(double lon, double lat);

signals:

public slots:

private:
    bool loadConfig(QString fileName);

    QMap<QString, MapBounds> m_configMap;
};
}

#endif // MAPPOSPROFILE_H
