cmake_minimum_required(VERSION 2.8)

project(Hex)

set( CMAKE_CXX_STANDARD 11 )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DDEBUG )
else()
endif()

# Set path to your SDL2 here.
if( WIN32 )
	set( SDL_BASE_DIR ../SDL2-2.0.3 )
	set( SDL_INCLUDES_DIR ${SDL_BASE_DIR}/include )
	set( SDL_LIBS_DIR ${SDL_BASE_DIR}/lib/x86 )
	get_filename_component( SDL_LIBS_DIR_ABSOLUTE ${SDL_LIBS_DIR} ABSOLUTE )
else()
	set( SDL_INCLUDES_DIR /usr/include/SDL2 )
endif()

#
# HexLib
#

set( HEX_LIB_HEADERS
	src/math_lib/binary_stream.hpp
	src/block.hpp
	src/block_collision.hpp
	src/blocks_list.hpp
	src/calendar.hpp
	src/chunk.hpp
	src/chunk_loader.hpp
	src/console.hpp
	src/fwd.hpp
	src/hex.hpp
	src/main_loop.hpp
	src/math_lib/allocation_free_list.hpp
	src/math_lib/allocation_free_set.hpp
	src/math_lib/assert.hpp
	src/math_lib/fixed.hpp
	src/math_lib/math.hpp
	src/math_lib/rand.hpp
	src/math_lib/small_objects_allocator.hpp
	src/path_finder.hpp
	src/player.hpp
	src/renderer/chunk_info.hpp
	src/renderer/fire_mesh.hpp
	src/renderer/i_world_renderer.hpp
	src/renderer/img_utils.hpp
	src/renderer/rendering_constants.hpp
	src/renderer/text.hpp
	src/renderer/texture_manager.hpp
	src/renderer/weather_effects_particle_manager.hpp
	src/renderer/world_renderer.hpp
	src/renderer/wvb.hpp
	src/settings.hpp
	src/settings_keys.hpp
	src/ticks_counter.hpp
	src/time.hpp
	src/world.hpp
	src/world_action.hpp
	src/world_loading.hpp
	src/ui/ingame_menu.hpp
	src/ui/loading_menu.hpp
	src/ui/main_menu.hpp
	src/ui/styles.hpp
	src/ui/ui_base_classes.hpp
	src/ui/ui_painter.hpp
	src/world_generator/noise.hpp
	src/world_generator/rivers.hpp
	src/world_generator/world_generator.hpp
	src/world_header.hpp
	src/world_phys_mesh.hpp )

set( HEX_LIB_SOURCES
	src/block.cpp
	src/block_collision.cpp
	src/calendar.cpp
	src/chunk.cpp
	src/chunk_loader.cpp
	src/console.cpp
	src/main.cpp
	src/main_loop.cpp
	src/math_lib/math.cpp
	src/math_lib/rand.cpp
	src/path_finder.cpp
	src/player.cpp
	src/renderer/chunk_info.cpp
	src/renderer/fire_mesh.cpp
	src/renderer/img_utils.cpp
	src/renderer/text.cpp
	src/renderer/texture_manager.cpp
	src/renderer/weather_effects_particle_manager.cpp
	src/renderer/world_renderer.cpp
	src/renderer/wvb.cpp
	src/settings.cpp
	src/settings_keys.cpp
	src/ticks_counter.cpp
	src/time.cpp
	src/ui/ingame_menu.cpp
	src/ui/loading_menu.cpp
	src/ui/main_menu.cpp
	src/ui/ui_base_classes.cpp
	src/ui/ui_painter.cpp
	src/world.cpp
	src/world_generator/noise.cpp
	src/world_generator/rivers.cpp
	src/world_generator/world_generator.cpp
	src/world_header.cpp
	src/world_lighting.cpp
	src/world_loading.cpp
	src/world_phys_mesh.cpp )

set( PANZER_OGL_LIB_HEADERS
	panzer_ogl_lib/framebuffer.hpp
	panzer_ogl_lib/func_declarations.hpp
	panzer_ogl_lib/glcorearb.h
	panzer_ogl_lib/glsl_program.hpp
	panzer_ogl_lib/matrix.hpp
	panzer_ogl_lib/ogl_state_manager.hpp
	panzer_ogl_lib/panzer_ogl_lib.hpp
	panzer_ogl_lib/polygon_buffer.hpp
	panzer_ogl_lib/shaders_loading.hpp
	panzer_ogl_lib/texture.hpp
	panzer_ogl_lib/vec.hpp )

set( PANZER_OGL_LIB_SOURCES
	panzer_ogl_lib/framebuffer.cpp
	panzer_ogl_lib/func_addresses.cpp
	panzer_ogl_lib/glsl_program.cpp
	panzer_ogl_lib/matrix.cpp
	panzer_ogl_lib/ogl_state_manager.cpp
	panzer_ogl_lib/polygon_buffer.cpp
	panzer_ogl_lib/shaders_loading.cpp
	panzer_ogl_lib/texture.cpp )

set( PANZER_JSON_HEADERS
	PanzerJson/include/PanzerJson/fwd.hpp
	PanzerJson/include/PanzerJson/value.hpp
	PanzerJson/include/PanzerJson/value.inl
	PanzerJson/include/PanzerJson/parser.hpp
	PanzerJson/include/PanzerJson/serializer.hpp
	PanzerJson/include/PanzerJson/serializer.inl
	PanzerJson/include/PanzerJson/streamed_serializer.hpp
	PanzerJson/include/PanzerJson/streamed_serializer.inl )

set( PANZER_JSON_SOURCES
	PanzerJson/src/panzer_json_assert.hpp
	PanzerJson/src/parser.cpp
	PanzerJson/src/serializer.cpp
	PanzerJson/src/serializers_common.cpp
	PanzerJson/src/serializers_common.hpp
	PanzerJson/src/serializers_common.inl
	PanzerJson/src/value.cpp )

set( STB_IMAGE_HEADERS stb/stb_image.h )

include_directories( panzer_ogl_lib )
include_directories( PanzerJson/include )
include_directories( ${SDL_INCLUDES_DIR} )

add_library(
	HexLib
	${HEX_LIB_HEADERS} ${HEX_LIB_SOURCES}
	${PANZER_OGL_LIB_HEADERS} ${PANZER_OGL_LIB_SOURCES}
	${PANZER_JSON_HEADERS} ${PANZER_JSON_SOURCES}
	${STB_IMAGE_HEADERS} )

if( WIN32 )
	target_link_libraries( HexLib opengl32 )
	target_link_libraries( HexLib ${SDL_LIBS_DIR_ABSOLUTE}/SDL2main.lib )
	target_link_libraries( HexLib ${SDL_LIBS_DIR_ABSOLUTE}/SDL2.lib )
else()
	target_link_libraries( HexLib GL )
	target_link_libraries( HexLib SDL2 )
endif()

# On GNU/Linux systems zlib is system, so we can found it.
# MinGW contains zlib too.
target_link_libraries( HexLib z )

#
# Hex
#

set( HEX_SOURCES
	src/main.cpp )

add_executable( Hex ${HEX_SOURCES} )
target_link_libraries( Hex HexLib )

#
# Tests
#

set( TESTS_SOURCES
	src/test/test.cpp
	src/test/test_test.cpp
	src/test/math_test.cpp
	src/test/calendar_test.cpp
	src/test/allocation_free_list_test.cpp
	src/test/allocation_free_set_test.cpp
	src/test/fixed_test.cpp )

set( TESTS_HEADERS
	src/test/test.h )

add_executable( Tests ${TESTS_SOURCES} ${TESTS_HEADERS} )
target_link_libraries( Tests HexLib )
