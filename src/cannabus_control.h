#ifndef CANNABUS_CONTROL_H
#define CANNABUS_CONTROL_H

#include <QMainWindow>
#include <QCanBusDevice>

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
    CannabusControl(QWidget *parent = nullptr);
    ~CannabusControl();

private slots:
    void connectDevice();
    void disconnectDevice();
    void busStatus();

private:
    void initActionsConnections();

    Ui::CannabusControl *ui = nullptr;
    ConnectDialog *connectDialog = nullptr;
    std::unique_ptr<QCanBusDevice> canDevice;
    QTimer *busStatusTimer = nullptr;
};

#endif // CANNABUS_CONTROL_H
