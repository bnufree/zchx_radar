#ifndef QCOMPORTSELECTEDCOMBOBOX_H
#define QCOMPORTSELECTEDCOMBOBOX_H

#include <QComboBox>

class QComPortSelectedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QComPortSelectedComboBox(QWidget *parent = 0);

signals:

public slots:
};

class QComBaudrateSelectedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QComBaudrateSelectedComboBox(QWidget *parent = 0);
};

class QComStopBitSelectedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QComStopBitSelectedComboBox(QWidget *parent = 0);
};

class QComDataBitSelectedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QComDataBitSelectedComboBox(QWidget *parent = 0);
};

class QComParityBitSelectedComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QComParityBitSelectedComboBox(QWidget *parent = 0);
};

#endif // QCOMPORTSELECTEDCOMBOBOX_H
