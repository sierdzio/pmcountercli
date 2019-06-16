QT = core serialport

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS
# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

HEADERS += \
    pms7003reader.h

SOURCES += \
    main.cpp \
    pms7003reader.cpp

OTHER_FILES += \
    pms7003.py
