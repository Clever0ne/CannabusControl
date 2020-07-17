#ifndef CONNECT_DIALOG_H
#define CONNECT_DIALOG_H

#include <QCanBusDevice>
#include <QCanBusDeviceInfo>

#include <QDialog>

QT_BEGIN_NAMESPACE

namespace Ui { class ConnectDialog; }

QT_END_NAMESPACE

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    typedef QPair<QCanBusDevice::ConfigurationKey, QVariant> ConfigurationItem;

    struct Settings {
        QString pluginName;
        QString deviceInterfaceName;
        QList<ConfigurationItem> configurations;
        bool useConfigurationEnabled = false;
    };

    explicit ConnectDialog(QWidget *parent = nullptr);
    ~ConnectDialog();

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

    Ui::ConnectDialog *ui = nullptr;
    Settings currentSettings;
    QList<QCanBusDeviceInfo> interfaces;
};

#endif // CONNECT_DIALOG_H
