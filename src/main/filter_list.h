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
    void setNewFilter(const QVector<uint32_t> regs, const QVector<uint32_t> data);

signals:
    void addNewFilter(QString regsRange, QString dataRange);
    void removeFilterAtIndex(int32_t index);

private slots:
    void addFilterButtonPressed();
    void removeFilterButtonPressed();

private:
    void makeHeader();

    QString rangesToString(const QVector<uint32_t> ranges, const int32_t base = 16);

    void addRow();
    void addButton(const QString buttonText);
    void setRegsRange(const QString regsRange);
    void setDataRange(const QString dataRange);

    int32_t m_currentRow;
};
