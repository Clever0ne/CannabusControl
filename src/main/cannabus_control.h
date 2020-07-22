#pragma once

#include <QMainWindow>
#include <QCanBusDevice>
#include <stdint.h>

QT_BEGIN_NAMESPACE

class QCanBusFrame;
class QLabel;
class QTimer;

namespace Ui { class CannabusControl; }

QT_END_NAMESPACE

class ConnectDialog;

class CannabusControl : public QMainWindow
{
    Q_OBJECT

public:
    explicit CannabusControl(QWidget *parent = nullptr);
    ~CannabusControl();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void connectDevice();
    void disconnectDevice();
    void busStatus();
    void processError(QCanBusDevice::CanBusError error) const;
    void processFramesReceived();

private:
    void initActionsConnections();

    uint64_t m_numberFramesReceived = 0;
    Ui::CannabusControl *m_ui = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_written = nullptr;
    ConnectDialog *m_connectDialog = nullptr;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    QTimer *m_logWindowUpdateTimer = nullptr;
    QTimer *m_busStatusTimer = nullptr;
};
