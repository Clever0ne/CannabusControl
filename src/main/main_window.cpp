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

    // Устанавливаем связь между чекбоксами настроек фильтра типов сообщений и
    // соответствующими методами-сеттерами (см. макросы)
    CONNECT_FILTER(HighPrioMaster);
    CONNECT_FILTER(HighPrioSlave);
    CONNECT_FILTER(Master);
    CONNECT_FILTER(Slave);
    CONNECT_FILTER(AllMsgTypes);

    // Устанавливаем связь между чекбоксами настроек фильтра F-кодов сообщений и
    // соответствующими методами-сеттерами (см. макросы)
    CONNECT_FILTER(ReadRegsRange);
    CONNECT_FILTER(ReadRegsSeries);
    CONNECT_FILTER(WriteRegsRange);
    CONNECT_FILTER(WriteRegsSeries);
    CONNECT_FILTER(DeviceSpecific_1);
    CONNECT_FILTER(DeviceSpecific_2);
    CONNECT_FILTER(DeviceSpecific_3);
    CONNECT_FILTER(DeviceSpecific_4);
    CONNECT_FILTER(AllFCodes);

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
    // Получаем указатель на настройки для адаптера
    const auto settings = m_settingsDialog->settings();

    // Сбрасываем настройки адаптера и настраиваем новый
    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(
                        settings.pluginName,
                        settings.deviceInterfaceName,
                        &errorString));

    // Если не удалось подключиться, выводим ошибку
    if (m_canDevice == nullptr)
    {
        m_status->setText(tr("Error creating device '%1': %2")
                        .arg(settings.pluginName)
                        .arg(errorString));
        return;
    }

    // Очищаем окно лога
    m_ui->logWindow->clearLog();

    // Устанавливаем связь между сигналом возникновения ошибки
    // и функцией-обработчиком ошибок
    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,
            this, &MainWindow::processError);

    // Настраиваем параметры конфигурации адаптера (битрейт, обратная связь, etc.)
    for (const SettingsDialog::ConfigurationItem &item : qAsConst(settings.configurations))
    {
        m_canDevice->setConfigurationParameter(item.first, item.second);
    }

    // Пробуем подключиться к шине адаптера
    // Если не удалось, выводим ошибку и сбрасываем настройки
    if (m_canDevice->connectDevice() == false)
    {
        m_status->setText(tr("Connection error: %1").arg(m_canDevice->errorString()));

        m_canDevice.reset();

        return;
    }

    // Делаем кнопку Connect недоступной, а Disconnect — доступной
    m_ui->actionConnect->setEnabled(false);
    m_ui->actionDisconnect->setEnabled(true);

    // Определяем битрейт и выводим его в поле статуса подключения
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

    // Если есть возможность определить статус шины, запускаем таймеры
    // Иначе выводим сообщения о невозможности определить статус шины
    if (m_canDevice->hasBusStatus() != false)
    {
        m_busStatusTimer->start(bus_status_timeout);
        m_logWindowUpdateTimer->start(log_window_update_timeout);
    }
    else
    {
        m_ui->busStatus->setText(tr("No CAN bus status available"));
    }
}

void MainWindow::disconnectDevice()
{
    // Ничего не делаем, если отключать и так нечего
    if (m_canDevice == nullptr)
    {
        return;
    }

    // Останавливаем таймеры
    m_busStatusTimer->stop();
    m_logWindowUpdateTimer->stop();

    // Обрабатываем полученные, но необработанные кадры
    processFramesReceived();

    // Отключаем адаптер
    m_canDevice->disconnectDevice();

    // Выводим сообщение о невозможности определить статус шины
    m_ui->busStatus->setText(tr("No CAN bus status available."));

    // Делаем кнопку Connect доступной, а Disconnect — недоступной
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);

    // Выводим в поле статуса сообщение об отключении
    m_status->setText("Disconnected");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settingsDialog->close();
    event->accept();
}

void MainWindow::processFramesReceived()
{
    // Ничего не делаем, если принимать кадры не от кого
    if (m_canDevice == nullptr)
    {
        return;
    }

    // Обрабатываем кадры в цикле
    while (m_canDevice->framesAvailable() != false)
    {
        // Вынимаем кадр из очереди
        const auto frame = m_canDevice->readFrame();

        // Обработка кадров ошибок
        if (frame.frameType() == QCanBusFrame::FrameType::ErrorFrame)
        {
            const QString errorInfo = m_canDevice->interpretErrorFrame(frame);
            m_ui->logWindow->processErrorFrame(frame, errorInfo);

            continue;
        }

        // Обработка обычных кадров
        m_ui->logWindow->processDataFrame(frame);
    }
}

void MainWindow::busStatus()
{
    // Выводим сообщение о невозможности определить статус шины и
    // останавливаем таймеры, если определять статус не у чего или
    // определить его невозможно
    if (m_canDevice == nullptr || m_canDevice->hasBusStatus() == false)
    {
        m_ui->busStatus->setText(tr("No CAN bus status available."));
        m_busStatusTimer->stop();
        m_logWindowUpdateTimer->stop();
        return;
    }

    // Определяем статус шины
    QString status;

    switch(m_canDevice->busStatus())
    {
        case QCanBusDevice::CanBusStatus::Good:
        {
            status = "Good";
            break;
        }
        case QCanBusDevice::CanBusStatus::Warning:
        {
            status = "Warning";
            break;
        }
        case QCanBusDevice::CanBusStatus::Error:
        {
            status = "Error";
            break;
        }
        case QCanBusDevice::CanBusStatus::BusOff:
        {
            status = "Bus Off";
            break;
        }
        default:
        {
            status = "Unknown.";
            break;
        }

        m_ui->busStatus->setText(tr("CAN bus status: %1.").arg(status));
    }
}

void MainWindow::setAllMsgTypesFiltrated()
{
    const bool isFiltrated = m_ui->filterAllMsgTypes->isChecked();

    m_ui->filterHighPrioMaster->setChecked(isFiltrated);
    m_ui->filterHighPrioSlave->setChecked(isFiltrated);
    m_ui->filterMaster->setChecked(isFiltrated);
    m_ui->filterSlave->setChecked(isFiltrated);
}

void MainWindow::setHighPrioMasterFiltrated()
{
    const bool isFiltrated = m_ui->filterHighPrioMaster->isChecked();

    m_ui->logWindow->setMsgTypeFiltrated(cannabus::IdMsgTypes::HIGH_PRIO_MASTER, isFiltrated);
}

void MainWindow::setHighPrioSlaveFiltrated()
{
    const bool isFiltrated = m_ui->filterHighPrioSlave->isChecked();

    m_ui->logWindow->setMsgTypeFiltrated(cannabus::IdMsgTypes::HIGH_PRIO_SLAVE, isFiltrated);
}

void MainWindow::setMasterFiltrated()
{
    const bool isFiltrated = m_ui->filterMaster->isChecked();

    m_ui->logWindow->setMsgTypeFiltrated(cannabus::IdMsgTypes::MASTER, isFiltrated);
}

void MainWindow::setSlaveFiltrated()
{
    const bool isFiltrated = m_ui->filterSlave->isChecked();

    m_ui->logWindow->setMsgTypeFiltrated(cannabus::IdMsgTypes::SLAVE, isFiltrated);
}

void MainWindow::setAllFCodesFiltrated()
{
    const bool isFiltrated = m_ui->filterAllFCodes->isChecked();

    m_ui->filterWriteRegsRange->setChecked(isFiltrated);
    m_ui->filterWriteRegsSeries->setChecked(isFiltrated);
    m_ui->filterReadRegsRange->setChecked(isFiltrated);
    m_ui->filterReadRegsSeries->setChecked(isFiltrated);
    m_ui->filterDeviceSpecific_1->setChecked(isFiltrated);
    m_ui->filterDeviceSpecific_2->setChecked(isFiltrated);
    m_ui->filterDeviceSpecific_3->setChecked(isFiltrated);
    m_ui->filterDeviceSpecific_4->setChecked(isFiltrated);
}

void MainWindow::setReadRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsRange->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::READ_REGS_RANGE, isFiltrated);
}

void MainWindow::setReadRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsSeries->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::READ_REGS_SERIES, isFiltrated);
}

void MainWindow::setWriteRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsRange->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_RANGE, isFiltrated);
}

void MainWindow::setWriteRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsSeries->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_SERIES, isFiltrated);
}

void MainWindow::setDeviceSpecific_1Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_1->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC1, isFiltrated);
}

void MainWindow::setDeviceSpecific_2Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_2->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC2, isFiltrated);
}

void MainWindow::setDeviceSpecific_3Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_3->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC3, isFiltrated);
}

void MainWindow::setDeviceSpecific_4Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_4->isChecked();

    m_ui->logWindow->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC4, isFiltrated);
}
