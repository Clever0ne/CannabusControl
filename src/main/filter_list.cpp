#include "filter_list.h"

#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>

#define ADD_FILTER    tr("+")
#define REMOVE_FILTER tr("–")

FilterList::FilterList(QWidget *parent) : QTableWidget(parent)
{
    verticalHeader()->hide();
    makeHeader();

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void FilterList::addRow()
{
    m_currentRow = rowCount();
    setRowCount(m_currentRow + 1);

    addButton(ADD_FILTER);

    auto addLineEdit = [this](FilterListColumn column, QString text) {
        auto lineEdit = new QLineEdit();
        lineEdit->setPlaceholderText(text);
        setCellWidget(m_currentRow, (uint32_t)column, lineEdit);
    };

    addLineEdit(FilterListColumn::regs, "00-FF");
    addLineEdit(FilterListColumn::data, "00-FF");
}

void FilterList::makeHeader()
{
    QStringList contentFilterListHeader = {"", "Regs", "Data"};

    setColumnCount(contentFilterListHeader.count());
    setHorizontalHeaderLabels(contentFilterListHeader);

    setColumnWidth((uint32_t)FilterListColumn::button, fontMetrics().horizontalAdvance(" - "));
    setColumnWidth((uint32_t)FilterListColumn::regs, fontMetrics().horizontalAdvance(" 00-FF "));
    setColumnWidth((uint32_t)FilterListColumn::data, fontMetrics().horizontalAdvance(" 00-FF "));

    horizontalHeader()->setSectionsClickable(false);
    horizontalHeader()->setFixedHeight(25);

    horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");

    addRow();
}

void FilterList::clearList()
{
    m_currentRow = 0;

    emit removeFilterAtIndex(-1);
    clear();
    setRowCount(0);
    makeHeader();
}

void FilterList::setNewFilter(const QVector<uint32_t> regs, const QVector<uint32_t> data)
{
    QString regsRange = rangesToString(regs);
    QString dataRange = rangesToString(data);

    removeRow(m_currentRow);
    setRowCount(m_currentRow + 1);

    addButton(REMOVE_FILTER);
    setRegsRange(regsRange);
    setDataRange(dataRange);

    addRow();

    scrollToBottom();
}

void FilterList::addFilterButtonPressed()
{
    auto range = [this](FilterListColumn column) {
        auto lineEdit = qobject_cast<QLineEdit *>(cellWidget(currentRow(), (uint32_t)column));

        Q_ASSERT(lineEdit != nullptr);

        return lineEdit->text();
    };

    QString regsRange = range(FilterListColumn::regs);
    QString dataRange = range(FilterListColumn::data);

    emit addNewFilter(regsRange, dataRange);
}

void FilterList::removeFilterButtonPressed()
{
    int32_t row = currentRow();
    removeRow(row);
    m_currentRow--;

    emit removeFilterAtIndex(row);
}

void FilterList::addButton(const QString buttonText)
{
    auto button = new QPushButton();
    button->setStyleSheet("QPushButton { background-color: beige; font-family: \"Courier New\"; font-size: 10pt }");
    button->setText(buttonText);

    if (buttonText == ADD_FILTER)
    {
        connect(button, &QPushButton::pressed, this, &FilterList::addFilterButtonPressed);
    }
    else
    {
        connect(button, &QPushButton::pressed, this, &FilterList::removeFilterButtonPressed);
    }

    setCellWidget(m_currentRow, (uint32_t)FilterListColumn::button, button);
}

void FilterList::setRegsRange(const QString regsRange)
{
    auto item = new QTableWidgetItem(regsRange);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)FilterListColumn::regs, item);
}

void FilterList::setDataRange(const QString dataRange)
{
    auto item = new QTableWidgetItem(dataRange);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    setItem(m_currentRow, (uint32_t)FilterListColumn::data, item);
}

QString FilterList::rangesToString(const QVector<uint32_t> ranges, const int32_t base)
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
