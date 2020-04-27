#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>
#include <QSettings>
//#include <QDebug>
#include <QTextCodec>

#define     PROFILES_INSTANCE       Utils::Profiles::instance()

namespace Utils {

class Profiles : public QObject
{
    Q_OBJECT
public:
    ~Profiles();
    static Profiles *instance();

public slots:
    void setValue( const QString & prefix, const QString & key, const QVariant & value );
    void setDefault( const QString & prefix, const QString & key, const QVariant & value );
    QVariant value( const QString & prefix,const QString &keys,const QVariant & defaultValue = QVariant() );
    QStringList subkeys(const QString& prefix);
    void        removeKeys(const QString& prefix, const QStringList& keys);
    void        removeGroup(const QString& prefix);
public slots:
    void setUserValue( const QString & prefix, const QString & key, const QVariant & value );
    void setUserDefault( const QString & prefix, const QString & key, const QVariant & value );
    QVariant userValue( const QString & prefix,const QString &keys,const QVariant & defaultValue = QVariant() );
private:
    explicit Profiles(QObject *parent = 0);
private:
    static Profiles     *minstance;
    QSettings           *configSettings;
    QSettings           *iniSettings;
    QHash<QString,double>  m_appropriateAllowance4Downgrade;
    QHash<QString, double> m_appropriateAllowance4Upgrade;
};

}
#endif // PROFILE_H
