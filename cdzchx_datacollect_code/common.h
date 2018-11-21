#ifndef COMMON_H
#define COMMON_H

#include <QDebug>
#include <QDateTime>

#define     LOGDEBUG        if(1) std::cout
#define     LOG_FUNC_DBG        if(1) qDebug()<<__FUNCTION__<<__LINE__
#define     LOG_FUNC_DBG_START   if(1)      qDebug()<<__FUNCTION__<<"Start"
#define     LOG_FUNC_DBG_END     if(1)    qDebug()<<__FUNCTION__<<"End"

class TimeStamp {
public:
    TimeStamp() {mDateTime.setMSecsSinceEpoch(0);}
    TimeStamp(const QDateTime& dt) {mDateTime = dt;}
    TimeStamp(const TimeStamp& other) {mDateTime = other.mDateTime;}
    TimeStamp& operator =(const TimeStamp& other)
    {
        if(this != &other)
        {
            this->mDateTime = other.mDateTime;
        }
        return *this;
    }
    static TimeStamp timeStampFromMsecs(qint64 msec)
    {
        return TimeStamp(QDateTime::fromMSecsSinceEpoch(msec));
    }

    long secs() const {return mDateTime.toTime_t();}
    qint64 msec() const {return mDateTime.toMSecsSinceEpoch();}
    static int  timeOfDay()
    {
        QDateTime cur = QDateTime::currentDateTime();
        QDateTime day = QDateTime(cur.date());
        return day.secsTo(cur);
    }

private:
    QDateTime       mDateTime;
};

#endif // COMMON_H
