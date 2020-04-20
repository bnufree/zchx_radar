#ifndef PROTOBUFDATADISPLAYWIDGET_H
#define PROTOBUFDATADISPLAYWIDGET_H

#include <QWidget>
#include "TWQMSComData.pb.h"

using namespace com::zhichenhaixin::gps::proto;

namespace Ui {
class ProtobufDataDisplayWidget;
}

class ProtobufDataDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProtobufDataDisplayWidget(QWidget *parent = 0);
    ~ProtobufDataDisplayWidget();
    void display(const DevInfo& info);

private:
    Ui::ProtobufDataDisplayWidget *ui;
};

#endif // PROTOBUFDATADISPLAYWIDGET_H
