HEADERS += \
    ../src/block.hpp \
    ../src/block_collision.hpp \
    ../src/chunk.hpp \
    ../src/chunk_loader.hpp \
    ../src/chunk_phys_mesh.hpp \
    ../src/console.hpp \
    ../src/fwd.hpp \
    ../src/hex.hpp \
    ../src/main_loop.hpp \
    ../src/player.hpp \
    ../src/settings.hpp \
    ../src/settings_keys.hpp \
    ../src/ticks_counter.hpp \
    ../src/world.hpp \
    ../src/world_action.hpp \
    ../src/world_loading.hpp \
    ../src/math_lib/allocation_free_list.hpp \
    ../src/math_lib/allocation_free_set.hpp \
    ../src/math_lib/assert.hpp \
    ../src/math_lib/collection.hpp \
    ../src/math_lib/m_math.h \
    ../src/math_lib/rand.h \
    ../src/math_lib/small_objects_allocator.hpp \
    ../src/renderer/chunk_data_cache.hpp \
    ../src/renderer/i_world_renderer.hpp \
    ../src/renderer/img_utils.hpp \
    ../src/renderer/rendering_constants.hpp \
    ../src/renderer/text.hpp \
    ../src/renderer/texture_manager.hpp \
    ../src/renderer/weather_effects_particle_manager.hpp \
    ../src/renderer/world_renderer.hpp \
    ../src/renderer/wvb.hpp \
    ../src/ui/main_menu.hpp \
    ../src/ui/ui_base_classes.hpp \
    ../src/ui/ui_painter.hpp \
    ../../panzer_ogl_lib/framebuffer.hpp \
    ../../panzer_ogl_lib/framebuffer_texture.hpp \
    ../../panzer_ogl_lib/func_declarations.hpp \
    ../../panzer_ogl_lib/glcorearb.h \
    ../../panzer_ogl_lib/glsl_program.hpp \
    ../../panzer_ogl_lib/matrix.hpp \
    ../../panzer_ogl_lib/ogl_state_manager.hpp \
    ../../panzer_ogl_lib/panzer_ogl_lib.hpp \
    ../../panzer_ogl_lib/polygon_buffer.hpp \
    ../../panzer_ogl_lib/vec.hpp \
    ../src/renderer/chunk_info.hpp \
    ../src/ui/ingame_menu.hpp \
    ../src/ui/styles.hpp \
    ../src/world_generator/world_generator.hpp \
    ../src/math_lib/fixed.hpp \
    ../src/world_generator/noise.hpp \
    ../src/world_generator/rivers.hpp

SOURCES += \
    ../src/block.cpp \
    ../src/block_collision.cpp \
    ../src/chunk.cpp \
    ../src/chunk_loader.cpp \
    ../src/chunk_phys_mesh.cpp \
    ../src/console.cpp \
    ../src/main.cpp \
    ../src/main_loop.cpp \
    ../src/player.cpp \
    ../src/settings.cpp \
    ../src/settings_keys.cpp \
    ../src/ticks_counter.cpp \
    ../src/world.cpp \
    ../src/world_lighting.cpp \
    ../src/world_loading.cpp \
    ../src/math_lib/m_math.cpp \
    ../src/math_lib/rand.cpp \
    ../src/renderer/chunk_data_cache.cpp \
    ../src/renderer/chunk_info.cpp \
    ../src/renderer/img_utils.cpp \
    ../src/renderer/text.cpp \
    ../src/renderer/texture_manager.cpp \
    ../src/renderer/weather_effects_particle_manager.cpp \
    ../src/renderer/world_renderer.cpp \
    ../src/renderer/wvb.cpp \
    ../src/ui/main_menu.cpp \
    ../src/ui/ui_base_classes.cpp \
    ../src/ui/ui_painter.cpp \
    ../../panzer_ogl_lib/framebuffer.cpp \
    ../../panzer_ogl_lib/framebuffer_texture.cpp \
    ../../panzer_ogl_lib/func_addresses.cpp \
    ../../panzer_ogl_lib/glsl_program.cpp \
    ../../panzer_ogl_lib/matrix.cpp \
    ../../panzer_ogl_lib/ogl_state_manager.cpp \
    ../../panzer_ogl_lib/polygon_buffer.cpp \
    ../src/ui/ingame_menu.cpp \
    ../src/world_generator/world_generator.cpp \
    ../src/world_generator/noise.cpp \
    ../src/world_generator/rivers.cpp


QT += widgets opengl
INCLUDEPATH = ../../panzer_ogl_lib
QMAKE_CXXFLAGS += -std=c++11 -fno-exceptions

debug {
} else {
	QMAKE_CXXFLAGS  += -O2 -s
}
