QT += core gui qml quick quickcontrols2 quickdialogs2 xml concurrent dbus
CONFIG += c++17 release
TARGET = omabook
TEMPLATE = app

SOURCES += src/main.cpp src/epubloader.cpp src/readercontroller.cpp src/omarchytheme.cpp src/systemtheme.cpp
HEADERS += src/epubloader.h src/readercontroller.h src/omarchytheme.h src/systemtheme.h
RESOURCES += src/resources.qrc
LIBS += -lzip

