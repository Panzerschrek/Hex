# Initially, CONFIG contains a lot of stuff like
# "debug release debug_and_release release_and_debug other_trash debug".
# Make debug build, only if "debug" is last word in config.
H_CONFIG_LAST= $$last(CONFIG)
# Remove stuff.
CONFIG=

equals( H_CONFIG_LAST, debug ) {
	CONFIG+= debug
	H_BUILD_TYPE_TARGET_PATH= debug
} else {
	CONFIG+= release
	H_BUILD_TYPE_TARGET_PATH= release
}

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

H_BUILD_ROOT_DIR= ../obj/qmake/$$H_BUILD_TYPE_TARGET_PATH

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
