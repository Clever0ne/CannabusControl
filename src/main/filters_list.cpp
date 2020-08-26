#include "filters_list.h"

#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>

#define ADD_FILTER    tr("+")
#define REMOVE_FILTER tr("–")

FiltersList::FiltersList(QWidget *parent) : QTableWidget(parent)
{
    verticalHeader()->hide();
    makeHeader();

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void FiltersList::addRow()
{
    m_currentRow = rowCount();
    setRowCount(m_currentRow + 1);

    addButton(ADD_FILTER);

    auto addLineEdit = [this](FiltersListColumn column, QString text) {
        auto lineEdit = new QLineEdit();
        lineEdit->setPlaceholderText(text);
        setCellWidget(m_currentRow, (uint32_t)column, lineEdit);
    };

    addLineEdit(FiltersListColumn::regs, "00-FF");
    addLineEdit(FiltersListColumn::data, "00-FF");
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

    addRow();
}

void FiltersList::clearList()
{
    m_currentRow = 0;

    emit removeFilterAtIndex(-1);
    clear();
    setRowCount(0);
    makeHeader();
}

void FiltersList::setNewFilter(const QVector<uint32_t> regs, const QVector<uint32_t> data)
{
    QString regsRange = rangesToString(regs);
    QString dataRange = rangesToString(data);

    removeRow(m_currentRow);
    setRowCount(m_currentRow + 1);

    addButton(REMOVE_FILTER);
    setRegsRange(regsRange);
    setDataRange(dataRange);

    addRow();
}

void FiltersList::addFilterButtonPressed()
{
    auto range = [this](FiltersListColumn column) {
        auto lineEdit = qobject_cast<QLineEdit *>(cellWidget(currentRow(), (uint32_t)column));

        Q_ASSERT(lineEdit != nullptr);

        return lineEdit->text();
    };

    QString regsRange = range(FiltersListColumn::regs);
    QString dataRange = range(FiltersListColumn::data);

    emit addNewFilter(regsRange, dataRange);
}

void FiltersList::removeFilterButtonPressed()
{
    int32_t row = currentRow();
    removeRow(row);
    m_currentRow--;

    emit removeFilterAtIndex(row);
}

void FiltersList::addButton(const QString buttonText)
{
    auto button = new QPushButton();
    button->setStyleSheet("QPushButton { background-color: beige; font-family: \"Courier New\"; font-size: 10pt }");
    button->setText(buttonText);

    if (buttonText == ADD_FILTER)
    {
        connect(button, &QPushButton::pressed, this, &FiltersList::addFilterButtonPressed);
    }
    else
    {
        connect(button, &QPushButton::pressed, this, &FiltersList::removeFilterButtonPressed);
    }

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
