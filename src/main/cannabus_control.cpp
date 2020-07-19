#include "cannabus_control.h"
#include "ui_cannabus_control.h"
#include "connect_dialog.h"
#include "bitrate_box.h"
#include "../cannabus_library/cannabus_common.h"

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
    ui->statusBar->addWidget(written);

    QStringList logWindowHeader = {"No.", "Time", "F-Code", "Slave Address", "Message Type", "DLC", "Data", "Info"};

    ui->receivedMessagesLogWindow->setColumnCount(logWindowHeader.count());
    ui->receivedMessagesLogWindow->setHorizontalHeaderLabels(logWindowHeader);

    ui->receivedMessagesLogWindow->resizeColumnsToContents();
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("No."), 60);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("Time"), 90);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("F-Code"), 120);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("Slave Address"), 120);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("Message Type"), 120);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("DLC"), 60);
    ui->receivedMessagesLogWindow->setColumnWidth(logWindowHeader.indexOf("Data"), 120);

    ui->receivedMessagesLogWindow->horizontalHeader()->setStretchLastSection(true);

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
    connect(ui->actionClearLog, &QAction::triggered,
            ui->receivedMessagesLogWindow, &QTableWidget::clearContents);
    connect(ui->actionQuit, &QAction::triggered,
            this, &QWidget::close);

    connect(connectDialog, &QDialog::accepted,
            this, &CannabusControl::connectDevice);
}

void CannabusControl::processError(QCanBusDevice::CanBusError error) const
{
    switch(error)
    {
        case QCanBusDevice::ReadError:
        case QCanBusDevice::WriteError:
        case QCanBusDevice::ConnectionError:
        case QCanBusDevice::ConfigurationError:
        case QCanBusDevice::UnknownError:
        {
            status->setText(canDevice->errorString());
            break;
        }
        default:
        {
            break;
        }
    }
}

void CannabusControl::connectDevice()
{
    const ConnectDialog::Settings settings = connectDialog->settings();

    QString errorString;
    canDevice.reset(QCanBus::instance()->createDevice(
                        settings.pluginName,
                        settings.deviceInterfaceName,
                        &errorString));

    if (canDevice == nullptr)
    {
        status->setText(tr("Error creating device '%1': '%2'")
                        .arg(settings.pluginName)
                        .arg(errorString));
        return;
    }

    numberFramesReceived = 0;

    connect(canDevice.get(), &QCanBusDevice::errorOccurred,
            this, &CannabusControl::processError);
    connect(canDevice.get(), &QCanBusDevice::framesReceived,
            this, &CannabusControl::processFramesReceived);

    if (settings.useCustomConfigurationEnabled != false)
    {
        for (const ConnectDialog::ConfigurationItem &item : qAsConst(settings.configurations))
        {
            canDevice->setConfigurationParameter(item.first, item.second);
        }
    }

    if (canDevice->connectDevice() == false)
    {
        status->setText(tr("Connection error: '%1'").arg(canDevice->errorString()));

        canDevice.reset();
    }
    else
    {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(false);

        const QVariant bitRate = canDevice->configurationParameter(QCanBusDevice::BitRateKey);
        if (bitRate.isValid() != false)
        {
            const bool useCanFd = canDevice->configurationParameter(QCanBusDevice::CanFdKey).toBool();
            const QVariant dataBitRate = canDevice->configurationParameter(QCanBusDevice::DataBitRateKey);

            if (useCanFd != false && dataBitRate.isValid() != false)
            {
                status->setText(tr("Plugin '%1': connected to %2 at %3 %4 / %5 %6 with CAN FD")
                                .arg(settings.pluginName)
                                .arg(settings.deviceInterfaceName)
                                .arg(bitRate.toUInt() / (bitRate.toUInt() < BitRate_1000000_bps ? 1000 : 1000000))
                                .arg(bitRate.toUInt() < BitRate_1000000_bps ? tr("kbit/s") : tr("Mbit/s"))
                                .arg(dataBitRate.toUInt() / (dataBitRate.toUInt() < BitRate_1000000_bps ? 1000 : 1000000))
                                .arg(dataBitRate.toUInt() < BitRate_1000000_bps ? tr("kbit/s") : tr("Mbit/s")));
            }
            else
            {
                status->setText(tr("Plugin '%1': connected to %2 at %3 %4")
                                .arg(settings.pluginName)
                                .arg(settings.deviceInterfaceName)
                                .arg(bitRate.toUInt() / (bitRate.toUInt() < BitRate_1000000_bps ? 1000 : 1000000))
                                .arg(bitRate.toUInt() < BitRate_1000000_bps ? tr("kbit/s") : tr("Mbit/s")));
            }
        }
        else
        {
            status->setText(tr("Plugin '%1': connected to %2")
                            .arg(settings.pluginName)
                            .arg(settings.deviceInterfaceName));
        }

        if (canDevice->hasBusStatus() != false)
        {
            busStatusTimer->start(1000);
        }
        else
        {
            ui->busStatus->setText(tr("No CAN bus status available"));
        }
    }
}

void CannabusControl::disconnectDevice()
{
    if (canDevice == nullptr)
    {
        return;
    }

    busStatusTimer->stop();

    canDevice->disconnectDevice();

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);

    status->setText("Disconnected");
}

void CannabusControl::closeEvent(QCloseEvent *event)
{
    connectDialog->close();
    event->accept();
}

void CannabusControl::processFramesReceived()
{
    if (canDevice == nullptr)
    {
        return;
    }

    while (canDevice->framesAvailable() != false)
    {
        const QCanBusFrame frame = canDevice->readFrame();

        numberFramesReceived++;

        if (frame.frameType() == QCanBusFrame::ErrorFrame)
        {
            return;
        }

        const uint32_t frameId = frame.frameId();
        const uint32_t fCode = (uint32_t)cannabus::getFCodeFromId(frameId);
        const uint32_t slaveAddress = cannabus::getAddressFromId(frameId);
        const uint32_t messageType = (uint32_t)cannabus::getMsgTypeFromId(frameId);

        const QString time = tr("%1.%2")
                .arg(frame.timeStamp().seconds())
                .arg(frame.timeStamp().microSeconds());
        const QString data = frame.toString();

        QStringList frameInfo = {
            tr("%1").arg(numberFramesReceived),
            tr("%1").arg(fCode),
            tr("%1").arg(slaveAddress),
            tr("%1").arg(messageType),
            "",
            data,
            ""
        };

        uint32_t row = ui->receivedMessagesLogWindow->rowCount();
        ui->receivedMessagesLogWindow->insertRow(row);

        for (uint8_t column = 0; column < ui->receivedMessagesLogWindow->columnCount(); column++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(frameInfo.takeFirst());
            ui->receivedMessagesLogWindow->setItem(row, column, item);
        }
    }
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

