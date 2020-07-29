#include "bitrate_box.h"
#include "bitrate.h"

#include <stdint.h>
#include <QLineEdit>

BitRateBox::BitRateBox(QWidget *parent) :
    QComboBox(parent),
    m_customSpeedValidator(new QIntValidator(0, (uint32_t)BitRate::KBPS_1000, this))
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

void BitRateBox::setFlexibleFataRateEnabled(const bool isEnabled)
{
    m_isFlexibleDataRateEnabled = isEnabled;
    m_customSpeedValidator->setTop(isEnabled == true ? (uint32_t)BitRate::KBPS_8000 : (uint32_t)BitRate::KBPS_1000);

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
        (uint32_t)BitRate::KBPS_10,
        (uint32_t)BitRate::KBPS_20,
        (uint32_t)BitRate::KBPS_50,
        (uint32_t)BitRate::KBPS_100,
        (uint32_t)BitRate::KBPS_125,
        (uint32_t)BitRate::KBPS_250,
        (uint32_t)BitRate::KBPS_500,
        (uint32_t)BitRate::KBPS_800,
        (uint32_t)BitRate::KBPS_1000
    };

    static const QList<uint32_t> dataRates = {
        (uint32_t)BitRate::KBPS_2000,
        (uint32_t)BitRate::KBPS_5000,
        (uint32_t)BitRate::KBPS_8000
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
    setCurrentIndex(findData((uint32_t)BitRate::KBPS_125));
}
