#pragma once

#include <QTableWidget>
#include <QCanBusFrame>
#include <stdint.h>
#include "../cannabus_library/cannabus_common.h"

enum Column {
    COUNT,
    TIME,
    MSG_TYPE,
    SLAVE_ADDRESS,
    F_CODE,
    DATA_SIZE,
    DATA,
    MSG_INFO
};

class LogWindow : public QTableWidget
{
public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

    struct Filter {
        bool msgTypeSettings[(uint32_t)cannabus::IdMsgTypes::SLAVE + 1];
        bool fCodeSettings[(uint32_t)cannabus::IdFCode::DEVICE_SPECIFIC4 + 1];
        bool slaveAddressSettings[(uint32_t)cannabus::IdAddresses::MAX_SLAVE_ADDRESS + 1];
    };

    void setMsgTypeFiltrated(const cannabus::IdMsgTypes msgType, const bool isFiltrated);
    bool isMsgTypeFiltrated(const cannabus::IdMsgTypes msgType);

    void setFCodeFiltrated(const cannabus::IdFCode fCode, const bool isFiltrated);
    bool isFCodeFiltrated(const cannabus::IdFCode fCode);

    void setSlaveAddressFiltrated(const uint32_t slaveAddress, const bool isFiltrated);
    bool isSlaveAddressFiltrated(const uint32_t slaveAddress);

    void processDataFrame(const QCanBusFrame &frame);
    void processErrorFrame(const QCanBusFrame &frame, const QString errorInfo);

public slots:
    void clearLog();
    void setDefaultMessageFilterSettings();

private:
    void makeHeader();
    bool isDataFrameMustBeProcessed(const QCanBusFrame &frame);
    void setCount();
    void setTime(const uint64_t seconds, const uint64_t microseconds);
    void setMsgType(const cannabus::IdMsgTypes msgType);
    void setSlaveAddress(const uint32_t slaveAddress);
    void setFCode(const cannabus::IdFCode fCode);
    void setDataSize(const uint32_t dataSize);
    void setData(const QByteArray data);
    void setMsgInfo(const cannabus::IdMsgTypes msgType, const cannabus::IdFCode fCode, const uint32_t dataSize);
    void setMsgInfo(const QString errorInfo);

    uint64_t m_numberFramesReceived = 0;
    uint64_t m_currentRow = 0;

    QString m_count;
    QString m_time;
    QString m_msgType;
    QString m_slaveAddress;
    QString m_fCode;
    QString m_dataSize;
    QString m_data;
    QString m_msgInfo;

    Filter m_filter;
};
