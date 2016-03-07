#pragma once
#include <mutex>

#include "hex.hpp"
#include "fwd.hpp"
#include "vec.hpp"
#include "math_lib/math.hpp"
#include "chunk_phys_mesh.hpp"

class h_Player
{
public:
	h_Player( const h_WorldPtr& world, const h_WorldHeaderPtr& world_header );
	~h_Player();

	void Move( const m_Vec3& direction );
	void Rotate( const m_Vec3& delta );
	void ToggleFly();
	void Jump();

	const m_Vec3& Pos() const;
	const m_Vec3& Angle() const;

	const m_Vec3& BuildPos() const;
	h_Direction BuildDirection() const;

	void Tick();

	void PauseWorldUpdates();
	void UnpauseWorldUpdates();

	void SetBuildBlock( h_BlockType block_type );
	h_BlockType BuildBlock() const;

	void Build();
	void Dig();
	void TestMobSetPosition();

	void SetCollisionMesh( h_ChunkPhysMesh mesh );

	void Lock();
	void Unlock();

private:
	void UpdateBuildPos();
	void MoveInternal( const m_Vec3& delta );

private:
	const h_WorldPtr world_;
	const h_WorldHeaderPtr world_header_;
	m_Vec3 pos_;
	m_Vec3 speed_;
	float vertical_speed_;
	bool is_flying_;
	bool in_air_;
	m_Vec3 view_angle_;

	uint64_t prev_move_time_ms_;

	m_Vec3 build_pos_;
	short discret_build_pos_[3]; // World space build position.
	h_Direction build_direction_; // Firection of build side. Unknown, if build block does not exist
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

