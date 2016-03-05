#message( CONFIG: $$CONFIG )
CONFIG=

CONFIG+= qt
CONFIG+= exceptions_off
CONFIG+= c++11
CONFIG+= import_plugins
CONFIG+= import_qpa_plugin
CONFIG+= link_prl
CONFIG+= create_prl
CONFIG+= win32 yacc

QT+= widgets
QT+= opengl

H_BUILD_ROOT_DIR= ../out

OBJECTS_DIR= $$H_BUILD_ROOT_DIR
MOC_DIR= $$H_BUILD_ROOT_DIR

INCLUDEPATH=
INCLUDEPATH+= ../../panzer_ogl_lib

QMAKE_CXXFLAGS=
QMAKE_CXXFLAGS+= -Wall -Wextra

debug {
	DEFINES += DEBUG
	QMAKE_CXXFLAGS+= -g
} else {
	QMAKE_CXXFLAGS+= -O3 -s
	QMAKE_LFLAGS+= -flto
}
