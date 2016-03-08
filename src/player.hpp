#pragma once
#include <mutex>

#include "hex.hpp"
#include "fwd.hpp"
#include "vec.hpp"

// Player. Methods must be called in UIi thread, unless methods, marked as "thread safe".
class h_Player
{
public:
	h_Player( const h_WorldPtr& world, const h_WorldHeaderPtr& world_header );
	~h_Player();

	// Set moving vector.
	// x - left/right, y - forward/backward, z - up/down.
	void SetMovingVector( const m_Vec3& moving_vector );
	void Rotate( const m_Vec3& delta );
	void ToggleFly();
	void Jump();

	// Get position, angle. Methods is thread safe.
	m_Vec3 Pos() const;
	m_Vec3 Angle() const;

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

private:
	void UpdateBuildPos();
	void Move( const m_Vec3& delta );

private:
	const h_WorldPtr world_;
	const h_WorldHeaderPtr world_header_;

	// All vectors and positions - in world space.
	m_Vec3 moving_vector_;
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

	mutable std::mutex player_data_mutex_;
};

inline m_Vec3 h_Player::Pos() const
{
	std::unique_lock<std::mutex> lock( player_data_mutex_ );
	return pos_;
}

inline m_Vec3 h_Player::Angle() const
{
	std::unique_lock<std::mutex> lock( player_data_mutex_ );
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
