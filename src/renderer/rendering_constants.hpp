#pragma once
#include "vec.hpp"

const m_Vec3 R_FIRE_LIGHT_COLOR( 1.4f, 1.2f, 0.8f );
const m_Vec3 R_SUN_LIGHT_COLOR( 0.95f, 0.95f, 1.0f  );
const m_Vec3 R_AMBIENT_LIGHT_COLOR( 0.06f, 0.06f, 0.06f );

#define R_MAX_TEXTURES 256
#define R_MAX_TEXTURE_RESOLUTION 256
#define R_MIN_TEXTURE_RESOLUTION 4

//z coord of water vertices multiplied by this coeff
#define R_WATER_VERTICES_Z_SCALER_LOG2 7
#define R_WATER_VERTICES_Z_SCALER (1 << R_WATER_VERTICES_Z_SCALER_LOG2)
