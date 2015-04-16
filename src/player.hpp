#pragma once
#include "hex.hpp"
#include "vec.hpp"
#include "math_lib/m_math.h"
#include "QMutex"
#include "chunk_phys_mesh.hpp"
class h_World;

class h_Player
{
public:

	h_Player( h_World* w );
	~h_Player();

	void Move( m_Vec3 delta );
	void Rotate( m_Vec3 delta );

	m_Vec3 Pos() const;
	m_Vec3 Angle() const;

	void SetCollisionMesh( h_ChunkPhysMesh* mesh );
	h_Direction GetBuildPos( short* x, short* y, short* z );

	void Lock();
	void Unlock();
private:

	const h_World* world;
	m_Vec3 pos;
	m_Vec3 view_angle;

	QMutex player_data_mutex;

	h_ChunkPhysMesh phys_mesh;
};


inline m_Vec3 h_Player::Pos() const
{
	return pos;
}
inline m_Vec3 h_Player::Angle() const
{
	return view_angle;
}

inline void h_Player::Rotate( m_Vec3 delta )
{
	view_angle+= delta;

	if( view_angle.z < 0.0f ) view_angle.z+= m_Math::FM_2PI;
	else if( view_angle.z > m_Math::FM_2PI ) view_angle.z-= m_Math::FM_2PI;

	if( view_angle.x > m_Math::FM_PI2 ) view_angle.x= m_Math::FM_PI2;
	else if( view_angle.x < -m_Math::FM_PI2 ) view_angle.x= -m_Math::FM_PI2;
}


inline void h_Player::Lock()
{
	player_data_mutex.lock();
}

inline void h_Player::Unlock()
{
	player_data_mutex.unlock();
}
