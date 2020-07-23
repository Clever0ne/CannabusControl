#include "main_window.h"

#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("qt.canbus* = true"));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
