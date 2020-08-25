#pragma once

#include <QTableWidget>
#include <stdint.h>

enum class FiltersListColumn {
    button,
    regs,
    data
};

class FiltersList : public QTableWidget
{
    Q_OBJECT

public:
    explicit FiltersList(QWidget *parent = nullptr);
    ~FiltersList() = default;

    void clearList();
    void addNewContentFilter(const QVector<uint32_t> regs, const QVector<uint32_t> data);

signals:
    void removeFilterAtIndex(int32_t index);

private:
    void makeHeader();

    QString rangesToString(const QVector<uint32_t> ranges, const int32_t base = 16);

    void addButton();
    void setRegsRange(const QString regsRange);
    void setDataRange(const QString dataRange);

    uint32_t m_currentRow;
};
