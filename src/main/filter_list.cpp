#include "filter_list.h"

#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>

#define ADD_FILTER    tr("+")
#define REMOVE_FILTER tr("–")
#define DEFAULT_RANGE tr("00-FF")

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

    addLineEdit(FilterListColumn::regs, DEFAULT_RANGE);
    addLineEdit(FilterListColumn::data, DEFAULT_RANGE);
}

void FilterList::makeHeader()
{
    QString backgroundColor = "beige";
    QFont currentFont = font();
    QString fontFamily = currentFont.family();
    int32_t fontSize = 8;
    horizontalHeader()->setStyleSheet(tr("QHeaderView::section { background-color: %1; font-family: \"%2\"; font-size: %3pt }")
                                      .arg(backgroundColor)
                                      .arg(fontFamily)
                                      .arg(fontSize));

    QStringList contentFilterListHeader = {"", "Regs", "Data"};

    setColumnCount(contentFilterListHeader.count());
    setHorizontalHeaderLabels(contentFilterListHeader);

    setColumnWidth((uint32_t)FilterListColumn::button, fontMetrics().horizontalAdvance(" - ")    );
    setColumnWidth((uint32_t)FilterListColumn::regs  , fontMetrics().horizontalAdvance(" 00-FF "));
    setColumnWidth((uint32_t)FilterListColumn::data  , fontMetrics().horizontalAdvance(" 00-FF "));

    horizontalHeader()->setSectionsClickable(false);
    horizontalHeader()->setFixedHeight(1.5 * fontMetrics().height());

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

void FilterList::setFilter(QString regsRange, QString dataRange)
{
    // Удаляем текущую строку (с кнопкой добавления строки)
    // и добавлем строку для текущего фильтра
    removeRow(m_currentRow);
    setRowCount(m_currentRow + 1);

    // Если строка пустая, записываем в неё весь диапазон 00-FF
    auto range = [](QString range) {
        if (range.isEmpty() != false)
        {
            range = DEFAULT_RANGE;
        }
        return range;
    };

    // Добавляем кнопку для удаления фильтра,
    // а также диапазоны фильтруемых регистров и данных
    addButton(REMOVE_FILTER);
    setRegsRange(range(regsRange));
    setDataRange(range(dataRange));

    // Добавляем новую строку для добавления фильтра
    addRow();

    scrollToBottom();
}

void FilterList::addFilterButtonPressed()
{
    // Получаем диапазоны регистров и данных из QLineEdit'ов в ячейках таблицы
    // Да, в ячейках таблицы в строке добавления фильтра изначально QLineEdit'ы
    // Всё ради Placeholder text, иначе пользователь может не догадаться,
    // что же ему нужно писать в эти ячейки
    auto range = [this](FilterListColumn column) {
        auto lineEdit = qobject_cast<QLineEdit *>(cellWidget(currentRow(), (uint32_t)column));

        Q_ASSERT(lineEdit != nullptr);

        return lineEdit->text();
    };

    QString regsRange = range(FilterListColumn::regs);
    QString dataRange = range(FilterListColumn::data);

    emit addFilter(regsRange, dataRange);
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
