#pragma once

#include <QMainWindow>
#include <QCanBusDevice>
#include <stdint.h>

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

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void setAllFCodesFiltrated();
    void setReadRegsRangeFiltrated();
    void setReadRegsSeriesFiltrated();
    void setWriteRegsRangeFiltrated();
    void setWriteRegsSeriesFiltrated();
    void setDeviceSpecificFiltrated_1();
    void setDeviceSpecificFiltrated_2();
    void setDeviceSpecificFiltrated_3();
    void setDeviceSpecificFiltrated_4();

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
