include (common.pri)

TEMPLATE = app
TARGET= $$OBJECTS_DIR/HexTest
CONFIG+= console

MAKEFILE= Makefile.tests

# Run target after build
QMAKE_POST_LINK= $$TARGET

SOURCES += \
    ../src/test/test.cpp \
    ../src/test/test_test.cpp \
    ../src/test/math_test.cpp \
    ../src/test/calendar_test.cpp \
    ../src/test/allocation_free_list_test.cpp

HEADERS += \
    ../src/test/test.h

LIBS+= $$OBJECTS_DIR/libhex_lib.a
LIBS+= libopengl32
