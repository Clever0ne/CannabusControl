/****************************************************************************

Класс LogWindow обеспечивает наглядное и человекочитаемое представление
кадров данных, которые передаются по протоколу CannabusPlus и принимаются
USB-CAN-адаптером. В логе сообщений выводится информация о номере и времени
принятого кадра, адресе ведомого узла, типе сообщения и коде функции (F-коде),
а также о содержимом кадра (регистрах и данных).

****************************************************************************/

#pragma once

#include <QTableWidget>
#include <QCanBusFrame>
#include <stdint.h>
#include "../cannabus_library/cannabus_common.h"

enum class LogWindowColumn {
    count,
    time,
    msg_type,
    slave_address,
    f_code,
    data_size,
    data,
    msg_info
};

class LogWindow : public QTableWidget
{
public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow() = default;

    // Обработка кадров данных/кадров ошибок
    void processDataFrame(const QCanBusFrame &frame);
    void processErrorFrame(const QCanBusFrame &frame, const QString errorInfo);

public slots:
    // Очистка лог и сброс счётчик принятых кадров
    void clearLog();

    // Инкремент счётчик принятых кадров
    void numberFramesReceivedIncrement();

private:
    // Создание заголовка лога сообщений
    void makeHeader();

    // Сеттеры ячеек 'No', 'Time', 'Msg Type', 'Address',
    // 'F-code', 'DLC', 'Data' и 'Info' соответственно
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
};
