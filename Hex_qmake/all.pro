TEMPLATE= subdirs
CONFIG=

MAKEFILE= Makefile.all

# We have 3 targets: lib, application, tests.
# Tests depends on library. Application depends on lib and tests.
SUBDIRS= hex hex_lib tests
hex.file= hex.pro
hex_lib.file= hex_lib.pro
tests.file= tests.pro

hex.depends= tests hex_lib
tests.depends= hex_lib
