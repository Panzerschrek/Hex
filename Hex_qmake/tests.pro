include (common.pri)

TEMPLATE = app
TARGET= $$OBJECTS_DIR/HexTest
CONFIG += console

SOURCES += \
    ../src/test/test.cpp \
    ../src/test/test_test.cpp

HEADERS += \
    ../src/test/test.h

LIBS+= $$OBJECTS_DIR/libhex_lib.a
LIBS+= libopengl32
