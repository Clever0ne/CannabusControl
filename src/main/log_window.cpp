#include "log_window.h"

#include <QHeaderView>
#include <algorithm>
#include <QFont>
#include <QFontDatabase>

using namespace cannabus;

namespace cannabus
{
    uint32_t qHash(const IdMsgTypes &msgType, uint32_t seed = 0)
    {
        return ::qHash((uint32_t)msgType, seed);
    }

    uint32_t qHash(const IdFCode &fCode, uint32_t seed = 0)
    {
        return ::qHash((uint32_t)fCode, seed);
    }
}

LogWindow::LogWindow(QWidget *parent) : QTableWidget(parent)
{
    makeHeader();
    verticalHeader()->hide();

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void LogWindow::makeHeader()
{
    QString backgroundColor = "beige";
    QFont currentFont = font();
    QString fontFamily = currentFont.family();
    int32_t fontSize = 8;
    horizontalHeader()->setStyleSheet(tr("QHeaderView::section { background-color: %1; font-family: \"%2\"; font-size: %3pt }")
                                      .arg(backgroundColor)
                                      .arg(fontFamily)
                                      .arg(fontSize));

    QStringList logWindowHeader = {"No.", "Time", "Msg Type", "Address", "F-Code", "DLC", "Data", "Info"};

    setColumnCount(logWindowHeader.count());
    setHorizontalHeaderLabels(logWindowHeader);

    auto resizeColumn = [this](LogWindowColumn column, QString text)
    {
        setColumnWidth((uint32_t)column, fontMetrics().horizontalAdvance(text));
    };

    resizeColumn(LogWindowColumn::count        , "123456 "                 );
    resizeColumn(LogWindowColumn::time         , "1234.1234 "              );
    resizeColumn(LogWindowColumn::msg_type     , " Msg Type "              );
    resizeColumn(LogWindowColumn::slave_address, "10 (0x0A) "              );
    resizeColumn(LogWindowColumn::f_code       , "F-Code "                 );
    resizeColumn(LogWindowColumn::data_size    , " [8] "                   );
    resizeColumn(LogWindowColumn::data         , "11 22 33 44 55 66 77 88 ");

    horizontalHeader()->setSectionsClickable(false);
    horizontalHeader()->setFixedHeight(1.5 * fontMetrics().height());
}

void LogWindow::clearLog()
{    
    m_numberFramesReceived = 0;
    m_currentRow = 0;

    clear();
    setRowCount(0);
    makeHeader();
}

void LogWindow::numberFramesReceivedIncrement()
{
    m_numberFramesReceived++;
}

void LogWindow::processDataFrame(const QCanBusFrame &frame)
{
    m_currentRow = rowCount();
    insertRow(m_currentRow);

    const uint32_t frameId = frame.frameId();
    const uint32_t slaveAddress = getAddressFromId(frameId);
    const IdMsgTypes msgType = getMsgTypeFromId(frameId);
    const IdFCode fCode = getFCodeFromId(frameId);

    setCount();
    setTime(frame.timeStamp().seconds(), frame.timeStamp().microSeconds());
    setMsgType(msgType);
    setSlaveAddress(slaveAddress);
    setFCode(fCode);
    setDataSize(frame.payload().size());
    setData(frame.payload());
    setMsgInfo(msgType, fCode, frame.payload().size());

    scrollToBottom();
}

void LogWindow::processErrorFrame(const QCanBusFrame &frame, const QString errorInfo)
{
    numberFramesReceivedIncrement();

    m_currentRow = rowCount();
    insertRow(m_currentRow);

    setCount();
    setTime(frame.timeStamp().seconds(), frame.timeStamp().microSeconds());
    setMsgInfo(errorInfo);
}

void LogWindow::setCount()
{
    // Выводим номер принятого кадра с шириной поля в 6 символов
    // в формате '123456'
    m_count = tr("%1").arg(m_numberFramesReceived, 6, 10, QLatin1Char(' '));

    auto item = new QTableWidgetItem(m_count);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::count, item);
}

void LogWindow::setTime(const uint64_t seconds, const uint64_t microseconds)
{
    // Выводим время в секундах с шириной поля в 9 символов
    // в формате '1234.1234'
    m_time = tr("%1.%2")
            .arg(seconds, 4, 10, QLatin1Char(' '))
            .arg(microseconds / 100, 4, 10, QLatin1Char('0'));

    auto item = new QTableWidgetItem(m_time);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::time, item);
}

void LogWindow::setMsgType(const IdMsgTypes msgType)
{
    // Выводим тип сообщения в двоичной системе счисления с шириной поля в 4 символа
    // в формате '0b10'
    m_msgType = tr("0b%1").arg((uint32_t)msgType, 2, 2, QLatin1Char('0'));

    auto item = new QTableWidgetItem(m_msgType);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::msg_type, item);
}

void LogWindow::setSlaveAddress(const uint32_t slaveAddress)
{
    // Выводим адрес ведомого устройства в десятичной
    // и шестнадцатеричной системах счисления с шириной поля в 9 символов
    // в формате '10 (0x0A)'
    m_slaveAddress = tr("%1 (0x").arg(slaveAddress, 2, 10, QLatin1Char(' ')) +
            tr("%1)").arg(slaveAddress, 2, 16, QLatin1Char('0')).toUpper();

    auto item = new QTableWidgetItem(m_slaveAddress);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::slave_address, item);
}

void LogWindow::setFCode(const IdFCode fCode)
{
    // Выводим F-код в двоичной системе счисления с шириной поля в 5 символов
    // в формате '0b101'
    m_fCode = tr("0b%1").arg((uint32_t)fCode, 3, 2, QLatin1Char('0'));

    auto item = new QTableWidgetItem(m_fCode);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::f_code, item);
}

void LogWindow::setDataSize(const uint32_t dataSize)
{
    // Выводим размер поля данных в байтах с шириной поля в 3 символа
    // в формате '[8]'
    m_dataSize = tr("[%1]").arg(dataSize);

    auto item = new QTableWidgetItem(m_dataSize);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::data_size, item);
}

void LogWindow::setData(const QByteArray data)
{
    // Выводим данные в шестнадцатеричном системе счисления без префиксов
    // с шириной поля данных до 25 символов (сколько получится)
    // в формате '11 22 33 44 55 66 77 88'
    m_data = data.toHex(' ').toUpper();

    auto item = new QTableWidgetItem(m_data);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::data, item);
}

void LogWindow::setMsgInfo(const IdMsgTypes msgType, const IdFCode fCode, const uint32_t dataSize)
{
    // Готовим словесное описание для типа сообщения и его F-кода,
    // а затем выводим информацию о кадре в текстовом формате
    // с шириной поля до 40 символов (сколько получится)
    // в формате '[MSG_TYPE_INFO] F_CODE_INFO'
    using namespace std;

    if (dataSize == 0)
    {
        m_msgInfo = tr("[Slave's response] Incorrect request");
    }
    else
    {
        static const QHash<IdMsgTypes, QString> msgTypeInfo = {
            make_pair(IdMsgTypes::HIGH_PRIO_MASTER, "Master's high-prio"),
            make_pair(IdMsgTypes::HIGH_PRIO_SLAVE , "Slave's high-prio" ),
            make_pair(IdMsgTypes::MASTER          , "Master's request"  ),
            make_pair(IdMsgTypes::SLAVE           , "Slave's response"  )
        };

        static const QHash<IdFCode, QString> fCodeInfo = {
            make_pair(IdFCode::WRITE_REGS_RANGE , "Writing regs range" ),
            make_pair(IdFCode::WRITE_REGS_SERIES, "Writing regs series"),
            make_pair(IdFCode::READ_REGS_RANGE  , "Reading regs range" ),
            make_pair(IdFCode::READ_REGS_SERIES , "Reading regs series"),
            make_pair(IdFCode::DEVICE_SPECIFIC1 , "Device-specific (1)"),
            make_pair(IdFCode::DEVICE_SPECIFIC2 , "Device-specific (2)"),
            make_pair(IdFCode::DEVICE_SPECIFIC3 , "Device-specific (3)"),
            make_pair(IdFCode::DEVICE_SPECIFIC4 , "Device-specific (4)")
        };

        auto compare = [](const auto &first, const auto &second)
        {
            return first.size() < second.size();
        };

        auto getMaxLength = [compare](const auto &info)
        {
            return max_element(begin(info), end(info), compare).value().size();
        };

        static const int32_t msgTypeInfoMaxLength = getMaxLength(msgTypeInfo);
        static const int32_t fCodeInfoMaxLength   = getMaxLength(fCodeInfo  );

        m_msgInfo = tr("%1 %2")
                .arg(msgTypeInfo.value(msgType), -msgTypeInfoMaxLength)
                .arg(fCodeInfo.value(fCode)    , -fCodeInfoMaxLength  );

        auto addBrackets = [](QString &info, QString msgTypeInfo)
        {
            info.insert(msgTypeInfo.size(), "]");
            info.insert(0                 , "[");
        };

        addBrackets(m_msgInfo, msgTypeInfo.value(msgType));
    }

    auto item = new QTableWidgetItem(m_msgInfo);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::msg_info, item);
}

void LogWindow::setMsgInfo(const QString errorInfo)
{
    // Выводим информацию о кадре ошибки в текстовом формате
    // с шириной поля до ? символов (сколько получится)
    // в формате 'ERROR_FRAME_INFO'
    m_msgInfo = errorInfo;

    auto item = new QTableWidgetItem(m_msgInfo);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)LogWindowColumn::msg_info, item);
}
