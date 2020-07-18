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

    status = new QLabel;
    ui->statusBar->addPermanentWidget(status);

    written = new QLabel;
    ui->statusBar->addPermanentWidget(written);

    initActionsConnections();

    connect(busStatusTimer, &QTimer::timeout, this, &CannabusControl::busStatus);
}

CannabusControl::~CannabusControl()
{
    delete connectDialog;
    delete ui;
}

void CannabusControl::initActionsConnections()
{
    ui->actionDisconnect->setEnabled(false);

    connect(ui->actionConnect, &QAction::triggered, [this]() {
        canDevice.release()->deleteLater();
        connectDialog->show();
    });
    connect(ui->actionDisconnect, &QAction::triggered,
            this, &CannabusControl::disconnectDevice);
    connect(ui->actionClearLog, &QAction::triggered, ui->receivedMassegesLogWindow, &QTextEdit::clear);
    connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);

    connect(connectDialog, &QDialog::accepted, this, &CannabusControl::connectDevice);
}

void CannabusControl::connectDevice()
{
    const ConnectDialog::Settings settings = connectDialog->settings();

    QString errorString;


}

void CannabusControl::disconnectDevice()
{

}

void CannabusControl::closeEvent(QCloseEvent *event)
{
    connectDialog->close();
    event->accept();
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

