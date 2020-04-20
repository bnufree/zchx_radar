#ifndef TESTMAPWATCHDOGTHREAD_H
#define TESTMAPWATCHDOGTHREAD_H

#include <QThread>

class testMapWatchDogThread : public QThread
{
    Q_OBJECT
public:
    explicit testMapWatchDogThread(QObject *parent = 0);
    void run();

signals:

public slots:
};

#endif // TESTMAPWATCHDOGTHREAD_H
