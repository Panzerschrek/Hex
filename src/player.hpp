#pragma once
#include <mutex>

#include "hex.hpp"
#include "vec.hpp"
#include "math_lib/m_math.h"
#include "chunk_phys_mesh.hpp"

class h_Player
{
public:
	h_Player( const h_WorldPtr& world );
	~h_Player();

	void Move( const m_Vec3& delta );
	void Rotate( const m_Vec3& delta );

	const m_Vec3& Pos() const;
	const m_Vec3& Angle() const;

	const m_Vec3& BuildPos() const;
	h_Direction BuildDirection() const;

	void Tick();

	void SetBuildBlock( h_BlockType block_type );
	h_BlockType BuildBlock() const;

	void Build();
	void Dig();

	void SetCollisionMesh( h_ChunkPhysMesh* mesh );

	void Lock();
	void Unlock();

private:
	void UpdateBuildPos();

private:
	const h_WorldPtr world_;
	m_Vec3 pos_;
	m_Vec3 view_angle_;

	m_Vec3 build_pos_;
	short discret_build_pos_[3]; // World space build position.
	h_Direction build_direction_;

	h_BlockType build_block_;

	std::mutex player_data_mutex_;

	h_ChunkPhysMesh phys_mesh_;
};

inline const m_Vec3& h_Player::Pos() const
{
	return pos_;
}

inline const m_Vec3& h_Player::Angle() const
{
	return view_angle_;
}

inline const m_Vec3& h_Player::BuildPos() const
{
	return build_pos_;
}

inline h_Direction h_Player::BuildDirection() const
{
	return build_direction_;
}

inline h_BlockType h_Player::BuildBlock() const
{
	return build_block_;
}

inline void h_Player::Lock()
{
	player_data_mutex_.lock();
}

inline void h_Player::Unlock()
{
	player_data_mutex_.unlock();
}

