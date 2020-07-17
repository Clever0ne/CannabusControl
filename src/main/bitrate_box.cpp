#include "bitrate_box.h"

#include <stdint.h>
#include <QLineEdit>

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
        10000, 20000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000
    };

    const QList<uint32_t> dataRates = {
        2000000, 4000000, 8000000
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
    setCurrentIndex(findData(500000));
}
