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

    void clearList();

signals:
    void addFilter(QString regsRange, QString dataRange);
    void removeFilterAtIndex(const int32_t index);

public slots:
    void setFilter(const QString regsRange, const QString dataRange);

private slots:
    void addFilterButtonPressed();
    void removeFilterButtonPressed();

private:
    void makeHeader();

    void addRow();
    void addButton(const QString buttonText);
    void setRegsRange(const QString regsRange);
    void setDataRange(const QString dataRange);

    int32_t m_currentRow;
};
