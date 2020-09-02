#pragma once

#include <QMainWindow>
#include <QCanBusDevice>
#include <stdint.h>

//#define EMULATION_ENABLED

// ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

#include <QQueue>
#include <random>

#endif

// ******************* Необходимо удалить после тестирования ******************

QT_BEGIN_NAMESPACE

class QCanBusFrame;
class QLabel;
class QTimer;

namespace Ui { class MainWindow; }

QT_END_NAMESPACE

class SettingsDialog;
class Filter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static constexpr uint32_t bus_status_timeout = 1000;
    static constexpr uint32_t log_window_update_timeout = 100;

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    static constexpr uint32_t send_message_timeout = 100;
    static constexpr uint32_t regs_range_size = 256;
    static constexpr uint32_t data_range_size = 256;
    static constexpr uint32_t slave_adresses_range_size = 61;

    struct Slave {
        QVector<uint8_t> reg;
        QVector<uint8_t> data;
    };

#endif

    // ******************* Необходимо удалить после тестирования ******************

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

    void setFilter(const QString addressesRange);

    void setDefaultFilterSettings();

    void connectDevice();
    void disconnectDevice();
    void busStatus();
    void processError(QCanBusDevice::CanBusError error) const;
    void processFramesReceived();
    void saveLog();

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    void emulateSendMessage();

#endif

    // ******************* Необходимо удалить после тестирования ******************

signals:
    void addSlaveAdressesFilter(QString addressesRange);

private:
    void initActionsConnections();
    void initFiltersConnections();

    Ui::MainWindow *m_ui = nullptr;
    QLabel *m_status = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;
    Filter *m_filter = nullptr;
    std::unique_ptr<QCanBusDevice> m_canDevice;
    QTimer *m_busStatusTimer = nullptr;
    QTimer *m_logWindowUpdateTimer = nullptr;

    // ************* Эмуляция общения между ведущим и ведомыми узлами *************

#ifdef EMULATION_ENABLED

    QVector<Slave> m_slave;
    QTimer *m_sendMessageTimer = nullptr;
    QQueue<QCanBusFrame> m_queue;

#endif

    // ******************* Необходимо удалить после тестирования ******************
};
