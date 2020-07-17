#ifndef BITRATE_BOX_H
#define BITRATE_BOX_H

#include <QComboBox>

QT_BEGIN_NAMESPACE

class QIntValidator;

QT_END_NAMESPACE

class BitRateBox : public QComboBox
{

public:
    explicit BitRateBox(QWidget *parent = nullptr);
    ~BitRateBox();

    uint32_t bitRate() const;

    bool isFlexibleDataRateEnabled() const;
    void setFlexibleFataRateEnabled(bool isEnabled);

private slots:
    void checkCustomSpeedPolicy(int32_t index);

private:
    void fillBitRates();

    bool flexibleDataRate = false;
    QIntValidator *customSpeedValidator = nullptr;
};

#endif // BITRATE_BOX_H
