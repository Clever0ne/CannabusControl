#include "main_window.h"
#include "ui_main_window.h"
#include "settings_dialog.h"
#include "bitrate.h"
#include "../cannabus_library/cannabus_common.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_busStatusTimer(new QTimer(this)),
    m_logWindowUpdateTimer(new QTimer(this))
{
    m_ui->setupUi(this);

    m_settingsDialog = new SettingsDialog;

    m_status = new QLabel;
    m_ui->statusBar->addPermanentWidget(m_status);

    initActionsConnections();
}

MainWindow::~MainWindow()
{
    delete m_settingsDialog;
    delete m_ui;
}

void MainWindow::initActionsConnections()
{
    m_ui->actionDisconnect->setEnabled(false);

    connect(m_ui->actionSettings, &QAction::triggered,
            m_settingsDialog, &QDialog::show);
    connect(m_ui->actionConnect, &QAction::triggered,
            this, &MainWindow::connectDevice);
    connect(m_ui->actionDisconnect, &QAction::triggered,
            this, &MainWindow::disconnectDevice);
    connect(m_ui->actionClearLog, &QAction::triggered,
            m_ui->logWindow, &LogWindow::clearLog);
    connect(m_ui->actionQuit, &QAction::triggered,
            this, &QWidget::close);

    connect(m_settingsDialog, &QDialog::accepted, [this] {
        disconnectDevice();
        connectDevice();
    });

    connect(m_ui->filterReadRegsRange, &QCheckBox::stateChanged,
            this, &MainWindow::setReadRegsRangeFiltrated);
    connect(m_ui->filterReadRegsSeries, &QCheckBox::stateChanged,
            this, &MainWindow::setReadRegsSeriesFiltrated);
    connect(m_ui->filterWriteRegsRange, &QCheckBox::stateChanged,
            this, &MainWindow::setWriteRegsRangeFiltrated);
    connect(m_ui->filterWriteRegsSeries, &QCheckBox::stateChanged,
            this, &MainWindow::setWriteRegsSeriesFiltrated);
    connect(m_ui->filterDeviceSpecific, &QCheckBox::stateChanged,
            this, &MainWindow::setDeviceSpecificFiltrated);
    connect(m_ui->filterAllMessageTypes, &QCheckBox::stateChanged,
            this, &MainWindow::setAllMessageTypesFiltrated);

    connect(m_busStatusTimer, &QTimer::timeout,
            this, &MainWindow::busStatus);
    connect(m_logWindowUpdateTimer, &QTimer::timeout,
            this, &MainWindow::processFramesReceived);
}

void MainWindow::processError(QCanBusDevice::CanBusError error) const
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

void MainWindow::connectDevice()
{
    const SettingsDialog::Settings settings = m_settingsDialog->settings();

    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(
                        settings.pluginName,
                        settings.deviceInterfaceName,
                        &errorString));

    if (m_canDevice == nullptr)
    {
        m_status->setText(tr("Error creating device '%1': %2")
                        .arg(settings.pluginName)
                        .arg(errorString));
        return;
    }

    m_ui->logWindow->clearLog();

    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,
            this, &MainWindow::processError);

    for (const SettingsDialog::ConfigurationItem &item : qAsConst(settings.configurations))
    {
        m_canDevice->setConfigurationParameter(item.first, item.second);
    }

    if (m_canDevice->connectDevice() == false)
    {
        m_status->setText(tr("Connection error: %1").arg(m_canDevice->errorString()));

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
                m_status->setText(tr("Plugin '%1': connected to %2 at %3 / %4 with CAN FD")
                                .arg(settings.pluginName)
                                .arg(settings.deviceInterfaceName)
                                .arg(bitRateToString(bitRate.toUInt()))
                                .arg(bitRateToString(dataBitRate.toUInt())));
            }
            else
            {
                m_status->setText(tr("Plugin '%1': connected to %2 at %3")
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
            m_logWindowUpdateTimer->start(100);
        }
        else
        {
            m_ui->busStatus->setText(tr("No CAN bus status available"));
        }
    }
}

void MainWindow::disconnectDevice()
{
    if (m_canDevice == nullptr)
    {
        return;
    }

    m_busStatusTimer->stop();
    m_logWindowUpdateTimer->stop();

    processFramesReceived();

    m_canDevice->disconnectDevice();

    m_ui->busStatus->setText(tr("No CAN bus status available."));

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);

    m_status->setText("Disconnected");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settingsDialog->close();
    event->accept();
}

void MainWindow::processFramesReceived()
{
    if (m_canDevice == nullptr)
    {
        return;
    }

    while (m_canDevice->framesAvailable() != false)
    {
        const QCanBusFrame frame = m_canDevice->readFrame();

        // Обработка кадров ошибок
        if (frame.frameType() == QCanBusFrame::FrameType::ErrorFrame)
        {
            const QString errorInfo = m_canDevice->interpretErrorFrame(frame);
            m_ui->logWindow->processErrorFrame(frame, errorInfo);
        }

        // Обработка обычных кадров
        m_ui->logWindow->processDataFrame(frame);
    }
}

void MainWindow::busStatus()
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

void MainWindow::setAllMessageTypesFiltrated()
{
    const bool isFiltrated = m_ui->filterAllMessageTypes->isChecked();

    m_ui->filterWriteRegsRange->setChecked(isFiltrated);
    m_ui->filterWriteRegsSeries->setChecked(isFiltrated);
    m_ui->filterReadRegsRange->setChecked(isFiltrated);
    m_ui->filterReadRegsSeries->setChecked(isFiltrated);
    m_ui->filterDeviceSpecific->setChecked(isFiltrated);
}

void MainWindow::setReadRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsRange->isChecked();

    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::READ_REGS_RANGE, isFiltrated);
}

void MainWindow::setReadRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsSeries->isChecked();

    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::READ_REGS_SERIES, isFiltrated);
}

void MainWindow::setWriteRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsRange->isChecked();

    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_RANGE, isFiltrated);
}

void MainWindow::setWriteRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsSeries->isChecked();

    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_SERIES, isFiltrated);
}

void MainWindow::setDeviceSpecificFiltrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific->isChecked();

    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC1, isFiltrated);
    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC2, isFiltrated);
    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC3, isFiltrated);
    m_ui->logWindow->setMsgFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC4, isFiltrated);
}
