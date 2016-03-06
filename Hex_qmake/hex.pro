include (common.pri)

TEMPLATE= app
CONFIG+= console
TARGET= $$OBJECTS_DIR/Hex

MAKEFILE= Makefile.hex

SOURCES+= ../src/main.cpp

LIBS+= $$OBJECTS_DIR/libhex_lib.a
LIBS+= libopengl32
