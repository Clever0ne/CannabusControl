#pragma once

#include <stdint.h>
#include <QString>
#include <QObject>

enum class BitRate {
    KBPS_10 = 10000,
    KBPS_20 = 20000,
    KBPS_50 = 50000,
    KBPS_100 = 100000,
    KBPS_125 = 125000,
    KBPS_250 = 250000,
    KBPS_500 = 500000,
    KBPS_800 = 800000,
    KBPS_1000 = 1000000,
    KBPS_2000 = 2000000,
    KBPS_5000 = 5000000,
    KBPS_8000 = 8000000,
};

inline QString bitRateToString(uint32_t bitRate)
{
    QString result = QObject::tr("%1 %2")
            .arg(bitRate / (bitRate < (uint32_t)BitRate::KBPS_1000 ? 1000 : 1000000))
            .arg(bitRate < (uint32_t)BitRate::KBPS_1000 ? "kbit/s" : "Mbit/s");
    return result;
}
