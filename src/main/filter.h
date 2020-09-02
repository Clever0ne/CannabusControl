/****************************************************************************

Класс Filter обеспечивает хранение информации о текущих настройках фильтрации
сообщений, передаваемых по протоколу CannabusPlus, а также методы, сигналы и
слоты для установки и/или удаления фильтров адресов ведомых узлов, типов
сообщений, кодов функций (F-кодов) и содержимого (регистров и данных).

****************************************************************************/

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
    ~Filter() = default;

    // Размеры векторов адресов, типов сообщений и F-кодов
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

    // Сеттер и геттер фильтра адресов
    void setSlaveAddressFiltrated(const uint32_t slaveAddress, const bool isFiltrated);
    bool isSlaveAddressFiltrated(const uint32_t slaveAddress) const;

    // Сеттер и геттер фильтра типов сообщений
    void setMsgTypeFiltrated(const cannabus::IdMsgTypes msgType, const bool isFiltrated);
    bool isMsgTypeFiltrated(const cannabus::IdMsgTypes msgType) const;

    // Сеттер и геттер фильтра F-кодов
    void setFCodeFiltrated(const cannabus::IdFCode fCode, const bool isFiltrated);
    bool isFCodeFiltrated(const cannabus::IdFCode fCode) const;

    // Сеттер и геттер фильтра содержимого
    void setContentFiltrated(const QVector<uint8_t> regs, const QVector<uint8_t> data);
    bool isContentFiltrated(const cannabus::IdMsgTypes msgType, const cannabus::IdFCode fCode, QByteArray dataArray) const;

    // Проверка фильтрации пары регистр-данные
    bool isPairRegDataFiltrated(const uint8_t reg, const uint8_t data) const;

    // Заполнение векторов адресов, типов сообщений и F-кодов значениями isFiltrated
    void fillSlaveAddressSettings(const bool isFiltrated);
    void fillMsgTypesSettings(const bool isFiltrated);
    void fillFCodeSettings(const bool isFiltrated);

    // Проверка фильтрации сообщения
    bool mustDataFrameBeProcessed(const QCanBusFrame &frame);

    // Преобразование строки диапазонов в вектор и наоборот
    QVector<uint8_t> rangesStringToVector(const QString ranges, const int32_t base);
    QString rangesVectorToString(const QVector<uint8_t> ranges, const int32_t base);

public slots:
    // Установка нового фильтра адресов
    void setSlaveAddressFilter(QString addressesRange);

    // Установка и удаление фильтра содержимого
    void setContentFilter(QString regsRange, QString dataRange);
    void removeContentFilter(const int32_t index);

signals:
    // Сигнал окну лога для инкрементации количества принятых сообщений
    void frameIsProcessing();

    // Сигнал об успешном добавлении фильтра адресов
    void slaveAddressesFilterAdded(const QString addressesRange);

    // Сигнал об успешном добавлении фильтра содержимого
    void contentFilterAdded(const QString regsRange, const QString dataRange);

private:
    Settings m_settings;
};
