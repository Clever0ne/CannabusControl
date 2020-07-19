#include "bitrate_box.h"

#include <stdint.h>
#include <QLineEdit>

BitRateBox::BitRateBox(QWidget *parent) :
    QComboBox(parent),
    customSpeedValidator(new QIntValidator(0, BitRate_1000000_bps, this))
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
    return itemData(currentIndex()).toInt();
}

bool BitRateBox::isFlexibleDataRateEnabled() const
{
    return useFlexibleDataRate;
}

void BitRateBox::setFlexibleFataRateEnabled(bool isEnabled)
{
    useFlexibleDataRate = isEnabled;
    customSpeedValidator->setTop(isEnabled == true ? BitRate_8000000_bps : BitRate_1000000_bps);

    fillBitRates();
}

void BitRateBox::checkCustomSpeedPolicy(const int32_t index)
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
