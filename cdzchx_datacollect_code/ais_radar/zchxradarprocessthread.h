#ifndef ZCHXRADARPROCESSTHREAD_H
#define ZCHXRADARPROCESSTHREAD_H

#include <QObject>
#include <QThread>
class ZCHXRadarProcessThread : public QObject
{
    Q_OBJECT
public:
    explicit ZCHXRadarProcessThread(QObject *parent = 0);

signals:

public slots:
private:
    QThread m_workThread;
};

#endif // ZCHXRADARPROCESSTHREAD_H
