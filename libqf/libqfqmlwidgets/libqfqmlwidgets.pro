message("including $$PWD")

TEMPLATE = lib
TARGET=qfqmlwidgets
unix:DESTDIR = $$OUT_PWD/../../lib
win32:DESTDIR = $$OUT_PWD/../../bin

QT += widgets qml sql
QT += xml printsupport svg # needed by reports

CONFIG += c++11
CONFIG += hide_symbols

#QMAKE_CXXFLAGS += -std=c++1y

DEFINES += QFQMLWIDGETS_BUILD_DLL

LIBS +=      \
	-lqfcore  \

win32: LIBS +=  \
	-L../../bin  \

unix: LIBS +=  \
	-L../../lib  \

include ($$PWD/../../crosscompile-support.pri)

include($$PWD/src/src.pri)

RESOURCES += \
    $${TARGET}.qrc \
