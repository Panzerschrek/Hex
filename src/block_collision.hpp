#pragma once
#include "hex.hpp"
#include "vec.hpp"

class p_UpperBlockFace
{
public:
	p_UpperBlockFace( short x, short y, short z, h_Direction dir );

	bool HasCollisionWithCircle( const m_Vec2& pos, float radius ) const;

	m_Vec2 edge[6];
	float z;
	h_Direction dir;//up/down
};

class p_BlockSide
{
public:
	p_BlockSide( short x, short y, short z, h_Direction dir );

	bool HasCollisionWithCircle( const m_Vec2& pos, float radius ) const;
	m_Vec2 CollideWithCirlce( const m_Vec2& pos, float radius ) const;// returns new position of circle

	m_Vec2 edge[2];//xy coordinates of edge vertices
	float z;//coordinate of lower edge
	h_Direction dir;
};

struct p_WaterBlock
{
	// World coordinates of water block.
	int x, y, z;
	// Water level in this block in range [0;1].
	float water_level;
};

void pGetHexogonCoord( const m_Vec2& pos, short* x, short* y );

bool pRayHasIniersectWithTriangle(
	const m_Vec3* triangle, const m_Vec3& normal,
	const m_Vec3& point, const m_Vec3& dir,
	m_Vec3* intersect_pos );

