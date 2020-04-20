#include "zchxmainwindow.h"
#include <QApplication>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMessageBox>
#include <QDebug>
#include <QTextCodec>
#include "dbgcrash.h"
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"


int main(int argc, char *argv[])
{
    //SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
    QApplication a(argc, argv);
    QSharedMemory mem("SCCMMS_RadarDataCollectServer");
//    if (!mem.create(1))
//    {
//        QMessageBox msgBox(QMessageBox::Warning,QString("提示"),QString("\n提示，您已开启一个服务器程序，该程序不能同时开启多个。\n"));
//        msgBox.addButton(QString("确定"), QMessageBox::AcceptRole);
//        msgBox.exec();
//        a.exit(1);
//        return 0;
//    }
    zchxMainWindow *w = new zchxMainWindow;
    w->show();
    return a.exec();
}
