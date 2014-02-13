#ifndef BLOCK_COLLISION_HPP
#define BLOCK_COLLISION_HPP


#include "hex.hpp"
#include "math_lib/vec.h"
class p_UpperBlockFace
{
public:
	p_UpperBlockFace(){}
	~p_UpperBlockFace(){}
	void Gen( short x, short y, short z, h_Direction dir );
    bool HasCollisionWithCircle( m_Vec2 pos,  float radius );
    template< unsigned char (*func)(short, short, short ) >
    void BuildSubMesh();

    m_Vec2 edge[6];
    float z;
    h_Direction dir;//up/down
};

class p_BlockSide
{
public:
	p_BlockSide(){}
	~p_BlockSide(){}
    void Gen( short x, short y, short z, h_Direction dir );
	bool HasCollisionWithCircle( m_Vec2 pos,  float radius );
	m_Vec2 CollideWithCirlce( m_Vec2 pos, float radius );// returns new position of circle

    m_Vec2 edge[2];//xy coordinates of edge vertices
    float z;//coordinate of lower edge
    h_Direction dir;
};


void GetHexogonCoord( m_Vec2 pos, short* x, short* y );
bool RayHasIniersectWithTriangle( m_Vec3* triangle, m_Vec3& normal, m_Vec3& point, m_Vec3& dir, m_Vec3* intersect_pos );

#endif//BLOCK_COLLISION_HPP

