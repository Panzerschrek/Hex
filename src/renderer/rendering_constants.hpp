#pragma once
#include "vec.hpp"
#include "../hex.hpp"

const m_Vec3 R_FIRE_LIGHT_COLOR( 1.4f, 1.2f, 0.8f );
const m_Vec3 R_DAY_SKY_LIGHT_COLOR( 0.95f, 0.95f, 1.0f );
const m_Vec3 R_NIGHT_SKY_LIGHT_COLOR( 0.2f, 0.2f, 0.2f );
const m_Vec3 R_AMBIENT_LIGHT_COLOR( 0.06f, 0.06f, 0.06f );

const m_Vec3 R_SKYBOX_COLOR( 1.0f, 1.0f, 1.5f );
const m_Vec3 R_CLOUDS_COLOR( 0.8f, 0.8f, 0.8f );
const m_Vec3 R_CLOUDS_NIGHT_COLOR( 0.1f, 0.1f, 0.1f );

const float R_SUN_LIGHT_FADE_WHILE_RAIN= 0.5f;

const float R_RAIN_ZONE_RADIUS_M= 24.0f;
const float R_RAIN_ZONE_HEIGHT_M= 48.0f;
const float R_RAIN_SPEED_MPS= 8.0f;
const float R_RAIN_DENSITY= 0.3f;// particles per cubic meter

const float R_SCENE_RADIUS= float( H_MAX_CHUNKS * H_CHUNK_WIDTH * 2 );

#define R_MAX_TEXTURES 256

#define R_MAX_TEXTURE_RESOLUTION_LOG2 8
#define R_MAX_TEXTURE_RESOLUTION (1 << R_MAX_TEXTURE_RESOLUTION_LOG2)
#define R_MIN_TEXTURE_RESOLUTION_LOG2 3
#define R_MIN_TEXTURE_RESOLUTION (1 << R_MIN_TEXTURE_RESOLUTION_LOG2)

#define H_TEXTURE_SCALE_MULTIPLIER 4
#define H_TEXTURE_MAX_SCALE 4

//z coord of water vertices multiplied by this coeff
#define R_WATER_VERTICES_Z_SCALER_LOG2 7
#define R_WATER_VERTICES_Z_SCALER (1 << R_WATER_VERTICES_Z_SCALER_LOG2)
