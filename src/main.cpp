#include "cannabus_control.h"

#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("qt.canbus* = true"));

    QApplication a(argc, argv);
    CannabusControl w;
    w.show();
    return a.exec();
}
