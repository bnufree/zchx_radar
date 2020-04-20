#include "profiles.h"
#include <QDebug>
#define cout qDebug()<< "在文件"<<__FILE__ << "第"<< __LINE__<< "行"

namespace Utils {

Profiles* Profiles::minstance = 0;

Profiles::Profiles(QObject *parent) :
    QObject(parent)
{
    configSettings = new QSettings("etc/profiles.ini", QSettings::IniFormat);
    configSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
    iniSettings = new QSettings(QSettings::IniFormat, QSettings::UserScope, QLatin1String("Frameworks"), QLatin1String("profiles"));
    iniSettings->setIniCodec(QTextCodec::codecForName("UTF-8"));
}



Profiles::~Profiles()
{
    if ( minstance )
    {
        delete minstance;
        minstance = 0;
    }
}
/*-------------------------------------------
 *
 * 实例化
 *
---------------------------------------------*/
Profiles *Profiles::instance()
{
    if ( minstance == 0)
    {
        minstance = new Profiles();
    }
    return minstance;
}


/*-------------------------------------------
 *
 * 设置默认值
 *
---------------------------------------------*/
void Profiles::setDefault(const QString & prefix,const QString &key, const QVariant &value)
{
    configSettings->beginGroup(prefix);
    QVariant oldValue = configSettings->value(key);
    if(oldValue.toString().isEmpty() && oldValue.toStringList().size() == 0)
    {
        cout<<"设置默认值,配置文件为空"<<key;
        configSettings->setValue(key, value);
    }
    configSettings->endGroup();
}
/*-------------------------------------------
 *
 * 设置配置文件值
 *
---------------------------------------------*/
void Profiles::setValue(const QString & prefix,const QString & key, const QVariant & value)
{
    configSettings->beginGroup(prefix);
    {
        //cout<<"设置配置文件值";
        configSettings->setValue(key, value);
    }
    configSettings->endGroup();
}
/*-------------------------------------------
 *
 * 返回值
 *
---------------------------------------------*/
QVariant Profiles::value(const QString & prefix,const QString &keys, const QVariant &defaultValue)
{
     /*QVariant values;
    configSettings->beginGroup(prefix);
    {
        values =  configSettings->value( keys,defaultValue);
    }
    configSettings->endGroup();*/

    QVariant values;
    values =  configSettings->value( prefix+"/"+keys,defaultValue);
    return values;
}
/*-------------------------------------------
 *
 * 设置Ini默认值
 *
---------------------------------------------*/
void Profiles::setUserDefault(const QString & prefix,const QString &key, const QVariant &value)
{
    iniSettings->beginGroup(prefix);
    if(iniSettings->value(key).toString().isEmpty())
    {
        iniSettings->setValue(key, value);
    }
    iniSettings->endGroup();
}
/*-------------------------------------------
 *
 * 设置Ini配置文件值
 *
---------------------------------------------*/
void Profiles::setUserValue(const QString & prefix,const QString & key, const QVariant & value)
{
    iniSettings->beginGroup(prefix);
    {
        iniSettings->setValue(key, value);
    }
    iniSettings->endGroup();
}
/*-------------------------------------------
 *
 * 返回值Ini
 *
---------------------------------------------*/
QVariant Profiles::userValue(const QString & prefix,const QString &keys, const QVariant &defaultValue)
{
    QVariant values;
    iniSettings->beginGroup(prefix);
    {
        values =  iniSettings->value( keys,defaultValue);
    }
    iniSettings->endGroup();
    return values;
}

QStringList Profiles::subkeys(const QString &prefix)
{
    QStringList keys;
    configSettings->beginGroup(prefix);
    {
        keys =  configSettings->childKeys();
    }
    configSettings->endGroup();
    return keys;
}

void Profiles::removeKeys(const QString &prefix, const QStringList &keys)
{
    configSettings->beginGroup(prefix);
    {
        foreach (QString key, keys) {
            configSettings->remove(key);
        }

    }
    configSettings->endGroup();
}

void Profiles::removeGroup(const QString &prefix)
{
    removeKeys(prefix, subkeys(prefix));
}



}
