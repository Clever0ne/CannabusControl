#pragma once

#include <QMainWindow>
#include <QCanBusDevice>
#include <stdint.h>

#define SUPER_CONNECT(sender, signal, receiver, slot) \
connect(sender, &std::remove_pointer<decltype(sender)>::type::signal, \
        receiver, &std::remove_pointer<decltype(receiver)>::type::slot)

#define CONNECT_FILTER(filter_name) SUPER_CONNECT(m_ui->filter##filter_name, stateChanged, this, set##filter_name##Filtrated);

QT_BEGIN_NAMESPACE

class QCanBusFrame;
class QLabel;
class QTimer;

namespace Ui { class MainWindow; }

QT_END_NAMESPACE

class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static constexpr uint32_t bus_status_timeout = 1000;
    static constexpr uint32_t log_window_update_timeout = 100;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void setAllMsgTypesFiltrated();
    void setHighPrioMasterFiltrated();
    void setHighPrioSlaveFiltrated();
    void setMasterFiltrated();
    void setSlaveFiltrated();

    void setAllFCodesFiltrated();
    void setReadRegsRangeFiltrated();
    void setReadRegsSeriesFiltrated();
    void setWriteRegsRangeFiltrated();
    void setWriteRegsSeriesFiltrated();
    void setDeviceSpecific_1Filtrated();
    void setDeviceSpecific_2Filtrated();
    void setDeviceSpecific_3Filtrated();
    void setDeviceSpecific_4Filtrated();

    void setSlaveAddressesFiltrated();

    void connectDevice();
    void disconnectDevice();
    void busStatus();
    void processError(QCanBusDevice::CanBusError error) const;
    void processFramesReceived();

private:
    void initActionsConnections();

    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    QTimer *m_busStatusTimer = nullptr;
    QTimer *m_logWindowUpdateTimer = nullptr;
};
