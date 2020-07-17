#include "cannabus_control.h"
#include "ui_cannabus_control.h"
#include "connect_dialog.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>

CannabusControl::CannabusControl(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CannabusControl),
    busStatusTimer(new QTimer(this))
{
    ui->setupUi(this);

    connectDialog = new ConnectDialog;

    initActionsConnections();

    connect(busStatusTimer, &QTimer::timeout, this, &CannabusControl::busStatus);
}

CannabusControl::~CannabusControl()
{
    delete ui;
}

void CannabusControl::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, [this]() { connectDialog->show(); });
}

void CannabusControl::connectDevice()
{
    const ConnectDialog::Settings p = connectDialog->settings();

    QString errorString;


}

void CannabusControl::disconnectDevice()
{

}

void CannabusControl::busStatus()
{
    if (canDevice == nullptr || canDevice->hasBusStatus() == false)
    {
        ui->busStatus->setText(tr("No CAN bus status available."));
        busStatusTimer->stop();
        return;
    }

    switch(canDevice->busStatus())
    {
        case QCanBusDevice::CanBusStatus::Good:
        {
            ui->busStatus->setText(tr("CAN bus status: Good."));
            break;
        }
        case QCanBusDevice::CanBusStatus::Warning:
        {
            ui->busStatus->setText(tr("CAN bus status: Warning."));
            break;
        }
        case QCanBusDevice::CanBusStatus::Error:
        {
            ui->busStatus->setText(tr("CAN bus status: Error."));
            break;
        }
        case QCanBusDevice::CanBusStatus::BusOff:
        {
            ui->busStatus->setText(tr("CAN bus status: Bus Off."));
            break;
        }
        default:
        {
            ui->busStatus->setText(tr("CAN bus status: Unknown."));
            break;
        }
    }
}

