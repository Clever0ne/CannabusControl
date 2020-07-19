#ifndef CANNABUS_CONTROL_H
#define CANNABUS_CONTROL_H

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

private slots:
    void connectDevice();
    void disconnectDevice();
    void busStatus();
    void processError(QCanBusDevice::CanBusError error) const;
    void processFramesReceived();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initActionsConnections();

    uint64_t numberFramesReceived = 0;
    Ui::CannabusControl *ui = nullptr;
    QLabel *status = nullptr;
    QLabel *written = nullptr;
    ConnectDialog *connectDialog = nullptr;
    std::unique_ptr<QCanBusDevice> canDevice;
    QTimer *busStatusTimer = nullptr;
};

#endif // CANNABUS_CONTROL_H
