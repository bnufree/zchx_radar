#include "testmainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QTextCodec>

void logMessageOutputQt5(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QString log_file_name = "";
    static qint64 max = 10485760;//10 * 1024 * 1024;
    static QMutex mutex;
    static qint64 log_file_lenth = 0;
    mutex.lock();
    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("Debug:");
        break;
    case QtWarningMsg:
        text = QString("Warning:");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:");
        abort();
    default:
        break;
    }
    QString message = QString("[%1] %2 [%3] [%4] %5").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
            .arg(text).arg(context.file).arg(context.line).arg(msg);

    QDir dir(QCoreApplication::applicationDirPath() + QString("/log"));
    if(!dir.exists())
    {
        dir.mkpath(dir.path());
    }
    if(log_file_name.isEmpty() || log_file_lenth > max)
    {
        //重新启动的情况,将日志目录下的文件删除,保留最近的文件
        {
            QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
            int del_num = list.size() - 2;
            if(del_num > 0)
            {
                int i = 0;
                foreach(QFileInfo info, list)
                {
                    QFile::remove(info.absoluteFilePath());
                    i++;
                    if(i == del_num) break;
                }
            }
        }
        log_file_lenth = 0;
        log_file_name = QCoreApplication::applicationDirPath() + QString("/log/LOG_%1.txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    }
    QFile file(log_file_name);
    if(file.open(QIODevice::ReadWrite | QIODevice::Append))
    {
        QTextStream text_stream(&file);
        text_stream << message << endl;
        file.flush();
        file.close();
        log_file_lenth = file.size();
    }


//    QString fileMsg;
//    fileMsg.sprintf("%s-%s-%d", context.file, context.function, context.line);
//    QString total = QString("%1:%2  %3").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(fileMsg).arg(msg);
//    std::cout<<total.toStdString().data()<<endl;
    mutex.unlock();

}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    if(argc >= 1)
    {
        QString appName = QString::fromStdString(argv[0]);
        qDebug()<<"appName:"<<appName;
    }
#ifdef FILE_LOG
    qInstallMessageHandler(logMessageOutputQt5);
#endif
    TestMainWindow w;
    w.showMaximized();
    return a.exec();
}
