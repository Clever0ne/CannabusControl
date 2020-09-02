#include "main_window.h"
#include "ui_main_window.h"
#include "settings_dialog.h"
#include "bitrate.h"
#include "filter.h"
#include "../cannabus_library/cannabus_common.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>
#include <QTime>
#include <QDate>
#include <QTextStream>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>

#define SUPER_CONNECT(sender, signal, receiver, slot) \
connect(sender  , &std::remove_pointer<decltype(sender)>::type::signal, \
        receiver, &std::remove_pointer<decltype(receiver)>::type::slot)

#define CONNECT_FILTER(filter_name) SUPER_CONNECT(m_ui->filter##filter_name, stateChanged, this, set##filter_name##Filtrated);

using namespace cannabus;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_busStatusTimer(new QTimer(this)),
    m_logWindowUpdateTimer(new QTimer(this))

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    , m_sendMessageTimer(new QTimer(this))

#endif

    // ******************* Необходимо удалить после тестирования ******************
{
    m_ui->setupUi(this);

    m_settingsDialog = new SettingsDialog;

    m_filter = new Filter;

    m_status = new QLabel;
    m_ui->statusBar->addPermanentWidget(m_status);

    m_ui->actionDisconnect->setEnabled(false);

    QString fontName = "DroidSansMono.ttf";
    int32_t id = QFontDatabase::addApplicationFont(tr(":/fonts/%1").arg(fontName));

    QString fontFamily = QFontDatabase::applicationFontFamilies(id).at(0);
    int32_t fontSize = 8;
    int32_t fontWeight = QFont::Weight::Normal;
    QFont font = QFont(fontFamily, fontSize, fontWeight);

    m_ui->logWindow->setFont(font);
    m_ui->logWindow->clearLog();

    m_ui->contentFilterList->setFont(font);
    m_ui->contentFilterList->clearList();

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    m_slave.resize(slave_adresses_range_size);

    for (Slave &slave : m_slave)
    {
        slave.reg.fill(0x00, regs_range_size);
        slave.data.fill(0x00, data_range_size);
    }

#endif

    // ******************* Необходимо удалить после тестирования ******************

    initActionsConnections();
    initFiltersConnections();
}

MainWindow::~MainWindow()
{
    delete m_settingsDialog;
    delete m_filter;
    delete m_ui;
}

void MainWindow::initActionsConnections()
{   
    SUPER_CONNECT(m_ui->actionConnect            , triggered, this            , connectDevice           );
    SUPER_CONNECT(m_ui->actionDisconnect         , triggered, this            , disconnectDevice        );
    SUPER_CONNECT(m_ui->actionClearLog           , triggered, m_ui->logWindow , clearLog                );
    SUPER_CONNECT(m_ui->actionQuit               , triggered, this            , close                   );
    SUPER_CONNECT(m_ui->actionSettings           , triggered, m_settingsDialog, show                    );
    SUPER_CONNECT(m_ui->actionResetFilterSettings, triggered, this            , setDefaultFilterSettings);
    SUPER_CONNECT(m_ui->actionSaveLog            , triggered, this            , saveLog                 );

    SUPER_CONNECT(m_settingsDialog, accepted, this, disconnectDevice);
    SUPER_CONNECT(m_settingsDialog, accepted, this, connectDevice);
}

void MainWindow::initFiltersConnections()
{
    // Устанавливаем связь между фильтром и списком фильтров содержимого
    SUPER_CONNECT(m_ui->contentFilterList, addFilter          , m_filter               , setContentFilter   );
    SUPER_CONNECT(m_ui->contentFilterList, removeFilterAtIndex, m_filter               , removeContentFilter);
    SUPER_CONNECT(m_filter               , contentFilterAdded , m_ui->contentFilterList, setFilter          );

    // Устанавливаем связь между фильтров и полем ввода диапазонов адресов ведомых узлов
    SUPER_CONNECT(m_ui->filterSlaveAddresses, editingFinished          , this    , setSlaveAddressesFiltrated);
    SUPER_CONNECT(this                      , addSlaveAdressesFilter   , m_filter, setSlaveAddressFilter     );
    SUPER_CONNECT(m_filter                  , slaveAddressesFilterAdded, this    , setFilter                 );

    // Устанавливаем связь между фильтром и окном лога
    SUPER_CONNECT(m_filter, frameIsProcessing, m_ui->logWindow, numberFramesReceivedIncrement);

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

    // Устанавливаем связь между таймерами и соответствующими событиями
    SUPER_CONNECT(m_busStatusTimer, timeout, this, busStatus);
    SUPER_CONNECT(m_logWindowUpdateTimer, timeout, this, processFramesReceived);

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    SUPER_CONNECT(m_sendMessageTimer, timeout, this, emulateSendMessage);

#endif

    // ******************* Необходимо удалить после тестирования ******************
}

void MainWindow::saveLog()
{
    QString currentTime = QTime::currentTime().toString().replace(":", "-");
    QString currentDate = QDate::currentDate().toString(Qt::ISODate);

    QString name = tr("message_log_%1_%2").arg(currentDate).arg(currentTime);

    QString filters("CSV files (*.csv);;All files (*.*)");
    QString defaultFilter("CSV files (*.csv)");
    QString fileName = QFileDialog::getSaveFileName(nullptr, "Save Message Log",
                                                    QCoreApplication::applicationDirPath() + "/" + name + ".csv",
                                                    filters, &defaultFilter);

    QFile saveFile(fileName);

    if (saveFile.open(QIODevice::WriteOnly) != false)
    {
        QTextStream data(&saveFile);
        QStringList stringList;

        for (int32_t column = 0; column < m_ui->logWindow->horizontalHeader()->count(); column++)
        {
            QString text = m_ui->logWindow->horizontalHeaderItem(column)->text();

            if (text.length() > 0)
            {
                stringList.append("\"" + text + "\"");
            }
            else
            {
                stringList.append("");
            }
        }

        data << stringList.join(";") + "\n";

        for(int32_t row = 0; row < m_ui->logWindow->verticalHeader()->count(); row++)
        {
            stringList.clear();

            for(int32_t column = 0; column < m_ui->logWindow->horizontalHeader()->count(); column++)
            {
                QString text = m_ui->logWindow->item(row, column)->text();

                if (text.length() > 0)
                {
                    stringList.append("\"" + text + "\"");
                }
                else
                {
                    stringList.append("");
                }
            }

            data << stringList.join(";") + "\n";
        }

        saveFile.close();
    }
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

// ***************** Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

void MainWindow::emulateSendMessage()
{
    static bool isResponse = false;

    static std::random_device randomNumber;
    static std::mt19937 gen(randomNumber());

    static std::uniform_int_distribution<> slaveAddressRange((uint32_t)IdAddresses::MIN_SLAVE_ADDRESS,
                                                             (uint32_t)IdAddresses::MAX_SLAVE_ADDRESS);

    static std::uniform_int_distribution<> fCodeRange((uint32_t)IdFCode::WRITE_REGS_RANGE,
                                                      (uint32_t)IdFCode::READ_REGS_SERIES);

    static std::uniform_int_distribution<> msgTypeRange((uint32_t)IdMsgTypes::HIGH_PRIO_MASTER,
                                                        (uint32_t)IdMsgTypes::SLAVE);

    static std::uniform_int_distribution<> rangeLengthRange(2, max_regs_in_range);
    static std::uniform_int_distribution<> seriesLengthRange(1, max_regs_in_series);

    static std::uniform_int_distribution<> regRange(0x00, 0xFF);
    static std::uniform_int_distribution<> byteRange(0x00, 0xFF);

    static IdAddresses slaveAddress;
    static IdFCode fCode;
    static IdMsgTypes msgType;

    static uint32_t id;

    static QByteArray data;

    if (isResponse == false)
    {
        data.clear();

        slaveAddress = IdAddresses(slaveAddressRange(gen));
        fCode = IdFCode(fCodeRange(gen));
        msgType = IdMsgTypes(msgTypeRange(gen));

        while (msgType == IdMsgTypes::HIGH_PRIO_SLAVE || msgType == IdMsgTypes::SLAVE)
        {
            msgType = IdMsgTypes(msgTypeRange(gen));
        }

        id = makeId(slaveAddress, fCode, msgType);

        switch(fCode)
        {
            case IdFCode::WRITE_REGS_RANGE:
            {
                uint8_t rangeLenght = rangeLengthRange(gen);

                uint8_t regBegin = regRange(gen);
                uint8_t regEnd = regBegin + (rangeLenght - 1);

                if (regEnd < regBegin)
                {
                    regEnd = regBegin;
                    regBegin = regEnd - (rangeLenght - 1);
                }

                data.append(regBegin);
                data.append(regEnd);

                for (uint32_t byteNumber = 0; byteNumber < rangeLenght; byteNumber++)
                {
                    uint8_t byte = rand();
                    data.append(byte);
                }

                break;
            }
            case IdFCode::WRITE_REGS_SERIES:
            {
                uint8_t seriesLenght = seriesLengthRange(gen);

                QVector<uint8_t> regAddress;

                for (uint32_t regNumber = 0; regNumber <= seriesLenght; regNumber++)
                {
                    uint8_t reg = regRange(gen);

                    while (regAddress.contains(reg) != false)
                    {
                        reg = regRange(gen);
                    }

                    regAddress.append(reg);;
                }

                for (uint32_t byteNumber = 0; byteNumber < seriesLenght; byteNumber++)
                {
                    uint8_t byte = byteRange(gen);

                    data.append(regAddress.takeFirst());
                    data.append(byte);
                }

                break;
            }
            case IdFCode::READ_REGS_RANGE:
            {
                uint8_t rangeLenght = rangeLengthRange(gen);

                uint8_t regBegin = regRange(gen);
                uint8_t regEnd = regBegin + (rangeLenght - 1);

                if (regEnd < regBegin)
                {
                    regEnd = regBegin;
                    regBegin = regEnd - (rangeLenght - 1);
                }

                data.append(regBegin);
                data.append(regEnd);

                break;
            }
            case IdFCode::READ_REGS_SERIES:
            {
                uint8_t seriesLenght = seriesLengthRange(gen);

                QVector<uint8_t> regAddress;

                for (uint32_t regNumber = 0; regNumber <= seriesLenght; regNumber++)
                {
                    uint8_t reg = regRange(gen);

                    while (regAddress.contains(reg) != false)
                    {
                        reg = regRange(gen);
                    }

                    regAddress.append(reg);;
                }

                for (uint32_t byteNumber = 0; byteNumber < seriesLenght; byteNumber++)
                {
                    data.append(regAddress.takeFirst());
                }

                break;
            }
            default:
            {
                break;
            }
        }

        isResponse = true;
    }
    else
    {
        msgType = IdMsgTypes::SLAVE;

        id = makeId(slaveAddress, fCode, msgType);

        switch (fCode)
        {
            case IdFCode::WRITE_REGS_RANGE:
            {
                uint32_t left = static_cast<uint8_t>(data[0]);
                uint32_t right = static_cast<uint8_t>(data[1]);

                for (uint32_t reg = left; reg <= right; reg++)
                {
                    uint32_t index = 2 + reg - left;
                    m_slave[(uint32_t)slaveAddress].data[reg] = static_cast<uint8_t>(data[index]);
                }

                data.clear();

                data.append(left);
                data.append(right);

                break;
            }
            case IdFCode::WRITE_REGS_SERIES:
            {
                QByteArray regAddress;

                for (int32_t index = 0; index < data.size(); index++)
                {
                    uint32_t reg = static_cast<uint8_t>(data[index]);
                    m_slave[(uint32_t)slaveAddress].data[reg] = static_cast<uint8_t>(data[++index]);

                    regAddress.append(reg);
                }

                data.clear();

                data = regAddress;

                break;
            }
            case IdFCode::READ_REGS_RANGE:
            {
                uint32_t left = static_cast<uint8_t>(data[0]);
                uint32_t right = static_cast<uint8_t>(data[1]);

                for (uint32_t reg = left; reg <= right; reg++)
                {
                    data.append(m_slave[(uint32_t)slaveAddress].data[reg]);
                }

                break;
            }
            case IdFCode::READ_REGS_SERIES:
            {
                QByteArray regAddress = data;

                data.clear();

                for (int32_t index = 0; index < regAddress.size(); index++)
                {
                    uint32_t reg = static_cast<uint8_t>(regAddress[index]);

                    data.append(reg);
                    data.append(m_slave[(uint32_t)slaveAddress].data[reg]);
                }

                break;
            }
            case IdFCode::DEVICE_SPECIFIC1:
            case IdFCode::DEVICE_SPECIFIC2:
            case IdFCode::DEVICE_SPECIFIC3:
            case IdFCode::DEVICE_SPECIFIC4:
            default:
            {
                break;
            }
        }

        isResponse = false;
    }

    auto frame = QCanBusFrame(id, data);

    m_queue.enqueue(frame);
}

#endif

// *********************** Необходимо удалить после тестирования ******************

void MainWindow::connectDevice()
{
    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    m_ui->actionConnect->setEnabled(false);
    m_ui->actionDisconnect->setEnabled(true);
    m_ui->logWindow->clearLog();

    m_logWindowUpdateTimer->start(log_window_update_timeout);
    m_sendMessageTimer->start(send_message_timeout);
    m_status->setText("Connected");
    return;

#endif

    // ******************* Необходимо удалить после тестирования ******************

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
    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    m_sendMessageTimer->stop();
    processFramesReceived();

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);

    m_status->setText("Disconnected");
    return;

#endif

    // ******************* Необходимо удалить после тестирования ******************

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
    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    while (m_queue.isEmpty() == false)
    {
        auto frame = m_queue.dequeue();

        if (m_filter->mustDataFrameBeProcessed(frame) != false)
        {
            m_ui->logWindow->processDataFrame(frame);
        }
    }

#endif

    // ******************* Необходимо удалить после тестирования ******************

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
        if (m_filter->mustDataFrameBeProcessed(frame) != false)
        {
            m_ui->logWindow->processDataFrame(frame);
        }
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
    }

    m_ui->busStatus->setText(tr("CAN bus status: %1.").arg(status));
}

void MainWindow::setSlaveAddressesFiltrated()
{
    // Получаем диапазон фильтруемых адресов в виде строки
    QString addressesRange = m_ui->filterSlaveAddresses->text();

    // Отправляем запрос на добавление фильтра адресов ведомых узлом
    emit addSlaveAdressesFilter(addressesRange);
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

    m_filter->setMsgTypeFiltrated(cannabus::IdMsgTypes::HIGH_PRIO_MASTER, isFiltrated);
}

void MainWindow::setHighPrioSlaveFiltrated()
{
    const bool isFiltrated = m_ui->filterHighPrioSlave->isChecked();

    m_filter->setMsgTypeFiltrated(cannabus::IdMsgTypes::HIGH_PRIO_SLAVE, isFiltrated);
}

void MainWindow::setMasterFiltrated()
{
    const bool isFiltrated = m_ui->filterMaster->isChecked();

    m_filter->setMsgTypeFiltrated(cannabus::IdMsgTypes::MASTER, isFiltrated);
}

void MainWindow::setSlaveFiltrated()
{
    const bool isFiltrated = m_ui->filterSlave->isChecked();

    m_filter->setMsgTypeFiltrated(cannabus::IdMsgTypes::SLAVE, isFiltrated);
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

void MainWindow::setWriteRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsRange->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_RANGE, isFiltrated);
}

void MainWindow::setWriteRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterWriteRegsSeries->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::WRITE_REGS_SERIES, isFiltrated);
}

void MainWindow::setReadRegsRangeFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsRange->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::READ_REGS_RANGE, isFiltrated);
}

void MainWindow::setReadRegsSeriesFiltrated()
{
    const bool isFiltrated = m_ui->filterReadRegsSeries->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::READ_REGS_SERIES, isFiltrated);
}

void MainWindow::setDeviceSpecific_1Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_1->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC1, isFiltrated);
}

void MainWindow::setDeviceSpecific_2Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_2->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC2, isFiltrated);
}

void MainWindow::setDeviceSpecific_3Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_3->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC3, isFiltrated);
}

void MainWindow::setDeviceSpecific_4Filtrated()
{
    const bool isFiltrated = m_ui->filterDeviceSpecific_4->isChecked();

    m_filter->setFCodeFiltrated(cannabus::IdFCode::DEVICE_SPECIFIC4, isFiltrated);
}

void MainWindow::setDefaultFilterSettings()
{
    m_ui->filterSlaveAddresses->setText("");
    m_ui->filterSlaveAddresses->editingFinished();

    m_ui->filterAllMsgTypes->setChecked(false);
    m_ui->filterAllMsgTypes->setChecked(true);

    m_ui->filterAllFCodes->setChecked(false);
    m_ui->filterAllFCodes->setChecked(true);

    m_ui->contentFilterList->clearList();
}

void MainWindow::setFilter(const QString addressesRange)
{
    m_ui->filterSlaveAddresses->setText(addressesRange);
}

#undef CONNECT_FILTER
#undef SUPER_CONNECT
