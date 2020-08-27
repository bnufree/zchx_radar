#ifndef RADARSERVERCONFIG_H
#define RADARSERVERCONFIG_H

#include <QObject>

class RadarServerConfig : public QObject
{
    Q_OBJECT
public:
    explicit RadarServerConfig(QObject *parent = 0);

signals:

public slots:
};

#endif // RADARSERVERCONFIG_H