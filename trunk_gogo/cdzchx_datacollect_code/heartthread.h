#ifndef HEARTTHREAD_H
#define HEARTTHREAD_H

#include <QThread>

class HeartThread : public QThread
{
    Q_OBJECT
public:
    explicit HeartThread(QObject *parent = 0);
    void setRun(bool brun);
    bool getPublish() {return mIsPublish;}
    void setHeartMsg(const QByteArray& msg);
protected:
    void run();

signals:
    void stopPublish();
    void startPublish();
    void hearmsg(QString msg);
    void signalUpdateGui(const QByteArray& config);

public slots:
private:
    bool mIsPublish;
    bool mRunning;
    QList<QByteArray> mGuiConfig;
};

#endif // HEARTTHREAD_H
