include (common.pri)

TEMPLATE= app
CONFIG+= console
TARGET= $$OBJECTS_DIR/Hex

SOURCES+= ../src/main.cpp

LIBS+= $$OBJECTS_DIR/libhex_lib.a
LIBS+= libopengl32
