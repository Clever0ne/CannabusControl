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
    QStringList logWindowHeader = {"No.", "Time", "Message Type", "Slave Address", "F-Code", "DLC", "Data", "Info"};

   setColumnCount(logWindowHeader.count());
   setHorizontalHeaderLabels(logWindowHeader);

   resizeColumnsToContents();
   setColumnWidth(logWindowHeader.indexOf("No."), 60);
   setColumnWidth(logWindowHeader.indexOf("Time"), 90);
   setColumnWidth(logWindowHeader.indexOf("Message Type"), 120);
   setColumnWidth(logWindowHeader.indexOf("Slave Address"), 120);
   setColumnWidth(logWindowHeader.indexOf("F-Code"), 120);
   setColumnWidth(logWindowHeader.indexOf("DLC"), 60);
   setColumnWidth(logWindowHeader.indexOf("Data"), 180);
}

void LogWindow::clearLog()
{
    clear();
    setRowCount(1);
    makeHeader();
}
