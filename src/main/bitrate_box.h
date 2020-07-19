#ifndef BITRATE_BOX_H
#define BITRATE_BOX_H

#include <QComboBox>

QT_BEGIN_NAMESPACE

class QIntValidator;

QT_END_NAMESPACE

enum BitRate {
    BitRate_10000_bps = 10000,
    BitRate_20000_bps = 20000,
    BitRate_50000_bps = 50000,
    BitRate_100000_bps = 100000,
    BitRate_125000_bps = 125000,
    BitRate_250000_bps = 250000,
    BitRate_500000_bps = 500000,
    BitRate_800000_bps = 800000,
    BitRate_1000000_bps = 1000000,
    BitRate_2000000_bps = 2000000,
    BitRate_5000000_bps = 5000000,
    BitRate_8000000_bps = 8000000,
};

class BitRateBox : public QComboBox
{
public:
    explicit BitRateBox(QWidget *parent = nullptr);
    ~BitRateBox();

    uint32_t bitRate() const;

    bool isFlexibleDataRateEnabled() const;
    void setFlexibleFataRateEnabled(bool isEnabled);

private slots:
    void checkCustomSpeedPolicy(const int32_t index);

private:
    void fillBitRates();

    bool useFlexibleDataRate = false;
    QIntValidator *customSpeedValidator = nullptr;
};

#endif // BITRATE_BOX_H
