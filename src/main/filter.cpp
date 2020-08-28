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
    // Отправляем сигнал для инкрементации счётчика принятых кадров
    emit frameIsProcessing();

    bool isFiltrated = true;

    // Получаем ID кадра
    const uint32_t frameId = frame.frameId();

    // Получаем из ID адрес ведомого узла и определяем, фильтруется ли он
    const uint32_t slaveAddress = getAddressFromId(frameId);
    isFiltrated &= isSlaveAddressFiltrated(slaveAddress);

    // Получаем из ID тип сообщения и определяем, фильтруется ли он
    const IdMsgTypes msgType = getMsgTypeFromId(frameId);
    isFiltrated &= isMsgTypeFiltrated(msgType);

    // Получаем из ID F-код сообщения и определяем, фильтруется ли он
    const IdFCode fCode = getFCodeFromId(frameId);
    isFiltrated &= isFCodeFiltrated(fCode);

    // Получаем содержимое сообщения и определяем, фильтруется ли оно
    const QByteArray dataArray = frame.payload();
    isFiltrated &= isContentFiltrated(msgType, fCode, dataArray);

    return isFiltrated;
}

void Filter::removeContentFilter(const int32_t index)
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

void Filter::setContentFiltrated(const QVector<uint8_t> regs, const QVector<uint8_t> data)
{
    // Добавляем пару векторов регистров и данных
    m_settings.content.append(qMakePair(regs, data));
}

bool Filter::isContentFiltrated(const IdMsgTypes msgType, const IdFCode fCode, const QByteArray dataArray) const
{
    // Если вектор фильтра содержимого пуст, то фильтруются все регистры и данные
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
                    // Получаем из байтового массива левую и правую границы диапазона
                    uint32_t left = static_cast<uint8_t>(dataArray[0]);
                    uint32_t right = static_cast<uint8_t>(dataArray[1]);

                    // Для каждого регистра из диапазона получаем данные
                    // и осуществяем проверку пары регистр-данные
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
                    // Получаем из байтового массива пары регистр-данные
                    // и осуществляем проверку
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
                    // Получаем из байтового массива левую и правую границы диапазона
                    uint32_t left = static_cast<uint8_t>(dataArray[0]);
                    uint32_t right = static_cast<uint8_t>(dataArray[1]);

                    // Для каждого регистра из диапазона получаем данные
                    // и осуществяем проверку пары регистр-данные
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
                    // Получаем из байтового массива пары регистр-данные
                    // и осуществляем проверку
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
            // В запросе ведущего или в ответе ведомого не содержится информация о парах регистр-данные,
            // т.к. device-specific сообщения могут иметь произвольное содержание
            return true;
        }
        default:
        {
            return false;
        }
    }
}

bool Filter::isPairRegDataFiltrated(const uint8_t reg, const uint8_t data) const
{
    // Поочерёдно проверяем пары регистр-данные
    for (const Content content : qAsConst(m_settings.content))
    {
        // Если регистр фильтруется или вектор пуст (соответствует диапазону 00-FF), то идём дальше
        // Иначе идём на следующую итерацию цикла
        if ((content.first.contains(reg) || content.first.isEmpty()) == false)
        {
            continue;
        }

        // Если данные фильтруются или вектор пуст (соответствует диапазону 00-FF), то идём дальше
        // Иначе идём на следующую итерацию цикла
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

void Filter::setSlaveAddressFilter(QString addressesRange)
{
    // Убираем пробелы
    addressesRange.remove(QChar(' '));

    // Создаём вектор из адресов и на его основании обновляем строку
    // Да, здесь нужно base = 0 (см. соответствующую функцию)
    QVector<uint8_t> slaveAddresses = rangesStringToVector(addressesRange, 0);
    addressesRange = rangesVectorToString(slaveAddresses, 10);

    // Если поле пустое, сбрасываем настройки фильтрации адресов к состоянию по-умолчанию
    if (addressesRange.isEmpty() != false)
    {
        fillSlaveAddressSettings(true);

        emit slaveAddressesFilterAdded(addressesRange);
        return;
    }

    // Иначе заполняем фильтр адресами из вектора с фильтруемыми адресами
    // и обновляем поле фильтра
    fillSlaveAddressSettings(false);

    for (uint8_t slaveAddress : slaveAddresses)
    {
        setSlaveAddressFiltrated(slaveAddress, true);
    }

    emit slaveAddressesFilterAdded(addressesRange);
}

void Filter::setContentFilter(QString regsRange, QString dataRange)
{
    // Создаём вектор адресов и на его основании обновляем строку
    auto processRange = [this](QString &newRange) {
        newRange.remove("0x");
        auto range = rangesStringToVector(newRange, 16);
        newRange = rangesVectorToString(range, 16);
        return range;
    };

    QVector<uint8_t> regs = processRange(regsRange);
    QVector<uint8_t> data = processRange(dataRange);

    // Если оба поля пустые, то ничего не добавляем
    if ((regsRange.isEmpty() && dataRange.isEmpty()) != false)
    {
        return;
    }

    // Иначе добавляем регистры и данные в фильтр содержимого
    // и обновляем поля фильтра содержимого
    setContentFiltrated(regs, data);

    emit contentFilterAdded(regsRange, dataRange);
}

QVector<uint8_t> Filter::rangesStringToVector(const QString ranges, const int32_t base)
{
    // Разбиваем строку на список подстрок с разделителями в виде запятых
    QStringList range = ranges.split(',', Qt::SkipEmptyParts);
    QVector<uint8_t> data;

    // Разбираем получившиеся подстроки
    for (QString currentRange : range)
    {
        currentRange = range.takeFirst();

        // Проверяем, не является ли подстрока диапазоном
        if (currentRange.contains(QChar('-')) != false)
        {
            // Разбиваем диапазон на левую и правую границу
            QStringList leftAndRight = currentRange.split('-');

            // При base = 0 конвертируем строки в беззнаковые целые числа с учетом соглашений языка Си:
            // Строка начинается с '0x' — шестнадцатеричное число
            // Строка начинается с '0' — восьмеричное число
            // Всё остально — десятичное число
            bool isConverted = true;
            uint8_t left = leftAndRight.takeFirst().toUInt(&isConverted, base);
            uint8_t right = leftAndRight.takeLast().toUInt(&isConverted, base);

            // Если не получилось конвертировать — диапазон в помойку
            if (isConverted == false)
            {
                continue;
            }

            // Если левая граница больше правой, возможно, пользователь всё напутал
            // Любезно поменяем границы местами без его согласия
            if (left > right)
            {
                std::swap(left, right);
            }

            // Добавляем все адреса из диапазона в вектор с фильтруемыми адресами
            for (uint8_t currentData = left; currentData <= right; currentData++)
            {
                data.append(currentData);
            }
        }
        else
        {
            // При base = 0 конвертируем строку в беззнаковое целое число с учетом соглашений языка Си:
            // Строка начинается с '0x' — шестнадцатеричное число
            // Строка начинается с '0' — восьмеричное число
            // Всё остально — десятичное число
            bool isConverted = true;
            uint8_t currentData = currentRange.toUInt(&isConverted, base);

            // Если не получилось конвертировать — адрес в помойку
            if (isConverted == false)
            {
                continue;
            }

            // Добавляем адрес в вектор с фильтруемыми адресами
            data.append(currentData);
        }
    }

    // Сортируем вектор по возрастанию для дальнейшей обработки
    std::sort(data.begin(), data.end());

    return data;
}

QString Filter::rangesVectorToString(const QVector<uint8_t> ranges, const int32_t base)
{
    QString data;

    // Ширина поля числа и символ замещения
    int32_t fieldWidht = 0;
    QChar fillChar = QLatin1Char(' ');

    // Если числа шестнадцатеричные, будем выравнивать их до ширины в два символа, дополняя нулём
    // Иначе выравнивание не производится
    if (base == 16)
    {
        fieldWidht = 2;
        fillChar = QLatin1Char('0');
    }

    // Если вектор пуст, возвращаем пустую строку
    // Пустой вектор и пустая строка соответсвуют всему диапазону (адресов, регистров, данных)
    if (ranges.isEmpty() != false)
    {
        return data;
    }

    // Условные границы диапазонов, содержащихся в векторе
    // Необходимы в том случае, когда в векторе содержится несколько диапазонов
    // Например: 1-3, 5, 7-9, ...
    uint8_t left = ranges.first();
    uint8_t right = ranges.first();

    // Проходим по всему вектору
    for (uint8_t currentData : ranges)
    {
        // Если текущий элемент вектора равен правой границе и это не конец вектора,
        // идём к следующей итерации (то есть это не конец диапазона, в векторе возможны повторы)
        if (currentData == right && currentData != ranges.last())
        {
            continue;
        }

        // Если текущий элемент вектора больше предыдущей правой границы на единицу,
        // то это новая граница диапазона, необходимо её обновить
        if (currentData == right + 1)
        {
            right = currentData;

            // Если это не конец вектора, идём к следующей итерации
            if (currentData != ranges.last())
            {
                continue;
            }
        }

        // Если в строке уже есть диапазоны, разделяем их запятой
        if (data.isEmpty() == false)
        {
            data += ", ";
        }

        // Если левая граница равна правой, то это не диапазон, а единичное число
        // Иначе это всё же диапазон
        // В любом случае добавляем к строке диапазонов новое содержимое
        if (left == right)
        {
            data += tr("%1")
                    .arg(left, fieldWidht, base, fillChar);
        }
        else
        {
            data += tr("%1-%2")
                    .arg(left, fieldWidht, base, fillChar)
                    .arg(right, fieldWidht, base, fillChar);
        }

        // Обновляем левую и правую границы
        // Попадание сюда гарантирует, что текущий элемент более чем на единицу
        // превосходит предыдущую правую границу диапазона, а значит, что
        // этот элемент не входит ни в один из ранее записанных диапазонов
        left = currentData;
        right = currentData;
    }

    return data;
}
