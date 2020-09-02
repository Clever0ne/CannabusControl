/****************************************************************************

Класс FilterList предоставляет пользовательский интерфейс для добавления и
удаления фильтров содержимого (регистров и данных).

****************************************************************************/

#pragma once

#include <QTableWidget>
#include <stdint.h>

enum class FilterListColumn {
    button,
    regs,
    data
};

class FilterList : public QTableWidget
{
    Q_OBJECT

public:
    explicit FilterList(QWidget *parent = nullptr);
    ~FilterList() = default;

    // Очистить список фильтров и соответствующий вектор в классе Filter
    void clearList();

public slots:
    // Добавление нового фильтра в список (визуальное)
    void setFilter(const QString regsRange, const QString dataRange);

private slots:
    // Нажата кнопка добавления/удаления фильтра
    void addFilterButtonPressed();
    void removeFilterButtonPressed();

signals:
    // Запрос на добавление фильтра содержимого
    void addFilter(QString regsRange, QString dataRange);

    // Запрос на удаление фильтра содержимого под индексом index
    void removeFilterAtIndex(const int32_t index);

private:
    // Создание заголовка списка фильтров и добавление первой строки
    void makeHeader();

    // Добавление строки
    void addRow();

    // Добавление кнопки с текстом buttonText
    void addButton(const QString buttonText);

    // Сеттеры для ячеек регистров и данных (устанавливают диапазоны в ячейки)
    void setRegsRange(const QString regsRange);
    void setDataRange(const QString dataRange);

    int32_t m_currentRow;
};
