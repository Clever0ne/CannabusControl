#pragma once

#include <QCanBusFrame>
#include <QObject>
#include <stdint.h>
#include "../cannabus_library/cannabus_common.h"

class Filter : public QObject
{
    Q_OBJECT

public:
    explicit Filter(QObject *parent = nullptr);

    static constexpr uint32_t id_addresses_size = 62;
    static constexpr uint32_t id_msg_types_size = 4;
    static constexpr uint32_t id_f_code_size = 8;

    typedef QPair<QVector<uint8_t>, QVector<uint8_t>> Content;

    struct Settings {
        QVector<bool> slaveAddress;
        QVector<bool> msgType;
        QVector<bool> fCode;
        QVector<Content> content;
    };

    void setSlaveAddressFiltrated(const uint32_t slaveAddress, const bool isFiltrated);
    bool isSlaveAddressFiltrated(const uint32_t slaveAddress) const;

    void setMsgTypeFiltrated(const cannabus::IdMsgTypes msgType, const bool isFiltrated);
    bool isMsgTypeFiltrated(const cannabus::IdMsgTypes msgType) const;

    void setFCodeFiltrated(const cannabus::IdFCode fCode, const bool isFiltrated);
    bool isFCodeFiltrated(const cannabus::IdFCode fCode) const;

    void setContentFiltrated(const QVector<uint8_t> regs, const QVector<uint8_t> data);
    bool isContentFiltrated(const cannabus::IdMsgTypes msgType, const cannabus::IdFCode fCode, QByteArray dataArray) const;

    bool isPairRegDataFiltrated(const uint8_t reg, const uint8_t data) const;

    void fillSlaveAddressSettings(const bool isFiltrated);
    void fillMsgTypesSettings(const bool isFiltrated);
    void fillFCodeSettings(const bool isFiltrated);

    bool mustDataFrameBeProcessed(const QCanBusFrame &frame);

    QString rangesVectorToString(const QVector<uint8_t> ranges, const int32_t base);
    QVector<uint8_t> rangesStringToVector(const QString ranges, const int32_t base);

public slots:
    void setSlaveAddressFilter(QString addressesRange);
    void setContentFilter(QString regsRange, QString dataRange);
    void removeContentFilter(const int32_t index);

signals:
    void frameIsProcessing();
    void slaveAddressesFilterAdded(const QString addressesRange);
    void contentFilterAdded(const QString regsRange, const QString dataRange);

private:
    Settings m_settings;
};
