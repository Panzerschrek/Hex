	/*
	*This file is part of FREG.
	*
	*FREG is free software: you can redistribute it and/or modify
	*it under the terms of the GNU General Public License as published by
	*the Free Software Foundation, either version 3 of the License, or
	*(at your option) any later version.
	*
	*FREG is distributed in the hope that it will be useful,
	*but WITHOUT ANY WARRANTY; without even the implied warranty of
	*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	*GNU General Public License for more details.
	*
	*You should have received a copy of the GNU General Public License
	*along with FREG. If not, see <http://www.gnu.org/licenses/>.
	*/

#ifdef OGL21
#define AMBIENT_SKY_DAY_LIGHT 0.08f
#define AMBIENT_SKY_NIGHT_LIGHT 0.01f
#define AMBIENT_FIRE_LIGHT 0.04f
#else//if OGL3.3
#define AMBIENT_SKY_DAY_LIGHT 0.5f
#define AMBIENT_SKY_NIGHT_LIGHT 0.001f
#define AMBIENT_FIRE_LIGHT 0.2f
#endif//OGL21

#define DIRECT_SUN_LIGHT 20.0f

#define DEFAULT_FONT_WIDTH 10
#define DEFAULT_FONT_HEIGHT 14

#define INVENTORY_WIDTH 27
#define PLAYER_INVENTORY_WIDTH 33//in symbols

#define MAP_SHRED_COLOR_DEFAULT		m_Vec3( 1.0f, 1.0f, 1.0f )
#define MAP_SHRED_COLOR_WATER 		m_Vec3( 0.1f, 0.1f, 1.0f )
#define MAP_SHRED_COLOR_PLAIN 		m_Vec3( 0.1f, 0.8f, 0.1f )
#define MAP_SHRED_COLOR_FOREST 		m_Vec3( 0.1f, 0.6f, 0.1f )
#define MAP_SHRED_COLOR_HILL		m_Vec3( 0.4f, 0.6f, 0.4f )
#define MAP_SHRED_COLOR_MOUNTAIN 	m_Vec3( 0.5f, 0.5f, 0.5f )
#define MAP_SHRED_COLOR_DESERT		m_Vec3( 0.4f, 0.4f, 0.1f )
#define MAP_SHRED_COLOR_TEST		m_Vec3( 0.8f, 0.8f, 0.8f )
#define MAP_SHRED_COLOR_PYRAMID		m_Vec3( 0.7f, 0.7f, 0.6f )
#define MAP_PLAYER_COLOR			m_Vec3( 1.0f, 0.3f, 0.3f )

#define MAP_SHRED_COLOR_ID_DEFAULT		0
#define MAP_SHRED_COLOR_ID_WATER 		1
#define MAP_SHRED_COLOR_ID_PLAIN 		2
#define MAP_SHRED_COLOR_ID_FOREST 		3
#define MAP_SHRED_COLOR_ID_HILL			4
#define MAP_SHRED_COLOR_ID_MOUNTAIN 	5
#define MAP_SHRED_COLOR_ID_DESERT		6
#define MAP_SHRED_COLOR_ID_TEST			7
#define MAP_SHRED_COLOR_ID_PYRAMID		8
#define MAP_PLAYER_COLOR_ID				9

