#include "statistcLineinfodialog.h"
#include "ui_statistcLineinfodialog.h"
#include <QColorDialog>

namespace qt {
StatistcLineInfoDialog::StatistcLineInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StatistcLineInfoDialog)
{
    ui->setupUi(this);
}

StatistcLineInfoDialog::~StatistcLineInfoDialog()
{
    delete ui;
}

QString StatistcLineInfoDialog::getName()
{
    return ui->nameLineEdit->text();
}

int StatistcLineInfoDialog::getEnterStatus()
{
    return ui->enterStatusComboBox->currentIndex() + 1;
}

QString StatistcLineInfoDialog::getColor()
{
    return m_color;
}

void StatistcLineInfoDialog::on_colorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(Qt::red, this, tr("Color"));
    if(!color.isValid())
    {
        return;
    }
    m_color = color.name();

    QPalette palette;
    palette.setColor(QPalette::Background, color);
    ui->colorLabel->setAutoFillBackground(true);  //一定要这句，否则不行
    ui->colorLabel->setPalette(palette);
}

void StatistcLineInfoDialog::on_comfirmButton_clicked()
{
    accept();
}

void StatistcLineInfoDialog::on_cancelButton_clicked()
{
    reject();
}
}
