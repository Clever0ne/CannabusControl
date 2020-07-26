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
    QStringList logWindowHeader = {"No.", "Time", "Msg Type", "Address", "F-Code", "DLC", "Data", "Info"};

   setColumnCount(logWindowHeader.count());
   setHorizontalHeaderLabels(logWindowHeader);

   resizeColumnsToContents();
   setColumnWidth(logWindowHeader.indexOf("No."), 50);
   setColumnWidth(logWindowHeader.indexOf("Time"), 80);
   setColumnWidth(logWindowHeader.indexOf("Msg Type"), 75);
   setColumnWidth(logWindowHeader.indexOf("Address"), 80);
   setColumnWidth(logWindowHeader.indexOf("F-Code"), 65);
   setColumnWidth(logWindowHeader.indexOf("DLC"), 40);
   setColumnWidth(logWindowHeader.indexOf("Data"), 195);

   horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: beige; font-family: \"Courier New\"; font-size: 8pt }");
}

void LogWindow::clearLog()
{
    clear();
    setRowCount(0);
    makeHeader();
}
