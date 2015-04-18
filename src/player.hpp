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
	h_Player( const h_World* w );
	~h_Player();

	void Move( const m_Vec3& delta );
	void Rotate( const m_Vec3& delta );

	m_Vec3 Pos() const;
	m_Vec3 Angle() const;

	void SetCollisionMesh( h_ChunkPhysMesh* mesh );
	h_Direction GetBuildPos( short* x, short* y, short* z ) const;

	void Lock();
	void Unlock();

private:
	const h_World* world_;
	m_Vec3 pos_;
	m_Vec3 view_angle_;

	QMutex player_data_mutex_;

	h_ChunkPhysMesh phys_mesh_;
};

inline m_Vec3 h_Player::Pos() const
{
	return pos_;
}
inline m_Vec3 h_Player::Angle() const
{
	return view_angle_;
}

inline void h_Player::Lock()
{
	player_data_mutex_.lock();
}

inline void h_Player::Unlock()
{
	player_data_mutex_.unlock();
}
