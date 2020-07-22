#pragma once

#include <QCanBusDevice>
#include <QCanBusDeviceInfo>

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui { class SettingsDialog; }

QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    typedef QPair<QCanBusDevice::ConfigurationKey, QVariant> ConfigurationItem;

    struct Settings {
        QString pluginName;
        QString deviceInterfaceName;
        QList<ConfigurationItem> configurations;
    };

    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    Settings settings() const;

private slots:
    void pluginChanged(const QString &plugin);
    void interfaceChanged(const QString &interface);
    void ok();
    void cancel();

private:
    QString configurationValue(QCanBusDevice::ConfigurationKey key);
    void updateSettings();
    void revertSettings();

    Ui::SettingsDialog *m_ui = nullptr;
    Settings m_currentSettings;
    QList<QCanBusDeviceInfo> m_interfaces;
};
