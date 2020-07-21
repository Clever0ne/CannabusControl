#include "bitrate_box.h"
#include "bitrate.h"

#include <stdint.h>
#include <QLineEdit>

BitRateBox::BitRateBox(QWidget *parent) :
    QComboBox(parent),
    m_customSpeedValidator(new QIntValidator(0, BITRATE_1000000_BPS, this))
{
    fillBitRates();

    connect(this, QOverload<int32_t>::of(&QComboBox::currentIndexChanged),
            this, &BitRateBox::checkCustomSpeedPolicy);
}

BitRateBox::~BitRateBox()
{
    delete m_customSpeedValidator;
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
    return m_isFlexibleDataRateEnabled;
}

void BitRateBox::setFlexibleFataRateEnabled(bool isEnabled)
{
    m_isFlexibleDataRateEnabled = isEnabled;
    m_customSpeedValidator->setTop(isEnabled == true ? BITRATE_8000000_BPS : BITRATE_1000000_BPS);

    fillBitRates();
}

void BitRateBox::checkCustomSpeedPolicy(const int32_t index)
{
    const bool isCustomSpeed = !(itemData(index).isValid());
    setEditable(isCustomSpeed);
    if (isCustomSpeed != false)
    {
        clearEditText();
        lineEdit()->setValidator(m_customSpeedValidator);
    }
}

void BitRateBox::fillBitRates()
{
    static const QList<uint32_t> rates = {
        BITRATE_10000_BPS,
        BITRATE_20000_BPS,
        BITRATE_50000_BPS,
        BITRATE_100000_BPS,
        BITRATE_125000_BPS,
        BITRATE_250000_BPS,
        BITRATE_500000_BPS,
        BITRATE_800000_BPS,
        BITRATE_1000000_BPS
    };

    static const QList<uint32_t> dataRates = {
        BITRATE_2000000_BPS,
        BITRATE_5000000_BPS,
        BITRATE_8000000_BPS
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
    // По-умолчанию битрейт равен 125 кбит/с
    setCurrentIndex(findData(BITRATE_125000_BPS));
}
