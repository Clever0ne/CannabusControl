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

    m_written = new QLabel;
    m_ui->statusBar->addWidget(m_written);

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
        m_status->setText(tr("Error creating device '%1': '%2'")
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

