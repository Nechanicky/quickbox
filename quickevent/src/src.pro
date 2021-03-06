message(including $$PWD)
QT += core gui qml widgets

CONFIG += C++11

TEMPLATE = app

DESTDIR = $$OUT_PWD/../../bin/
TARGET = quickevent

LIBS +=      \
	-lqfcore  \
	-lqfqmlwidgets  \

win32: LIBS +=  \
	-L../../bin  \

DOLAR=$

unix: LIBS +=  \
	-L../../lib  \
	-Wl,-rpath,\'$${DOLAR}$${DOLAR}ORIGIN/../lib\'  \

include ($$PWD/../../crosscompile-support.pri)

include($$PWD/src.pri)

OTHER_FILES += \
    ../plugins/qml/Core/*.qml \
    ../plugins/qml/SqlDb/*.qml \
    ../plugins/qml/Event/*.qml \
    ../plugins/qml/Help/*.qml \

QML_IMPORT_PATH += \
    $$OUT_PWD/../lib/qml \
    $$OUT_PWD/../../bin/divers/quickevent/plugins \
