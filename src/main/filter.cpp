#include "filter.h"

using namespace cannabus;

Filter::Filter(QObject *parent) : QObject(parent)
{
    fillSlaveAddressSettings(true);
    fillMsgTypesSettings(true);
    fillFCodeSettings(true);
}

bool Filter::mustDataFrameBeProcessed(const QCanBusFrame &frame)
{
    emit frameInProcessing();

    bool isFiltrated = true;

    const uint32_t frameId = frame.frameId();

    const uint32_t slaveAddress = getAddressFromId(frameId);
    isFiltrated &= isSlaveAddressFiltrated(slaveAddress);

    const IdMsgTypes msgType = getMsgTypeFromId(frameId);
    isFiltrated &= isMsgTypeFiltrated(msgType);

    const IdFCode fCode = getFCodeFromId(frameId);
    isFiltrated &= isFCodeFiltrated(fCode);

    const QByteArray dataArray = frame.payload();
    isFiltrated &= isContentFiltrated(msgType, fCode, dataArray);

    return isFiltrated;
}

void Filter::removeContentFilter(int32_t index)
{
    if (index == -1)
    {
        m_settings.content.clear();
        return;
    }

    m_settings.content.remove(index);
}

void Filter::setSlaveAddressFiltrated(const uint32_t slaveAddress, const bool isFiltrated)
{
    if (slaveAddress > (uint32_t)IdAddresses::DIRECT_ACCESS)
    {
        return;
    }
    m_settings.slaveAddress.replace(slaveAddress, isFiltrated);
}

bool Filter::isSlaveAddressFiltrated(const uint32_t slaveAddress) const
{
    if (slaveAddress > (uint32_t)IdAddresses::DIRECT_ACCESS)
    {
        return false;
    }
    return m_settings.slaveAddress.value(slaveAddress);
}

void Filter::setMsgTypeFiltrated(const IdMsgTypes msgType, const bool isFiltrated)
{
    switch (msgType)
    {
        case IdMsgTypes::HIGH_PRIO_MASTER:
        case IdMsgTypes::HIGH_PRIO_SLAVE:
        case IdMsgTypes::MASTER:
        case IdMsgTypes::SLAVE:
        {
            m_settings.msgType.replace((uint32_t)msgType, isFiltrated);
        }
        default:
        {
            return;
        }
    }
}

bool Filter::isMsgTypeFiltrated(const IdMsgTypes msgType) const
{
    switch (msgType)
    {
        case IdMsgTypes::HIGH_PRIO_MASTER:
        case IdMsgTypes::HIGH_PRIO_SLAVE:
        case IdMsgTypes::MASTER:
        case IdMsgTypes::SLAVE:
        {
            return m_settings.msgType.value((uint32_t)msgType);
        }
        default:
        {
            return false;
        }
    }
}

void Filter::setFCodeFiltrated(const IdFCode fCode, const bool isFiltrated)
{
    switch (fCode)
    {
        case IdFCode::WRITE_REGS_RANGE:
        case IdFCode::WRITE_REGS_SERIES:
        case IdFCode::READ_REGS_RANGE:
        case IdFCode::READ_REGS_SERIES:
        case IdFCode::DEVICE_SPECIFIC1:
        case IdFCode::DEVICE_SPECIFIC2:
        case IdFCode::DEVICE_SPECIFIC3:
        case IdFCode::DEVICE_SPECIFIC4:
        {
            m_settings.fCode.replace((uint32_t)fCode, isFiltrated);
        }
        default:
        {
            return;
        }
    }
}

bool Filter::isFCodeFiltrated(const IdFCode fCode) const
{
    switch (fCode)
    {
        case IdFCode::WRITE_REGS_RANGE:
        case IdFCode::WRITE_REGS_SERIES:
        case IdFCode::READ_REGS_RANGE:
        case IdFCode::READ_REGS_SERIES:
        case IdFCode::DEVICE_SPECIFIC1:
        case IdFCode::DEVICE_SPECIFIC2:
        case IdFCode::DEVICE_SPECIFIC3:
        case IdFCode::DEVICE_SPECIFIC4:
        {
            return m_settings.fCode.value((uint32_t)fCode);
        }
        default:
        {
            return false;
        }
    }
}

void Filter::setContentFiltrated(const QVector<uint32_t> regs, const QVector<uint32_t> data)
{
    m_settings.content.append(qMakePair(regs, data));
}

bool Filter::isContentFiltrated(const IdMsgTypes msgType, const IdFCode fCode, const QByteArray dataArray) const
{
    if (m_settings.content.isEmpty() != false)
    {
        return true;
    }

    switch (fCode)
    {
        case IdFCode::WRITE_REGS_RANGE:
        {
            switch (msgType)
            {
                case IdMsgTypes::HIGH_PRIO_MASTER:
                case IdMsgTypes::MASTER:
                {
                    uint32_t left = static_cast<uint8_t>(dataArray[0]);
                    uint32_t right = static_cast<uint8_t>(dataArray[1]);

                    for (uint32_t reg = left; reg <= right; reg++)
                    {
                        uint32_t index = 2 + reg - left;
                        uint32_t data = static_cast<uint8_t>(dataArray[index]);

                        if (isPairRegDataFiltrated(reg, data) != false)
                        {
                            return true;
                        }
                    }

                    return false;
                }
                case IdMsgTypes::HIGH_PRIO_SLAVE:
                case IdMsgTypes::SLAVE:
                {
                    // В ответе ведомого не содержится информация о данных,
                    // т.к. ответ в формате 'Адрес начальный Адрес конечный'
                    return false;
                }
                default:
                {
                    return false;
                }
            }
        }
        case IdFCode::WRITE_REGS_SERIES:
        {
            switch (msgType)
            {
                case IdMsgTypes::HIGH_PRIO_MASTER:
                case IdMsgTypes::MASTER:
                {
                    for (int32_t index = 0; index < dataArray.size(); index++)
                    {
                        uint32_t reg = static_cast<uint8_t>(dataArray[index]);
                        uint32_t data = static_cast<uint8_t>(dataArray[++index]);

                        if (isPairRegDataFiltrated(reg, data) != false)
                        {
                            return true;
                        }
                    }

                    return false;
                }
                case IdMsgTypes::HIGH_PRIO_SLAVE:
                case IdMsgTypes::SLAVE:
                {
                    // В ответе ведомого не содержится информация о данных,
                    // т.к. ответ в формате 'Адрес 1 ... Адрес N'
                    return false;
                }
                default:
                {
                    return false;
                }
            }
        }
        case IdFCode::READ_REGS_RANGE:
        {
            switch (msgType)
            {
                case IdMsgTypes::HIGH_PRIO_MASTER:
                case IdMsgTypes::MASTER:
                {
                    // В запросе ведущего не содержится информация о данных,
                    // т.к. запрос в формате 'Адрес начальный Адрес конечный'
                    return false;
                }
                case IdMsgTypes::HIGH_PRIO_SLAVE:
                case IdMsgTypes::SLAVE:
                {
                    uint32_t left = static_cast<uint8_t>(dataArray[0]);
                    uint32_t right = static_cast<uint8_t>(dataArray[1]);

                    for (uint32_t reg = left; reg <= right; reg++)
                    {
                        uint32_t index = 2 + reg - left;
                        uint32_t data = static_cast<uint8_t>(dataArray[index]);
                        if(isPairRegDataFiltrated(reg, data) != false)
                        {
                            return true;
                        }
                    }

                    return false;
                }
                default:
                {
                    return false;
                }
            }
        }
        case IdFCode::READ_REGS_SERIES:
        {
            switch (msgType)
            {
                case IdMsgTypes::HIGH_PRIO_MASTER:
                case IdMsgTypes::MASTER:
                {
                    // В запросе ведущего не содержится информация о данных,
                    // т.к. ответ в формате 'Адрес 1 ... Адрес N'
                    return false;
                }
                case IdMsgTypes::HIGH_PRIO_SLAVE:
                case IdMsgTypes::SLAVE:
                {
                    for (int32_t index = 0; index < dataArray.size(); index++)
                    {
                        uint32_t reg = static_cast<uint8_t>(dataArray[index]);
                        uint32_t data = static_cast<uint8_t>(dataArray[++index]);
                        if (isPairRegDataFiltrated(reg, data) != false)
                        {
                            return true;
                        }
                    }

                    return false;
                }
                default:
                {
                    return false;
                }
            }
        }
        case IdFCode::DEVICE_SPECIFIC1:
        case IdFCode::DEVICE_SPECIFIC2:
        case IdFCode::DEVICE_SPECIFIC3:
        case IdFCode::DEVICE_SPECIFIC4:
        {
            // В запросе ведущего или в ответе ведомого не содержится информация о парах адрес-данные,
            // т.к. device-specific сообщения могут иметь произвольное содержание
            return true;
        }
        default:
        {
            return false;
        }
    }
}

bool Filter::isPairRegDataFiltrated(const uint32_t reg, const uint32_t data) const
{
    for (const Content content : qAsConst(m_settings.content))
    {
        if ((content.first.contains(reg) || content.first.isEmpty()) == false)
        {
            continue;
        }

        if ((content.second.contains(data) || content.second.isEmpty()) == false)
        {
            continue;
        }

        return true;
    }

    return false;
}

void Filter::fillSlaveAddressSettings(const bool isFiltrated)
{
    m_settings.slaveAddress.fill(isFiltrated, id_addresses_size);
}

void Filter::fillMsgTypesSettings(const bool isFiltrated)
{
    m_settings.msgType.fill(isFiltrated, id_msg_types_size);
}

void Filter::fillFCodeSettings(const bool isFiltrated)
{
    m_settings.fCode.fill(isFiltrated, id_f_code_size);
}
