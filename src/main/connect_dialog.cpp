#include "connect_dialog.h"
#include "ui_connect_dialog.h"

#include <QCanBus>

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::ConnectDialog)
{
    m_ui->setupUi(this);

    m_ui->rawFilterListBox->addItem(tr("base format"), QVariant(QCanBusDevice::Filter::MatchBaseFormat));
    m_ui->rawFilterListBox->addItem(tr("extended format"), QVariant(QCanBusDevice::Filter::MatchExtendedFormat));
    m_ui->rawFilterListBox->addItem(tr("base and extended format"), QVariant(QCanBusDevice::Filter::MatchBaseAndExtendedFormat));
    m_ui->rawFilterListBox->setCurrentIndex(m_ui->rawFilterListBox->findText("base format"));

    m_ui->errorFilterEdit->setValidator(new QIntValidator(QCanBusFrame::NoError, QCanBusFrame::AnyError, this));

    m_ui->loopbackListBox->addItem(tr("true"), QVariant(true));
    m_ui->loopbackListBox->addItem(tr("false"), QVariant(false));
    m_ui->loopbackListBox->addItem(tr("unspecified"), QVariant());
    m_ui->loopbackListBox->setCurrentIndex(m_ui->loopbackListBox->findText("unspecified"));

    m_ui->receiveOwnListBox->addItem(tr("true"), QVariant(true));
    m_ui->receiveOwnListBox->addItem(tr("false"), QVariant(false));
    m_ui->receiveOwnListBox->addItem(tr("unspecified"), QVariant());
    m_ui->receiveOwnListBox->setCurrentIndex(m_ui->receiveOwnListBox->findText("unspecified"));

    m_ui->canFdListBox->addItem(tr("true"), QVariant(true));
    m_ui->canFdListBox->addItem(tr("false"), QVariant(false));
    m_ui->canFdListBox->setCurrentIndex(m_ui->canFdListBox->findText("false"));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectDialog::ok);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &ConnectDialog::cancel);

    connect(m_ui->useCustomConfigurationCheckBox, &QCheckBox::clicked,
            m_ui->configurationBox, &QGroupBox::setEnabled);
    connect(m_ui->pluginListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::pluginChanged);
    connect(m_ui->interfaceListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::interfaceChanged);

    m_ui->pluginListBox->addItems(QCanBus::instance()->plugins());

    m_ui->isVirtual->setEnabled(false);
    m_ui->isFlexibleDataRateCapable->setEnabled(false);
    m_ui->configurationBox->setEnabled(m_ui->useCustomConfigurationCheckBox->isChecked());

    updateSettings();
}

ConnectDialog::~ConnectDialog()
{
    delete m_ui;
}

ConnectDialog::Settings ConnectDialog::settings() const
{
    return m_currentSettings;
}

void ConnectDialog::pluginChanged(const QString &plugin)
{
    m_ui->interfaceListBox->clear();

    m_interfaces = QCanBus::instance()->availableDevices(plugin);
    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
    {
        m_ui->interfaceListBox->addItem(info.name());
    }
}

void ConnectDialog::interfaceChanged(const QString &interface)
{
    m_ui->descriptionLabel->setText(tr("Interface: "));
    m_ui->serialNumberLabel->setText(tr("Serial: "));
    m_ui->channelLabel->setText(tr("Channel: "));
    m_ui->isVirtual->setChecked(false);
    m_ui->isFlexibleDataRateCapable->setChecked(false);

    for (const QCanBusDeviceInfo &info : qAsConst(m_interfaces))
    {
        if (info.name() == interface)
        {
            QString serialNumber = info.serialNumber();
            if (serialNumber.isEmpty() != false)
            {
                serialNumber = tr("N/A");
            }

            m_ui->descriptionLabel->setText(tr("Interface: %1").arg(info.description()));
            m_ui->serialNumberLabel->setText(tr("Serial: %1").arg(serialNumber));
            m_ui->channelLabel->setText(tr("Channel: %1").arg(info.channel()));
            m_ui->isVirtual->setChecked(info.isVirtual());
            m_ui->isFlexibleDataRateCapable->setChecked(info.hasFlexibleDataRate());

            break;
        }
    }
}

void ConnectDialog::ok()
{
    updateSettings();
    accept();
}

void ConnectDialog::cancel()
{
    revertSettings();
    reject();
}

QString ConnectDialog::configurationValue(QCanBusDevice::ConfigurationKey key)
{
    QVariant result;

    for (const ConfigurationItem &item : qAsConst(m_currentSettings.configurations))
    {
        if (item.first == key)
        {
            result = item.second;
            break;
        }
    }

    if (result.isNull() != false && (key == QCanBusDevice::LoopbackKey || key == QCanBusDevice::ReceiveOwnKey))
    {
        return tr("unspecified");
    }

    return result.toString();
}

void ConnectDialog::updateSettings()
{
    m_currentSettings.pluginName = m_ui->pluginListBox->currentText();
    m_currentSettings.deviceInterfaceName = m_ui->interfaceListBox->currentText();
    m_currentSettings.isCustomConfigurationEnabled = m_ui->useCustomConfigurationCheckBox->isChecked();
    // Пользовательские настройки
    if (m_currentSettings.isCustomConfigurationEnabled == true)
    {
        m_currentSettings.configurations.clear();

        // Формат кадра данных шины CAN
        if (m_ui->rawFilterListBox->currentText().isEmpty() == false)
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::RawFilterKey;
            item.second = m_ui->rawFilterListBox->currentData();

            m_currentSettings.configurations.append(item);
        }

        // Режим обратной связи
        if (m_ui->loopbackListBox->currentData() != "false")
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::LoopbackKey;
            item.second = m_ui->loopbackListBox->currentData();

            m_currentSettings.configurations.append(item);
        }

        // Режим приёма адаптером собственных сообщений
        if (m_ui->receiveOwnListBox->currentText() != "false")
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::ReceiveOwnKey;
            item.second = m_ui->receiveOwnListBox->currentData();

            m_currentSettings.configurations.append(item);
        }

        if (m_ui->errorFilterEdit->text().isEmpty() == false)
        {

        }

        // Скорость передачи битов поля арбитража в кадре данных
        if (m_ui->bitRateListBox->bitRate() != 0)
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::BitRateKey;
            item.second = m_ui->bitRateListBox->bitRate();

            m_currentSettings.configurations.append(item);
        }

        // Использование шины CAN FD
        if (m_ui->canFdListBox->currentText() != "false")
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::CanFdKey;
            item.second = m_ui->canFdListBox->currentData();

            m_currentSettings.configurations.append(item);
        }

        // Скорость передачи битов поля данных в кадре данных
        if (m_ui->dataBitRateListBox->bitRate() != 0)
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::DataBitRateKey;
            item.second = m_ui->dataBitRateListBox->bitRate();

            m_currentSettings.configurations.append(item);
        }
    }
}

void ConnectDialog::revertSettings()
{
    QString value;

    m_ui->pluginListBox->setCurrentText(m_currentSettings.pluginName);
    m_ui->interfaceListBox->setCurrentText(m_currentSettings.deviceInterfaceName);
    m_ui->useCustomConfigurationCheckBox->setChecked(m_currentSettings.isCustomConfigurationEnabled);
    m_ui->configurationBox->setEnabled(m_ui->useCustomConfigurationCheckBox->isChecked());

    value = configurationValue(QCanBusDevice::RawFilterKey);
    m_ui->rawFilterListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::ErrorFilterKey);
    m_ui->errorFilterEdit->setText(value);

    value = configurationValue(QCanBusDevice::LoopbackKey);
    m_ui->loopbackListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::ReceiveOwnKey);
    m_ui->receiveOwnListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::BitRateKey);
    m_ui->bitRateListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::CanFdKey);
    m_ui->canFdListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::DataBitRateKey);
    m_ui->dataBitRateListBox->setCurrentText(value);
}
