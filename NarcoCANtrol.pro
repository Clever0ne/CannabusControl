QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialbus

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main/bitrate_box.cpp \
    src/main/log_window.cpp \
    src/main/main.cpp \
    src/main/main_window.cpp \
    src/main/settings_dialog.cpp

HEADERS += \
    src/cannabus_library/cannabus_common.h \
    src/main/bitrate.h \
    src/main/bitrate_box.h \
    src/main/log_window.h \
    src/main/main_window.h \
    src/main/settings_dialog.h

FORMS += \
    src/main/main_window.ui \
    src/main/settings_dialog.ui

TRANSLATIONS += \
    src/main/NarcoCANtrol_ru_RU.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    src/images/images.qrc
