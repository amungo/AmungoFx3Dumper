TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

unix: DEFINES += NO_CY_API


INCLUDEPATH += ../lib

SOURCES += main.cpp \
    processors/streamdumper.cpp \
    sys/NamedClass.cpp \
    sys/Runnable.cpp \
    pointdrawer.cpp \
    ../lib/FileSimDev.cpp \
    ../lib/fx3devcyapi.cpp \
    ../lib/fx3deverr.cpp \
    ../lib/fx3devifce.cpp \
    ../lib/fx3fwparser.cpp \
    ../lib/fx3tuner.cpp \
    ../lib/FX3Dev.cpp \
    ../lib/Fx3Factory.cpp

HEADERS += \
    processors/streamdumper.h \
    sys/AsyncQueueHandler.h \
    sys/BlockQueue.h \
    sys/MessageReceiverIfce.h \
    sys/NamedClass.h \
    sys/Runnable.h \
    sys/SingleEvent.h \
    pointdrawer.h \
    ../lib/devioifce.h \
    ../lib/FileSimDev.h \
    ../lib/fx3commands.h \
    ../lib/fx3config.h \
    ../lib/fx3devcyapi.h \
    ../lib/fx3devdebuginfo.h \
    ../lib/fx3devdrvtype.h \
    ../lib/fx3deverr.h \
    ../lib/fx3devifce.h \
    ../lib/fx3fwparser.h \
    ../lib/fx3tuner.h \
    ../lib/FX3Dev.h \
    ../lib/libusb.h \
    ../lib/cy_inc/CyAPI.h \
    ../lib/cy_inc/cyioctl.h \
    ../lib/cy_inc/CyUSB30_def.h \
    ../lib/cy_inc/usb100.h \
    ../lib/cy_inc/usb200.h \
    ../lib/cy_inc/UsbdStatus.h \
    ../lib/cy_inc/VersionNo.h \
    ../lib/IFx3Device.h \
    ../lib/Fx3Factory.h

win32: SOURCES +=

win32: HEADERS += \






win32: LIBS += -luser32
win32: LIBS += -lsetupapi
win32: QMAKE_LFLAGS += /NODEFAULTLIB:LIBCMT

win32:!win32-g++: LIBS += -L$$PWD/libs/libusb/MS64/dll/ -llibusb-1.0
else:win32-g++:   LIBS += -L$$PWD/libs/libusb/MinGW32/static/ -lusb-1.0

win32:!win32-g++: INCLUDEPATH += $$PWD/libs/libusb/MS64/dll
else:win32-g++:   INCLUDEPATH += $$PWD/libs/libusb/MinGW32/static

win32:!win32-g++: DEPENDPATH += $$PWD/libs/libusb/MS64/dll
else:win32-g++:   DEPENDPATH += $$PWD/libs/libusb/MinGW32/static

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/libs/libusb/MS64/dll/libusb-1.0.lib
else:win32-g++:   PRE_TARGETDEPS += $$PWD/libs/libusb/MinGW32/static/libusb-1.0.a



win32: LIBS += -L$$PWD/libs/cyapi/x64/ -lCyAPI -llegacy_stdio_definitions

win32: INCLUDEPATH += $$PWD/libs/cyapi/x86
win32: DEPENDPATH += $$PWD/libs/cyapi/x86

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/libs/cyapi/x64/CyAPI.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/libs/cyapi/x64/libCyAPI.a

unix:!macx: LIBS += -lusb-1.0
unix: LIBS += -lpthread
