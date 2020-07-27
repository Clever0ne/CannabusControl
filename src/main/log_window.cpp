#include "log_window.h"
#include <QHeaderView>

LogWindow::LogWindow(QWidget *parent) : QTableWidget(parent)
{    
     makeHeader();

     horizontalHeader()->setStretchLastSection(true);
     horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
     verticalHeader()->hide();
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
   setColumnWidth(logWindowHeader.indexOf("No."), 50);
   setColumnWidth(logWindowHeader.indexOf("Time"), 80);
   setColumnWidth(logWindowHeader.indexOf("Msg Type"), 75);
   setColumnWidth(logWindowHeader.indexOf("Address"), 80);
   setColumnWidth(logWindowHeader.indexOf("F-Code"), 65);
   setColumnWidth(logWindowHeader.indexOf("DLC"), 40);
   setColumnWidth(logWindowHeader.indexOf("Data"), 195);

   horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");
}

void LogWindow::clearLog()
{
    clear();
    setRowCount(0);
    makeHeader();
}

void LogWindow::addReceivedMessage(const QCanBusFrame &frame)
{
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
    setMsgInfo(msgType, fCode);

    QStringList frameInfo = {
        m_count,
        m_time,
        m_msgType,
        m_slaveAddress,
        m_fCode,
        m_dataSize,
        m_data,
        m_msgInfo
    };

    uint32_t row = rowCount();
    insertRow(row);

    for (uint8_t column = 0; column < columnCount(); column++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(frameInfo.takeFirst());
        setItem(row, column, item);
    }

    scrollToBottom();
}

void LogWindow::setCount()
{
    m_numberFramesReceived++;

    m_count = tr("%1").arg(m_numberFramesReceived, 5, 10, QLatin1Char(' '));
}

void LogWindow::setTime(const uint64_t seconds, const uint64_t microseconds)
{
    m_time = tr("%1.%2")
            .arg(seconds, 4, 10, QLatin1Char(' '))
            .arg(microseconds, 4, 10, QLatin1Char('0'));
}

void LogWindow::setMsgType(const cannabus::IdMsgTypes msgType)
{
    m_msgType = tr("  0b%1").arg((uint32_t)msgType, 2, 2, QLatin1Char('0'));
}

void LogWindow::setSlaveAddress(const uint32_t slaveAddress)
{
    m_slaveAddress = tr("%2 (0x%1)")
            .arg(slaveAddress, 2, 16, QLatin1Char('0')).toUpper()
            .arg(slaveAddress, 2, 10, QLatin1Char(' '));
}

void LogWindow::setFCode(const cannabus::IdFCode fCode)
{
    m_fCode = tr(" 0b%1").arg((uint32_t)fCode, 3, 2, QLatin1Char('0'));
}

void LogWindow::setDataSize(const uint32_t dataSize)
{
    m_dataSize = tr(" [%1]").arg(dataSize);

    if (dataSize == 0)
    {
        m_msgInfo = tr("[Slave's response] Incorrect request");
    }
}

void LogWindow::setData(const QByteArray data)
{
    m_data = data.toHex(' ').toUpper();
}

void LogWindow::setMsgInfo(const cannabus::IdMsgTypes msgType, const cannabus::IdFCode fCode)
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
