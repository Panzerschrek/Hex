TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    ../src/test/test.cpp \
    ../src/test/test_test.cpp

HEADERS += \
    ../src/test/test.h

QMAKE_CXXFLAGS += -std=c++11

debug {
} else {
	QMAKE_CXXFLAGS  += -O2 -s
}


