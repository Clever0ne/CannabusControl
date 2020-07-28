#include "log_window.h"
#include <QHeaderView>
#include <algorithm>

LogWindow::LogWindow(QWidget *parent) : QTableWidget(parent)
{    
    makeHeader();

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->hide();

    setDefaultMessageFilterSettings();
}

LogWindow::~LogWindow()
{
    ;
}

void LogWindow::makeHeader()
{
    QStringList logWindowHeader = {"No.", "Time", "Msg Type", "Address", "F-Code", "DLC", "Data", "Info"};

   setColumnCount(logWindowHeader.count());
   setHorizontalHeaderLabels(logWindowHeader);

   resizeColumnsToContents();
   setColumnWidth(COUNT, 50);
   setColumnWidth(TIME, 80);
   setColumnWidth(MSG_TYPE, 75);
   setColumnWidth(SLAVE_ADDRESS, 80);
   setColumnWidth(F_CODE, 65);
   setColumnWidth(DATA_SIZE, 40);
   setColumnWidth(DATA, 195);

   horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");
}

void LogWindow::clearLog()
{    
    m_numberFramesReceived = 0;

    clear();
    setRowCount(0);
    makeHeader();
}

void LogWindow::processDataFrame(const QCanBusFrame &frame)
{
    m_numberFramesReceived++;

    if (isDataFrameMustBeProcessed(frame) == false)
    {
        return;
    }

    m_currentRow = rowCount();
    insertRow(m_currentRow);

    const uint32_t frameId = frame.frameId();
    const uint32_t slaveAddress = cannabus::getAddressFromId(frameId);
    const cannabus::IdMsgTypes msgType = cannabus::getMsgTypeFromId(frameId);
    const cannabus::IdFCode fCode = cannabus::getFCodeFromId(frameId);

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
    m_numberFramesReceived++;

    m_currentRow = rowCount();
    insertRow(m_currentRow);

    setCount();
    setTime(frame.timeStamp().seconds(), frame.timeStamp().microSeconds());
    setMsgInfo(errorInfo);
}

void LogWindow::setCount()
{
    m_count = tr("%1").arg(m_numberFramesReceived, 5, 10, QLatin1Char(' '));

    QTableWidgetItem *item = new QTableWidgetItem(m_count);
    setItem(m_currentRow, COUNT, item);
}

void LogWindow::setTime(const uint64_t seconds, const uint64_t microseconds)
{
    m_time = tr("%1.%2")
            .arg(seconds, 4, 10, QLatin1Char(' '))
            .arg(microseconds / 100, 4, 10, QLatin1Char('0'));

    QTableWidgetItem *item = new QTableWidgetItem(m_time);
    setItem(m_currentRow, TIME, item);
}

void LogWindow::setMsgType(const cannabus::IdMsgTypes msgType)
{
    m_msgType = tr("  0b%1").arg((uint32_t)msgType, 2, 2, QLatin1Char('0'));

    QTableWidgetItem *item = new QTableWidgetItem(m_msgType);
    setItem(m_currentRow, MSG_TYPE, item);
}

void LogWindow::setSlaveAddress(const uint32_t slaveAddress)
{
    m_slaveAddress = tr("%1 (0x").arg(slaveAddress, 2, 10, QLatin1Char(' ')) +
            tr("%1)").arg(slaveAddress, 2, 16, QLatin1Char('0')).toUpper();

    QTableWidgetItem *item = new QTableWidgetItem(m_slaveAddress);
    setItem(m_currentRow, SLAVE_ADDRESS, item);
}

void LogWindow::setFCode(const cannabus::IdFCode fCode)
{
    m_fCode = tr(" 0b%1").arg((uint32_t)fCode, 3, 2, QLatin1Char('0'));

    QTableWidgetItem *item = new QTableWidgetItem(m_fCode);
    setItem(m_currentRow, F_CODE, item);
}

void LogWindow::setDataSize(const uint32_t dataSize)
{
    m_dataSize = tr(" [%1]").arg(dataSize);

    QTableWidgetItem *item = new QTableWidgetItem(m_dataSize);
    setItem(m_currentRow, DATA_SIZE, item);
}

void LogWindow::setData(const QByteArray data)
{
    m_data = data.toHex(' ').toUpper();

    QTableWidgetItem *item = new QTableWidgetItem(m_data);
    setItem(m_currentRow, DATA, item);
}

void LogWindow::setMsgInfo(const cannabus::IdMsgTypes msgType, const cannabus::IdFCode fCode, const uint32_t dataSize)
{
    if (dataSize == 0)
    {
        m_msgInfo = tr("[Slave's response] Incorrect request");
    }
    else
    {
        QString msgTypeInfo;
        QString fCodeInfo;

        switch (msgType)
        {
            case cannabus::IdMsgTypes::HIGH_PRIO_MASTER:
            {
                msgTypeInfo = tr("Master's high-prio");
                break;
            }
            case cannabus::IdMsgTypes::HIGH_PRIO_SLAVE:
            {
                msgTypeInfo = tr("Slave's high-prio");
                break;
            }
            case cannabus::IdMsgTypes::MASTER:
            {
                msgTypeInfo = tr("Master's request");
                break;
            }
            case cannabus::IdMsgTypes::SLAVE:
            {
                msgTypeInfo = tr("Slave's response");
                break;
            }
            default:
            {
                break;
            }
        }

        switch (fCode)
        {
            case cannabus::IdFCode::WRITE_REGS_RANGE:
            {
                fCodeInfo = tr("Writing regs range");
                break;
            }
            case cannabus::IdFCode::WRITE_REGS_SERIES:
            {
                fCodeInfo = tr("Writing regs series");
                break;
            }
            case cannabus::IdFCode::READ_REGS_RANGE:
            {
                fCodeInfo = tr("Reading regs range");
                break;
            }
            case cannabus::IdFCode::READ_REGS_SERIES:
            {
                fCodeInfo = tr("Reading regs series");
                break;
            }
            case cannabus::IdFCode::DEVICE_SPECIFIC1:
            {
                fCodeInfo = tr("Device-specific (1)");
                break;
            }
            case cannabus::IdFCode::DEVICE_SPECIFIC2:
            {
                fCodeInfo = tr("Device-specific (2)");
                break;
            }
            case cannabus::IdFCode::DEVICE_SPECIFIC3:
            {
                fCodeInfo = tr("Device-specific (3)");
                break;
            }
            case cannabus::IdFCode::DEVICE_SPECIFIC4:
            {
                fCodeInfo = tr("Device-specific (4)");
                break;
            }
            default:
            {
                break;
            }
        }

        m_msgInfo = tr("[%1] %2").arg(msgTypeInfo).arg(fCodeInfo);
    }

    QTableWidgetItem *item = new QTableWidgetItem(m_msgInfo);
    setItem(m_currentRow, MSG_INFO, item);
}

void LogWindow::setMsgInfo(const QString errorInfo)
{
    m_msgInfo = errorInfo;

    QTableWidgetItem *item = new QTableWidgetItem(m_msgInfo);
    setItem(m_currentRow, MSG_INFO, item);
}

void LogWindow::setDefaultMessageFilterSettings()
{
    std::fill(m_filter.fCodeSettings, m_filter.fCodeSettings
              + sizeof(m_filter.fCodeSettings) / sizeof(m_filter.fCodeSettings[0]), true);
    std::fill(m_filter.msgTypeSettings, m_filter.msgTypeSettings
              + sizeof(m_filter.msgTypeSettings) / sizeof(m_filter.msgTypeSettings[0]), true);
    std::fill(m_filter.slaveAddressSettings, m_filter.slaveAddressSettings
              + sizeof(m_filter.slaveAddressSettings) / sizeof(m_filter.slaveAddressSettings[0]), true);
}

bool LogWindow::isDataFrameMustBeProcessed(const QCanBusFrame &frame)
{
    bool isFiltrated = true;

    const uint32_t frameId = frame.frameId();

    const uint32_t slaveAddress = cannabus::getAddressFromId(frameId);
    isFiltrated &= isSlaveAddressFiltrated(slaveAddress);

    const cannabus::IdMsgTypes msgType = cannabus::getMsgTypeFromId(frameId);
    isFiltrated &= isMsgTypeFiltrated(msgType);

    const cannabus::IdFCode fCode = cannabus::getFCodeFromId(frameId);
    isFiltrated &= isFCodeFiltrated(fCode);

    return isFiltrated;
}

void LogWindow::setSlaveAddressFiltrated(const uint32_t slaveAddress, const bool isFiltrated)
{
    m_filter.msgTypeSettings[slaveAddress] = isFiltrated;
}

bool LogWindow::isSlaveAddressFiltrated(const uint32_t slaveAddress)
{
    return m_filter.msgTypeSettings[slaveAddress];
}

void LogWindow::setMsgTypeFiltrated(const cannabus::IdMsgTypes msgType, const bool isFiltrated)
{
    m_filter.msgTypeSettings[(uint32_t)msgType] = isFiltrated;
}

bool LogWindow::isMsgTypeFiltrated(const cannabus::IdMsgTypes msgType)
{
    return m_filter.msgTypeSettings[(uint32_t)msgType];
}

void LogWindow::setFCodeFiltrated(const cannabus::IdFCode fCode, const bool isFiltrated)
{
    m_filter.fCodeSettings[(uint32_t)fCode] = isFiltrated;
}

bool LogWindow::isFCodeFiltrated(const cannabus::IdFCode fCode)
{
    return m_filter.fCodeSettings[(uint32_t)fCode];
}
