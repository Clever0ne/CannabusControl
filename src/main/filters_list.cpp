#include "filters_list.h"

#include <QHeaderView>
#include <QPushButton>

FiltersList::FiltersList(QWidget *parent) : QTableWidget(parent)
{
    verticalHeader()->hide();
    makeHeader();

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void FiltersList::makeHeader()
{
    QStringList contentFilterListHeader = {"", "Regs", "Data"};

    setColumnCount(contentFilterListHeader.count());
    setHorizontalHeaderLabels(contentFilterListHeader);

    setColumnWidth((uint32_t)FiltersListColumn::button, fontMetrics().horizontalAdvance(" - "));
    setColumnWidth((uint32_t)FiltersListColumn::regs, fontMetrics().horizontalAdvance(" 00-FF "));
    setColumnWidth((uint32_t)FiltersListColumn::data, fontMetrics().horizontalAdvance(" 00-FF "));

    horizontalHeader()->setSectionsClickable(false);
    horizontalHeader()->setFixedHeight(25);

    horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");
}

void FiltersList::clearList()
{
    m_currentRow = 0;

    emit removeFilterAtIndex(-1);
    clear();
    setRowCount(0);
    makeHeader();
}

void FiltersList::addNewContentFilter(const QVector<uint32_t> regs, const QVector<uint32_t> data)
{
    m_currentRow = rowCount();
    setRowCount(m_currentRow + 1);

    QString regsRange = rangesToString(regs);
    QString dataRange = rangesToString(data);

    addButton();
    setRegsRange(regsRange);
    setDataRange(dataRange);
}

void FiltersList::addButton()
{
    auto button = new QPushButton();
    button->setStyleSheet("QPushButton { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");
    button->setText("—");
    button->setObjectName(tr("button_%1").arg(m_currentRow));
    setCellWidget(m_currentRow, (uint32_t)FiltersListColumn::button, button);
}

void FiltersList::setRegsRange(const QString regsRange)
{
    auto item = new QTableWidgetItem(regsRange);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)FiltersListColumn::regs, item);
}

void FiltersList::setDataRange(const QString dataRange)
{
    auto item = new QTableWidgetItem(dataRange);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)FiltersListColumn::data, item);
}

QString FiltersList::rangesToString(const QVector<uint32_t> ranges, const int32_t base)
{
    if (ranges.isEmpty() != false)
    {
        return "00-FF";
    }

    QString data;
    uint32_t left = ranges.first();
    uint32_t right = ranges.first();

    for (uint32_t currentData : ranges)
    {
        if (currentData == right)
        {
            if (currentData != ranges.last())
            {
                continue;
            }
        }

        if (currentData == right + 1)
        {
            right = currentData;
            if (currentData != ranges.last())
            {
                continue;
            }
        }

        if (data.isEmpty() == false)
        {
            data += ", ";
        }

        if (left == right)
        {
            data += tr("%1").arg(left, 2, base, QLatin1Char('0'));
        }
        else
        {
            data += tr("%1-%2")
                    .arg(left, 2, base, QLatin1Char('0')).toUpper()
                    .arg(right, 2, base, QLatin1Char('0')).toUpper();
        }

        left = currentData;
        right = currentData;
    }

    return data;
}
