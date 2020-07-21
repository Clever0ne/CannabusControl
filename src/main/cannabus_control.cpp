#include "cannabus_control.h"
#include "ui_cannabus_control.h"
#include "connect_dialog.h"
#include "bitrate.h"
#include "../cannabus_library/cannabus_common.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>

CannabusControl::CannabusControl(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::CannabusControl),
    m_busStatusTimer(new QTimer(this))
{
    m_ui->setupUi(this);

    m_connectDialog = new ConnectDialog;

    m_status = new QLabel;
    m_ui->statusBar->addPermanentWidget(m_status);

    m_written = new QLabel;
    m_ui->statusBar->addWidget(m_written);

    initActionsConnections();

    connect(m_busStatusTimer, &QTimer::timeout, this, &CannabusControl::busStatus);
}

CannabusControl::~CannabusControl()
{
    delete m_connectDialog;
    delete m_ui;
}

void CannabusControl::initActionsConnections()
{
    m_ui->actionDisconnect->setEnabled(false);

    connect(m_ui->actionConnect, &QAction::triggered, [this]() {
        m_canDevice.release()->deleteLater();
        m_connectDialog->show();
    });
    connect(m_ui->actionDisconnect, &QAction::triggered,
            this, &CannabusControl::disconnectDevice);
    connect(m_ui->actionClearLog, &QAction::triggered, [this]() {
        m_ui->receivedMessagesLogWindow->clear();
        m_ui->receivedMessagesLogWindow->makeHeader();
    });
    connect(m_ui->actionQuit, &QAction::triggered,
            this, &QWidget::close);

    connect(m_connectDialog, &QDialog::accepted,
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
            m_status->setText(m_canDevice->errorString());
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
    const ConnectDialog::Settings settings = m_connectDialog->settings();

    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(
                        settings.pluginName,
                        settings.deviceInterfaceName,
                        &errorString));

    if (m_canDevice == nullptr)
    {
        m_status->setText(tr("Error creating device '%1': '%2'")
                        .arg(settings.pluginName)
                        .arg(errorString));
        return;
    }

    m_numberFramesReceived = 0;

    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,
            this, &CannabusControl::processError);
    connect(m_canDevice.get(), &QCanBusDevice::framesReceived,
            this, &CannabusControl::processFramesReceived);

    if (settings.isCustomConfigurationEnabled != false)
    {
        for (const ConnectDialog::ConfigurationItem &item : qAsConst(settings.configurations))
        {
            m_canDevice->setConfigurationParameter(item.first, item.second);
        }
    }

    if (m_canDevice->connectDevice() == false)
    {
        m_status->setText(tr("Connection error: '%1'").arg(m_canDevice->errorString()));

        m_canDevice.reset();
    }
    else
    {
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);

        const QVariant bitRate = m_canDevice->configurationParameter(QCanBusDevice::BitRateKey);
        if (bitRate.isValid() != false)
        {
            const bool isCanFdEnabled = m_canDevice->configurationParameter(QCanBusDevice::CanFdKey).toBool();
            const QVariant dataBitRate = m_canDevice->configurationParameter(QCanBusDevice::DataBitRateKey);

            if (isCanFdEnabled != false && dataBitRate.isValid() != false)
            {
                m_status->setText(tr("Plugin '%1': connected to %2 at %3/ %4 with CAN FD")
                                .arg(settings.pluginName)
                                .arg(settings.deviceInterfaceName)
                                .arg(bitRateToString(bitRate.toUInt()))
                                .arg(bitRateToString(dataBitRate.toUInt())));
            }
            else
            {
                m_status->setText(tr("Plugin '%1': connected to %2 at %3 %4")
                                .arg(settings.pluginName)
                                .arg(settings.deviceInterfaceName)
                                .arg(bitRateToString(bitRate.toUInt())));
            }
        }
        else
        {
            m_status->setText(tr("Plugin '%1': connected to %2")
                            .arg(settings.pluginName)
                            .arg(settings.deviceInterfaceName));
        }

        if (m_canDevice->hasBusStatus() != false)
        {
            m_busStatusTimer->start(1000);
        }
        else
        {
            m_ui->busStatus->setText(tr("No CAN bus status available"));
        }
    }
}

void CannabusControl::disconnectDevice()
{
    if (m_canDevice == nullptr)
    {
        return;
    }

    m_busStatusTimer->stop();

    m_canDevice->disconnectDevice();

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);

    m_status->setText("Disconnected");
}

void CannabusControl::closeEvent(QCloseEvent *event)
{
    m_connectDialog->close();
    event->accept();
}

void CannabusControl::processFramesReceived()
{
    if (m_canDevice == nullptr)
    {
        return;
    }

    while (m_canDevice->framesAvailable() != false)
    {
        const QCanBusFrame frame = m_canDevice->readFrame();

        m_numberFramesReceived++;

        if (frame.frameType() == QCanBusFrame::ErrorFrame)
        {
            return;
        }        

        const uint32_t frameId = frame.frameId();


        uint32_t msgType = (uint32_t)cannabus::getMsgTypeFromId(frameId);
        QString msgTypeInfo;

        uint32_t fCode = (uint32_t)cannabus::getFCodeFromId(frameId);
        QString fCodeInfo;

        switch (msgType)
        {
            case (uint32_t)cannabus::IdMsgTypes::HIGH_PRIO_MASTER:
            {
                msgTypeInfo = tr("Master's high-priority");
                break;
            }
            case (uint32_t)cannabus::IdMsgTypes::HIGH_PRIO_SLAVE:
            {
                msgTypeInfo = tr("Slave's high-priority");
                break;
            }
            case (uint32_t)cannabus::IdMsgTypes::MASTER:
            {
                msgTypeInfo = tr("Master's request");
                break;
            }
            case (uint32_t)cannabus::IdMsgTypes::SLAVE:
            {
                msgTypeInfo = tr("Slave's response");
                break;
            }
            default:
            {
                break;
            }
        }

        switch (msgType)
        {
            case (uint32_t)cannabus::IdFCode::WRITE_REGS_RANGE:
            {
                fCodeInfo = tr("Writing register's range");
                break;
            }
            case (uint32_t)cannabus::IdFCode::WRITE_REGS_SERIES:
            {
                fCodeInfo = tr("Writing register's series");
                break;
            }
            case (uint32_t)cannabus::IdFCode::READ_REGS_RANGE:
            {
                fCodeInfo = tr("Reading register's range");
                break;
            }
            case (uint32_t)cannabus::IdFCode::READ_REGS_SERIES:
            {
                fCodeInfo = tr("Reading register's series");
                break;
            }
            case (uint32_t)cannabus::IdFCode::DEVICE_SPECIFIC1:
            {
                fCodeInfo = tr("Device-specific function (1)");
                break;
            }
            case (uint32_t)cannabus::IdFCode::DEVICE_SPECIFIC2:
            {
                fCodeInfo = tr("Device-specific function (2)");
                break;
            }
            case (uint32_t)cannabus::IdFCode::DEVICE_SPECIFIC3:
            {
                fCodeInfo = tr("Device-specific function (3)");
                break;
            }
            case (uint32_t)cannabus::IdFCode::DEVICE_SPECIFIC4:
            {
                fCodeInfo = tr("Device-specific function (4)");
                break;
            }
            default:
            {
                break;
            }
        }

        QString count = QString::number(m_numberFramesReceived);
        QString time = tr("%1.%2")
                .arg(frame.timeStamp().seconds())
                .arg(frame.timeStamp().microSeconds());
        QString slaveAddress = QString::number(cannabus::getAddressFromId(frameId));
        QString dataSize = "[" + QString::number(frame.payload().size()) + "]";
        QString data(frame.payload().toHex(' '));
        QString info = tr("[%1] %2")
                .arg(fCodeInfo)
                .arg(msgTypeInfo);

        QStringList frameInfo = {
            count,
            time,
            QString::number(fCode, 2),
            slaveAddress,
            QString::number(msgType, 2),
            dataSize,
            data,
            info
        };

        uint32_t row = m_ui->receivedMessagesLogWindow->rowCount();
        m_ui->receivedMessagesLogWindow->insertRow(row);

        for (uint8_t column = 0; column < m_ui->receivedMessagesLogWindow->columnCount(); column++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(frameInfo.takeFirst());
            m_ui->receivedMessagesLogWindow->setItem(row, column, item);
        }
    }
}

void CannabusControl::busStatus()
{
    if (m_canDevice == nullptr || m_canDevice->hasBusStatus() == false)
    {
        m_ui->busStatus->setText(tr("No CAN bus status available."));
        m_busStatusTimer->stop();
        return;
    }

    switch(m_canDevice->busStatus())
    {
        case QCanBusDevice::CanBusStatus::Good:
        {
            m_ui->busStatus->setText(tr("CAN bus status: Good."));
            break;
        }
        case QCanBusDevice::CanBusStatus::Warning:
        {
            m_ui->busStatus->setText(tr("CAN bus status: Warning."));
            break;
        }
        case QCanBusDevice::CanBusStatus::Error:
        {
            m_ui->busStatus->setText(tr("CAN bus status: Error."));
            break;
        }
        case QCanBusDevice::CanBusStatus::BusOff:
        {
            m_ui->busStatus->setText(tr("CAN bus status: Bus Off."));
            break;
        }
        default:
        {
            m_ui->busStatus->setText(tr("CAN bus status: Unknown."));
            break;
        }
    }
}

