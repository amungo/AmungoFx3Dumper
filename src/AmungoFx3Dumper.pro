TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    hwfx3/fx3dev.cpp \
    hwfx3/fx3devcyapi.cpp \
    hwfx3/fx3deverr.cpp \
    hwfx3/fx3fwparser.cpp \
    hwfx3/HexParser.cpp \
    processors/streamdumper.cpp \
    sys/NamedClass.cpp \
    sys/Runnable.cpp

HEADERS += \
    hwfx3/cy_inc/CyAPI.h \
    hwfx3/cy_inc/cyioctl.h \
    hwfx3/cy_inc/CyUSB30_def.h \
    hwfx3/cy_inc/usb100.h \
    hwfx3/cy_inc/usb200.h \
    hwfx3/cy_inc/UsbdStatus.h \
    hwfx3/cy_inc/VersionNo.h \
    hwfx3/fx3dev.h \
    hwfx3/fx3devcyapi.h \
    hwfx3/fx3devdebuginfo.h \
    hwfx3/fx3deverr.h \
    hwfx3/fx3devifce.h \
    hwfx3/fx3fwparser.h \
    hwfx3/HexParser.h \
    hwfx3/libusb.h \
    processors/streamdumper.h \
    sys/AsyncQueueHandler.h \
    sys/BlockQueue.h \
    sys/MessageReceiverIfce.h \
    sys/NamedClass.h \
    sys/Runnable.h \
    sys/SingleEvent.h




win32: LIBS += -luser32
win32: LIBS += -lsetupapi
win32: LIBS += /NODEFAULTLIB:LIBCMT

win32:!win32-g++: LIBS += -L$$PWD/libs/libusb/MS32/static/ -llibusb-1.0
else:win32-g++:   LIBS += -L$$PWD/libs/libusb/MinGW32/static/ -lusb-1.0

win32:!win32-g++: INCLUDEPATH += $$PWD/libs/libusb/MS32/static
else:win32-g++:   INCLUDEPATH += $$PWD/libs/libusb/MinGW32/static

win32:!win32-g++: DEPENDPATH += $$PWD/libs/libusb/MS32/static
else:win32-g++:   DEPENDPATH += $$PWD/libs/libusb/MinGW32/static

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/libs/libusb/MS32/static/libusb-1.0.lib
else:win32-g++:   PRE_TARGETDEPS += $$PWD/libs/libusb/MinGW32/static/libusb-1.0.a



win32: LIBS += -L$$PWD/libs/cyapi/x86/ -lCyAPI

win32: INCLUDEPATH += $$PWD/libs/cyapi/x86
win32: DEPENDPATH += $$PWD/libs/cyapi/x86

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/libs/cyapi/x86/CyAPI.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/libs/cyapi/x86/libCyAPI.a

unix:!macx: LIBS += -lusb-1.0
