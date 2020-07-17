#include "connect_dialog.h"
#include "ui_connect_dialog.h"

#include <QCanBus>

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    ui->errorFilterEdit->setValidator(new QIntValidator(0, 0x1FFFFFFFU, this));

    ui->loopbackListBox->addItem(tr("true"), QVariant(true));
    ui->loopbackListBox->addItem(tr("false"), QVariant(false));
    ui->loopbackListBox->addItem(tr("unspecified"), QVariant());
    ui->loopbackListBox->setCurrentIndex(ui->loopbackListBox->findText("unspecified"));

    ui->receiveOwnListBox->addItem(tr("true"), QVariant(true));
    ui->receiveOwnListBox->addItem(tr("false"), QVariant(false));
    ui->receiveOwnListBox->addItem(tr("unspecified"), QVariant());
    ui->receiveOwnListBox->setCurrentIndex(ui->receiveOwnListBox->findText("unspecified"));

    ui->canFdListBox->addItem(tr("true"), QVariant(true));
    ui->canFdListBox->addItem(tr("false"), QVariant(false));
    ui->canFdListBox->setCurrentIndex(ui->canFdListBox->findText("false"));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectDialog::ok);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConnectDialog::cancel);

    connect(ui->useConfigurationBox, &QCheckBox::clicked,
            ui->configurationBox, &QGroupBox::setEnabled);
    connect(ui->pluginListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::pluginChanged);
    connect(ui->interfaceListBox, &QComboBox::currentTextChanged,
            this, &ConnectDialog::interfaceChanged);

    ui->pluginListBox->addItems(QCanBus::instance()->plugins());

    ui->isVirtual->setEnabled(false);
    ui->isFlexibleDataRateCapable->setEnabled(false);
    ui->configurationBox->setEnabled(ui->useConfigurationBox->isChecked());

    updateSettings();
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

ConnectDialog::Settings ConnectDialog::settings() const
{
    return currentSettings;
}

void ConnectDialog::pluginChanged(const QString &plugin)
{
    ui->interfaceListBox->clear();

    interfaces = QCanBus::instance()->availableDevices(plugin);
    for (const QCanBusDeviceInfo &info : qAsConst(interfaces))
    {
        ui->interfaceListBox->addItem(info.name());
    }
}

void ConnectDialog::interfaceChanged(const QString &interface)
{
    ui->descriptionLabel->setText(tr("Device: "));
    ui->serialNumberLabel->setText(tr("Serial: "));
    ui->channelLabel->setText(tr("Channel: "));
    ui->isVirtual->setChecked(false);
    ui->isFlexibleDataRateCapable->setChecked(false);

    for (const QCanBusDeviceInfo &info : qAsConst(interfaces))
    {
        if (info.name() == interface)
        {
            QString serialNumber = info.serialNumber();
            if (serialNumber.isEmpty() != false)
            {
                serialNumber = tr("N/A");
            }

            ui->descriptionLabel->setText(tr("Device: %1").arg(info.description()));
            ui->serialNumberLabel->setText(tr("Serial: %1").arg(serialNumber));
            ui->channelLabel->setText(tr("Channel: %1").arg(info.channel()));
            ui->isVirtual->setChecked(info.isVirtual());
            ui->isFlexibleDataRateCapable->setChecked(info.hasFlexibleDataRate());

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

    for (const ConfigurationItem &item : qAsConst(currentSettings.configurations))
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
    currentSettings.pluginName = ui->pluginListBox->currentText();
    currentSettings.deviceInterfaceName = ui->interfaceListBox->currentText();
    currentSettings.useConfigurationEnabled = ui->useConfigurationBox->isChecked();
    // Пользовательские настройки
    if (currentSettings.useConfigurationEnabled == true)
    {
        currentSettings.configurations.clear();

        if (ui->loopbackListBox->currentText() != "false")
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::LoopbackKey;
            item.second = ui->loopbackListBox->currentData();

            currentSettings.configurations.append(item);
        }

        if (ui->receiveOwnListBox->currentText() != "false")
        {
            ConfigurationItem item;

            item.first = QCanBusDevice::ReceiveOwnKey;
            item.second = ui->receiveOwnListBox->currentData();

            currentSettings.configurations.append(item);
        }
    }
}

void ConnectDialog::revertSettings()
{
    QString value;

    ui->pluginListBox->setCurrentText(currentSettings.pluginName);
    ui->interfaceListBox->setCurrentText(currentSettings.deviceInterfaceName);
    ui->useConfigurationBox->setChecked(currentSettings.useConfigurationEnabled);
    ui->configurationBox->setEnabled(ui->useConfigurationBox->isChecked());

    value = configurationValue(QCanBusDevice::RawFilterKey);
    ui->rawFilterEdit->setText(value);

    value = configurationValue(QCanBusDevice::ErrorFilterKey);
    ui->errorFilterEdit->setText(value);

    value = configurationValue(QCanBusDevice::LoopbackKey);
    ui->loopbackListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::ReceiveOwnKey);
    ui->receiveOwnListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::BitRateKey);
    ui->bitrateListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::CanFdKey);
    ui->canFdListBox->setCurrentText(value);

    value = configurationValue(QCanBusDevice::DataBitRateKey);
    ui->dataBitrateListBox->setCurrentText(value);
}
