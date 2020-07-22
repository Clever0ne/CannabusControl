#pragma once

#include <QTableWidget>

class LogWindow : public QTableWidget
{
public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow();

public slots:
    void makeHeader();
    void clearLog();
};
