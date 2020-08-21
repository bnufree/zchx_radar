#ifndef MAINPROCESS_H
#define MAINPROCESS_H

#include <QObject>

#define MainProc        MainProcess::instance()

class MainProcess : public QObject
{
    Q_OBJECT
private:
    explicit MainProcess(QObject *parent = 0);
public:
    static MainProcess* instance();
    void    start();
    bool    isStart() const {return mStartFlag;}

signals:

public slots:
private:
    static  MainProcess* m_pInstance;
    bool    mStartFlag;

};

#endif // MAINPROCESS_H
