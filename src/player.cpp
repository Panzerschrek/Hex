#include "player.hpp"
#include "block_collision.hpp"
#include "world_phys_mesh.hpp"
#include "world_header.hpp"
#include "world.hpp"
#include "time.hpp"
#include "math_lib/assert.hpp"
#include "math_lib/math.hpp"

#include "matrix.hpp"

static const float g_default_player_radius= 0.25f * 0.9f; // 90% of block side
static const float g_default_player_eyes_level= 1.67f;
static const float g_default_player_height= 1.75f;

static const float g_acceleration= 40.0f;
static const float g_deceleration= 40.0f;
static const float g_air_acceleration= 2.0f;
static const float g_air_deceleration= 4.0f;
static const float g_water_acceleration= 4.0f;
static const float g_water_deceleration= 8.0f;
static const float g_vertical_acceleration= -9.8f * 1.5f;
static const float g_vertical_water_acceleration= g_vertical_acceleration * 0.03f;
static const float g_max_speed= 5.0f;
static const float g_max_underwater_speed_= 3.0f;
static const float g_max_vertical_speed= 30.0f;
static const float g_max_underwater_vertical_speed= 2.0f;

static const float g_jump_height= 1.4f;
static const float g_jump_speed= std::sqrt( 2.0f * g_jump_height * -g_vertical_acceleration );

static const float g_max_build_distance= 4.0f;

static const m_Vec3 g_block_normals[8]=
{
	m_Vec3( 0.0f, 1.0f, 0.0f ),      m_Vec3( 0.0f, -1.0f, 0.0f ),
	m_Vec3( +H_SPACE_SCALE_VECTOR_X, 0.5f, 0.0 ), m_Vec3( -H_SPACE_SCALE_VECTOR_X, -0.5f, 0.0 ),
	m_Vec3( -H_SPACE_SCALE_VECTOR_X, 0.5f, 0.0 ), m_Vec3( +H_SPACE_SCALE_VECTOR_X, -0.5f, 0.0 ),
	m_Vec3( 0.0f, 0.0f, 1.0f ),      m_Vec3( 0.0f, 0.0f, -1.0f )
};

h_Player::h_Player(
	const h_WorldPtr& world,
	const h_WorldHeaderPtr& world_header )
	: world_(world)
	, world_header_(world_header)
	, radius_(g_default_player_radius)
	, eyes_level_(g_default_player_eyes_level)
	, height_(g_default_player_height)
	, moving_vector_( 0.0f, 0.0f, 0.0f )
	, pos_( world_header->player.x, world_header->player.y, world_header->player.z )
	, speed_( 0.0f, 0.0f, 0.0f )
	, vertical_speed_(0.0f)
	, is_flying_(false)
	, in_air_(true)
	, water_submerging_(0.0f)
	, eyes_is_underwater_(false)
	, view_angle_( world_header->player.rotation_x, 0.0f, world_header->player.rotation_z )
	, prev_move_time_ms_(0)
	, build_direction_( h_Direction::Unknown )
	, build_block_( h_BlockType::Unknown )
	, player_data_mutex_()
{
	if( pos_.z <= 0.0f )
	{
		// TODO - fetch data from world, place player on ground level
		pos_.z= 125.0f;
	}

	build_pos_= pos_;
	discret_build_pos_[0]= discret_build_pos_[1]= discret_build_pos_[2]= 0;
}

h_Player::~h_Player()
{
	world_header_->player.x= pos_.x;
	world_header_->player.y= pos_.y;
	world_header_->player.z= pos_.z;
	world_header_->player.rotation_x= view_angle_.x;
	world_header_->player.rotation_z= view_angle_.z;
}

void h_Player::SetMovingVector( const m_Vec3& moving_vector )
{
	m_Mat4 mat;
	mat.RotateZ( view_angle_.z );

	moving_vector_= moving_vector * mat;
}

void h_Player::Rotate( const m_Vec3& delta )
{
	std::lock_guard<std::mutex> lock( player_data_mutex_ );
	view_angle_+= delta;

	if( view_angle_.z < 0.0f ) view_angle_.z+= m_Math::two_pi;
	else if( view_angle_.z > m_Math::two_pi ) view_angle_.z-= m_Math::two_pi;

	if( view_angle_.x > m_Math::pi_2 ) view_angle_.x= m_Math::pi_2;
	else if( view_angle_.x < -m_Math::pi_2 ) view_angle_.x= -m_Math::pi_2;
}

void h_Player::ToggleFly()
{
	is_flying_= !is_flying_;
}

void h_Player::Jump()
{
	if( !is_flying_ && !in_air_ )
	{
		vertical_speed_+= g_jump_speed;
		in_air_= true;
	}
}

float h_Player::MinEyesCollidersDistance() const
{
	return
		std::min(
			height_ - eyes_level_,
			radius_ );
}

void h_Player::Tick()
{
	uint64_t current_time_ms = hGetTimeMS();
	if( prev_move_time_ms_ == 0 ) prev_move_time_ms_= current_time_ms;
	float dt_s= float(current_time_ms - prev_move_time_ms_) / 1000.0f;
	prev_move_time_ms_= current_time_ms;

	const float c_min_dt= 1.0f / 128.0f;
	const float c_max_dt= 1.0f / 16.0f;
	dt_s= std::max( c_min_dt, std::min( c_max_dt, dt_s ) );

	const float c_eps= 0.001f;

	bool use_ground_acceleration= !in_air_ || is_flying_;
	float acceleration, deceleration;

	acceleration= use_ground_acceleration ? g_acceleration : g_air_acceleration;
	deceleration= use_ground_acceleration ? g_deceleration : g_air_deceleration;
	acceleration= acceleration * (1.0f - water_submerging_) + g_water_acceleration * water_submerging_;
	deceleration= deceleration * (1.0f - water_submerging_) + g_water_deceleration * water_submerging_;

	m_Vec3 move_delta= moving_vector_;
	if( !is_flying_ ) move_delta.z= 0.0f;

	float move_delta_length= move_delta.Length();
	if( move_delta_length > c_eps )
	{
		m_Vec3 acceleration_vec= acceleration / move_delta_length * move_delta;
		speed_+= acceleration_vec * dt_s;
	}

	float speed_value= speed_.Length();
	float max_speed=
		g_max_underwater_speed_ * water_submerging_ +
		g_max_speed * (1.0f - water_submerging_);
	if( speed_value > max_speed )
		speed_*= max_speed / speed_value;

	if( speed_value > c_eps )
	{
		if( move_delta_length <= c_eps )
		{
			m_Vec3 deceleration_vec= speed_;
			deceleration_vec.Normalize();

			float d_speed= std::min( dt_s * deceleration, speed_value );
			speed_-= deceleration_vec * d_speed;
		}
	}
	else
		speed_= m_Vec3( 0.0f, 0.0f, 0.0f );

	if( !is_flying_ )
	{
		speed_.z= 0.0f;

		float vertical_acceleration=
			g_vertical_water_acceleration * water_submerging_+
			g_vertical_acceleration * (1.0f - water_submerging_);
		float max_vertical_speed=
			g_max_underwater_vertical_speed * water_submerging_ +
			g_max_vertical_speed * (1.0f - water_submerging_);

		vertical_speed_+= ( vertical_acceleration + acceleration * moving_vector_.z * water_submerging_ ) * dt_s;

		if( vertical_speed_ > max_vertical_speed ) vertical_speed_= max_vertical_speed;
		else if( vertical_speed_ < -max_vertical_speed ) vertical_speed_= -max_vertical_speed;
	}
	else
		vertical_speed_= 0.0f;

	p_WorldPhysMeshConstPtr phys_mesh= world_->GetPhysMesh();
	if( !phys_mesh ) return;

	Move( m_Vec3( speed_.xy(), speed_.z + vertical_speed_ ) * dt_s, *phys_mesh );
	UpdateBuildPos( *phys_mesh );
	CheckUnderwater( *phys_mesh );
}

void h_Player::PauseWorldUpdates()
{
	world_->PauseUpdates();
}

void h_Player::UnpauseWorldUpdates()
{
	world_->UnpauseUpdates();
}

void h_Player::SetBuildBlock( h_BlockType block_type )
{
	build_block_= block_type;
}

void h_Player::Build()
{
	if( build_block_ == h_BlockType::Unknown || build_direction_ == h_Direction::Unknown )
		return;

	// Check intersection with builded block.
	float z_min= std::max( float( discret_build_pos_[2]     ), pos_.z );
	float z_max= std::min( float( discret_build_pos_[2] + 1 ), pos_.z + height_ );
	if( z_min < z_max )
	{
		p_UpperBlockFace face( discret_build_pos_[0], discret_build_pos_[1], discret_build_pos_[2], h_Direction::Up );
		if( face.HasCollisionWithCircle( pos_.xy(), radius_ ) )
			return;
	}

	unsigned int sector= (unsigned int)( 6.0f * ( view_angle_.z + m_Math::pi_2 + m_Math::two_pi ) / m_Math::two_pi );
	sector%= 6;
	static const h_Direction c_dir_table[]=
	{
		h_Direction::ForwardRight, h_Direction::Forward, h_Direction::ForwardLeft,
		h_Direction::BackLeft, h_Direction::Back, h_Direction::BackRight,
	};

	h_Direction horizontal_direction= c_dir_table[sector];
	h_Direction vertical_direction= view_angle_.x <= 0.0f ? h_Direction::Up : h_Direction::Down;

	world_->AddBuildEvent(
		discret_build_pos_[0], discret_build_pos_[1], discret_build_pos_[2],
		build_block_,
		horizontal_direction, vertical_direction );
}

void h_Player::Dig()
{
	if( build_direction_ != h_Direction::Unknown )
		world_->AddDestroyEvent( destroy_pos_[0], destroy_pos_[1], destroy_pos_[2] );
}

void h_Player::TestMobSetPosition()
{
	if( build_direction_ != h_Direction::Unknown )
	{
		world_->TestMobSetTargetPosition( discret_build_pos_[0], discret_build_pos_[1], discret_build_pos_[2] );
	}
}

void h_Player::CheckUnderwater( const p_WorldPhysMesh& phys_mesh )
{
	short player_world_space_xy[2];
	pGetHexogonCoord( pos_.xy(), &player_world_space_xy[0], &player_world_space_xy[1] );

	float feet_z= pos_.z;
	float head_z= pos_.z + height_;
	float eyes_z= pos_.z + eyes_level_;
	float blocks_underwater= 0.0f;

	eyes_is_underwater_= false;

	for( const p_WaterBlock& water_block : phys_mesh.water_blocks )
	{
		if( water_block.x == player_world_space_xy[0] &&
			water_block.y == player_world_space_xy[1] )
		{
			float water_min= float(water_block.z);
			float water_max= water_min + water_block.water_level;

			// Take intersection length of two segments on z axis.
			float z_min= std::max( feet_z, water_min );
			float z_max= std::min( head_z, water_max );
			if( z_max > z_min ) blocks_underwater+= z_max - z_min;

			if( eyes_z > water_min && eyes_z < water_max )
				eyes_is_underwater_= true;
		}
	}

	water_submerging_= std::max( 0.0f, std::min( 1.0f, blocks_underwater / height_ ) );
}

void h_Player::UpdateBuildPos( const p_WorldPhysMesh& phys_mesh )
{
	m_Vec3 eye_dir(
		-std::sin( view_angle_.z ) * std::cos( view_angle_.x ),
		+std::cos( view_angle_.z ) * std::cos( view_angle_.x ),
		+std::sin( view_angle_.x ) );

	m_Vec3 eye_pos= pos_;
	eye_pos.z+= eyes_level_;
	float square_dist= std::numeric_limits<float>::max();
	h_Direction block_dir= h_Direction::Unknown;

	m_Vec3 intersect_pos;
	m_Vec3 candidate_pos;
	m_Vec3 n;

	for( const p_UpperBlockFace& face : phys_mesh.upper_block_faces )
	{
		n= g_block_normals[ static_cast<size_t>(face.dir) ];

		// Hexagon triangulation to 4 triangles.
		static const unsigned int traingles_edges[4][3]=
		{
			{ 0, 1, 2 }, { 2, 3, 4, }, { 4, 5, 0 }, { 0, 2, 4 },
		};

		for( unsigned int i= 0; i < 4; i++ )
		{
			m_Vec3 triangle[3];

			triangle[0]= m_Vec3( face.edge[ traingles_edges[i][0] ], face.z );
			triangle[1]= m_Vec3( face.edge[ traingles_edges[i][1] ], face.z );
			triangle[2]= m_Vec3( face.edge[ traingles_edges[i][2] ], face.z );
			if( pRayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
			{
				float candidate_square_dist= ( candidate_pos - eye_pos ).SquareLength();
				if( candidate_square_dist < square_dist )
				{
					square_dist= candidate_square_dist;
					intersect_pos= candidate_pos;
					block_dir= face.dir;
				}
			}
		}
	}

	for( const p_BlockSide& side : phys_mesh.block_sides )
	{
		n= g_block_normals[ static_cast<size_t>(side.dir) ];

		m_Vec3 triangles[6];
		triangles[0]= m_Vec3( side.edge[0], side.z0 );
		triangles[1]= m_Vec3( side.edge[1], side.z0 );
		triangles[2]= m_Vec3( side.edge[1], side.z1 );
		triangles[3]= m_Vec3( side.edge[0], side.z1 );
		triangles[4]= m_Vec3( side.edge[0], side.z0 );
		triangles[5]= m_Vec3( side.edge[1], side.z1 );
		for( unsigned int i= 0; i < 2; i++ )
		{
			if( pRayHasIniersectWithTriangle( triangles + i * 3, n, eye_pos, eye_dir, &candidate_pos ) )
			{
				float candidate_square_dist= ( candidate_pos - eye_pos ).SquareLength();
				if( candidate_square_dist < square_dist )
				{
					square_dist= candidate_square_dist;
					intersect_pos= candidate_pos;
					block_dir= side.dir;
				}
			}
		}
	}

	if( block_dir == h_Direction::Unknown ||
		square_dist > g_max_build_distance * g_max_build_distance )
	{
		build_direction_= h_Direction::Unknown;
		return;
	}

	// Fix accuracy. Move intersection point inside target block.
	intersect_pos-= g_block_normals[ static_cast<size_t>(block_dir) ] * 0.1f;

	pGetHexogonCoord( intersect_pos.xy(), &destroy_pos_[0], &destroy_pos_[1] );
	destroy_pos_[2]= short( intersect_pos.z );

	discret_build_pos_[0]= destroy_pos_[0];
	discret_build_pos_[1]= destroy_pos_[1];
	discret_build_pos_[2]= destroy_pos_[2];

	switch( block_dir )
	{
	case h_Direction::Up: discret_build_pos_[2]++; break;
	case h_Direction::Down: discret_build_pos_[2]--; break;

	case h_Direction::Forward: discret_build_pos_[1]++; break;
	case h_Direction::Back: discret_build_pos_[1]--; break;

	case h_Direction::ForwardRight:
		discret_build_pos_[1]+= ( (discret_build_pos_[0]^1) & 1 );
		discret_build_pos_[0]++;
		break;
	case h_Direction::BackRight:
		discret_build_pos_[1]-= discret_build_pos_[0] & 1;
		discret_build_pos_[0]++;
		break;

	case h_Direction::ForwardLeft:
		discret_build_pos_[1]+= ( (discret_build_pos_[0]^1) & 1 );
		discret_build_pos_[0]--;
		break;
	case h_Direction::BackLeft:
		discret_build_pos_[1]-= discret_build_pos_[0] & 1;
		discret_build_pos_[0]--;
		break;

	case h_Direction::Unknown: H_ASSERT(false); break;
	}

	build_pos_.x= float( discret_build_pos_[0] + 1.0f / 3.0f ) * H_SPACE_SCALE_VECTOR_X;
	build_pos_.y= float( discret_build_pos_[1] ) - 0.5f * float(discret_build_pos_[0]&1) + 0.5f;
	build_pos_.z= float( discret_build_pos_[2] );

	build_direction_= block_dir;
}

void h_Player::Move( const m_Vec3& delta, const p_WorldPhysMesh& phys_mesh )
{
	const float c_eps= 0.00001f;
	const float c_vertical_collision_eps= 0.001f;
	const float c_on_ground_eps= 0.01f;

	/* Reduce player radius for vertical collisions.
	 * This approach fix bug, when player can stay on block,
	 * where he can`t stay by logic of human, but stay, beacause calculation errors.
	*/
	const float c_vertical_collision_player_radius= radius_ * 0.9f;

	m_Vec3 new_pos= pos_ + delta;

	for( const p_UpperBlockFace& face : phys_mesh.upper_block_faces )
	{
		if( delta.z > c_eps )
		{
			if( face.dir == h_Direction::Down &&
				face.z >= (pos_.z + height_) &&
				face.z < (new_pos.z + height_) &&
				face.HasCollisionWithCircle( new_pos.xy(), c_vertical_collision_player_radius ) )
				{
					new_pos.z= face.z - height_ - c_vertical_collision_eps;
					break;
				}
		}
		else if( delta.z < -c_eps )
		{
			if( face.dir == h_Direction::Up &&
				face.z <= pos_.z &&
				face.z > new_pos.z &&
				face.HasCollisionWithCircle( new_pos.xy(), c_vertical_collision_player_radius ) )
			{
				new_pos.z= face.z + c_vertical_collision_eps;
				break;
			}
		}
	}// upeer faces

	for( const p_BlockSide& side : phys_mesh.block_sides )
	{
		if( ( side.z0 > new_pos.z && side.z0 < new_pos.z + height_ ) ||
			( side.z1 > new_pos.z && side.z1 < new_pos.z + height_ ) )
		{
			m_Vec2 collide_pos= side.CollideWithCirlce( new_pos.xy(), radius_ );
			if( collide_pos != new_pos.xy() )
			{
				new_pos.x= collide_pos.x;
				new_pos.y= collide_pos.y;

				// Zero speed component, perpendicalar to this side, but only if speed vector directed to block.
				const m_Vec3& normal= g_block_normals[ static_cast<size_t>(side.dir) ];
				speed_-= std::min( 0.0f, speed_ * normal ) * normal;
			}
		}
	}

	// Check in_air
	in_air_= true;
	for( const p_UpperBlockFace& face : phys_mesh.upper_block_faces )
	{
		if( face.dir == h_Direction::Up &&
			new_pos.z <= face.z + c_on_ground_eps &&
			new_pos.z > face.z &&
			face.HasCollisionWithCircle( new_pos.xy(), c_vertical_collision_player_radius ) )
		{
			in_air_= false;
			vertical_speed_= 0.0f;
			break;
		}
		if( face.dir == h_Direction::Down &&
			new_pos.z + height_ >= face.z - c_on_ground_eps &&
			new_pos.z + height_ < face.z &&
			face.HasCollisionWithCircle( new_pos.xy(), c_vertical_collision_player_radius ) )
		{
			vertical_speed_= 0.0f;
			break;
		}
	}

	// Modify pos_ only under mutex.
	std::lock_guard<std::mutex> lock( player_data_mutex_ );
	pos_= new_pos;
}
