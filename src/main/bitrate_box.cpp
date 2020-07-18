#include "bitrate_box.h"

#include <stdint.h>
#include <QLineEdit>

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

BitRateBox::BitRateBox(QWidget *parent) :
    QComboBox(parent),
    customSpeedValidator(new QIntValidator(0, 1000000, this))
{
    fillBitRates();

    connect(this, QOverload<int32_t>::of(&QComboBox::currentIndexChanged),
            this, &BitRateBox::checkCustomSpeedPolicy);
}

BitRateBox::~BitRateBox()
{
    delete customSpeedValidator;
}

uint32_t BitRateBox::bitRate() const
{
    if (currentIndex() == (count() - 1))
    {
        return currentText().toUInt();
    }
    return itemData(currentIndex()).toUInt();
}

bool BitRateBox::isFlexibleDataRateEnabled() const
{
    return flexibleDataRate;
}

void BitRateBox::setFlexibleFataRateEnabled(bool isEnabled)
{
    flexibleDataRate = isEnabled;
    customSpeedValidator->setTop(isEnabled == true ? 10000000 : 1000000);

    fillBitRates();
}

void BitRateBox::checkCustomSpeedPolicy(int index)
{
    const bool isCustomSpeed = !(itemData(index).isValid());
    setEditable(isCustomSpeed);
    if (isCustomSpeed != false)
    {
        clearEditText();
        lineEdit()->setValidator(customSpeedValidator);
    }
}


void BitRateBox::fillBitRates()
{
    const QList<uint32_t> rates = {
        BitRate_10000_bps,
        BitRate_20000_bps,
        BitRate_50000_bps,
        BitRate_100000_bps,
        BitRate_125000_bps,
        BitRate_250000_bps,
        BitRate_500000_bps,
        BitRate_800000_bps,
        BitRate_1000000_bps
    };

    const QList<uint32_t> dataRates = {
        BitRate_2000000_bps,
        BitRate_5000000_bps,
        BitRate_8000000_bps
    };

    clear();

    for (uint32_t rate : rates)
    {
        addItem(QString::number(rate), rate);
    }

    if (isFlexibleDataRateEnabled() != false)
    {
        for (uint32_t rate : dataRates)
        {
            addItem(QString::number(rate), rate);
        }
    }
    addItem(tr("Custom"));
    // По-умолчанию битрейт равен 500 кбит/с
    setCurrentIndex(findData(BitRate_500000_bps));
}
