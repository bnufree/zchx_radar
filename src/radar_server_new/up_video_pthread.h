#ifndef UP_VIDEO_PTHREAD_H
#define UP_VIDEO_PTHREAD_H

#include <QThread>

class up_video_pthread : public QThread
{
    Q_OBJECT
public:
     up_video_pthread(QString type,QString filename,QObject *parent = 0);
     ~up_video_pthread();
     void run();
     void readAllFile(QString);
     void setIsOver(bool sts) {isOver  = sts;}


signals:
     void send_video_signal(QByteArray,QString,int,int,int);//导入回波数据

public slots:

private:
     QString file_name;
     QString type;
     QByteArray mAisData;
     int mTod;
     bool    isOver;
};

#endif // UP_VIDEO_PTHREAD_H
