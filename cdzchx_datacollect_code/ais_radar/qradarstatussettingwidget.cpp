#include "qradarstatussettingwidget.h"
#include "ui_qradarstatussettingwidget.h"
#include <QLabel>
#include <QSpinBox>

QRadarStatusSettingWidget::QRadarStatusSettingWidget(int radarID, QWidget *parent) :
    mRadarID(radarID),
    QWidget(parent),
    ui(new Ui::QRadarStatusSettingWidget)
{
    ui->setupUi(this);
    mValueSpinBoxMap.clear();
}

QRadarStatusSettingWidget::~QRadarStatusSettingWidget()
{
    delete ui;
}

void QRadarStatusSettingWidget::setRadarReportSeting(const QList<RadarStatus> &report)
{
    //清理所有的元素
    mValueSpinBoxMap.clear();
    QGridLayout *layout = qobject_cast<QGridLayout*>(this->layout());
    if(!layout)
    {
        layout = new QGridLayout(this);
        this->setLayout(layout);
    }
    int count = layout->count();
    while (count > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if(item )
        {
            QWidget *w = item->widget();
            if(w)
            {
                delete w;
                w = 0;
            }
        }
        count = layout->count();
    }

    int row = 0;
    int col = 0;
    int num = 0;
    foreach (RadarStatus element, report) {
        int elelmentID = element.id;
        int min = element.min;
        int max = element.max;
        int value = element.value;
        int unit = element.unit;

        QLabel *label = new QLabel(RadarStatus::getTypeString((INFOTYPE)elelmentID), this);
        QSpinBox *box = new QSpinBox(this);
        box->setMinimumHeight(35);
        box->setRange(min,max); // setRange before setValue
        box->setValue(value);
        box->setObjectName(QString("SpinBox_%1").arg(elelmentID));
        connect(box,SIGNAL(valueChanged(int)),this,SLOT(slotValueChanged(int)));

        layout->addWidget(label, row, col++);
        layout->addWidget(box, row, col++);

        num++;
        if(num % 2 == 0)
        {
            row++;
            col = 0;
        }

        mValueSpinBoxMap[elelmentID] = box;
    }
}

void QRadarStatusSettingWidget::slotValueChanged(int val)
{
    QSpinBox *box = qobject_cast<QSpinBox*> (sender());
    if(!box) return;
    QString name= box->objectName();
    QRegExp num("[0-9]{1,}");
    if(num.indexIn(name) >= 0)
    {
        int type = num.cap().toInt();
        emit signalRadarConfigChanged( type , val);
    }
}

void QRadarStatusSettingWidget::slotValueChangedFromServer(int type, int value)
{
    QSpinBox *box = mValueSpinBoxMap[type];
    if(!box) return;
    disconnect(box,SIGNAL(valueChanged(int)),this,SLOT(slotValueChanged(int)));
    box->setValue(value);
    connect(box,SIGNAL(valueChanged(int)),this,SLOT(slotValueChanged(int)));
}
