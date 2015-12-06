#include "player.hpp"
#include "block_collision.hpp"
#include "world.hpp"

h_Player::h_Player( const h_WorldPtr& world  )
	: world_(world)
	, pos_()
	, view_angle_( 0.0f, 0.0f, 0.0f )
	, build_direction_( DIRECTION_UNKNOWN )
	, build_block_( BLOCK_UNKNOWN )
	, player_data_mutex_()
	, phys_mesh_()
{
	pos_.x= ( world->Longitude() + world->ChunkNumberX()/2 ) * H_SPACE_SCALE_VECTOR_X * float( H_CHUNK_WIDTH );
	pos_.y= ( world->Latitude() + world->ChunkNumberY()/2 ) * H_SPACE_SCALE_VECTOR_Y  * float( H_CHUNK_WIDTH );
	pos_.z= float(H_CHUNK_HEIGHT/2 + 10);

	build_pos_= pos_;
	discret_build_pos_[0]= discret_build_pos_[1]= discret_build_pos_[2]= 0;
}

h_Player::~h_Player()
{
}

void h_Player::SetCollisionMesh( h_ChunkPhysMesh* mesh )
{
	phys_mesh_= *mesh;
}

void h_Player::UpdateBuildPos()
{
	m_Vec3 eye_dir(
		-std::sin( view_angle_.z ) * std::cos( view_angle_.x ),
		+std::cos( view_angle_.z ) * std::cos( view_angle_.x ),
		+std::sin( view_angle_.x ) );

	m_Vec3 eye_pos= pos_;
	eye_pos.z+= H_PLAYER_EYE_LEVEL;
	float dst= 1024.0f;
	h_Direction block_dir= DIRECTION_UNKNOWN;

	m_Vec3 intersect_pos;
	p_UpperBlockFace* face;
	p_BlockSide* side;
	unsigned int count;

	static const m_Vec3 normals[9]=
	{
		m_Vec3( 0.0f, 1.0f, 0.0f ),      m_Vec3( 0.0f, -1.0f, 0.0f ),
		m_Vec3( +H_SPACE_SCALE_VECTOR_X, 0.5f, 0.0 ), m_Vec3( -H_SPACE_SCALE_VECTOR_X, -0.5f, 0.0 ),
		m_Vec3( -H_SPACE_SCALE_VECTOR_X, 0.5f, 0.0 ), m_Vec3( +H_SPACE_SCALE_VECTOR_X, -0.5f, 0.0 ),
		m_Vec3( 0.0f, 0.0f, 1.0f ),      m_Vec3( 0.0f, 0.0f, -1.0f )
	};

	m_Vec3 candidate_pos;
	m_Vec3 triangle[3];
	m_Vec3 n;

	face= phys_mesh_.upper_block_faces.Data();
	count= phys_mesh_.upper_block_faces.Size();
	for( unsigned int k= 0; k< count; k++, face++ )
	{
		n= normals[ face->dir ];

		triangle[0]= m_Vec3( face->edge[0].x, face->edge[0].y, face->z );
		triangle[1]= m_Vec3( face->edge[1].x, face->edge[1].y, face->z );
		triangle[2]= m_Vec3( face->edge[2].x, face->edge[2].y, face->z );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= face->dir;
			}
		}

		triangle[0]= m_Vec3( face->edge[2].x, face->edge[2].y, face->z );
		triangle[1]= m_Vec3( face->edge[3].x, face->edge[3].y, face->z );
		triangle[2]= m_Vec3( face->edge[4].x, face->edge[4].y, face->z );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= face->dir;
			}
		}

		triangle[0]= m_Vec3( face->edge[4].x, face->edge[4].y, face->z );
		triangle[1]= m_Vec3( face->edge[5].x, face->edge[5].y, face->z );
		triangle[2]= m_Vec3( face->edge[0].x, face->edge[0].y, face->z );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= face->dir;
			}
		}

		triangle[0]= m_Vec3( face->edge[0].x, face->edge[0].y, face->z );
		triangle[1]= m_Vec3( face->edge[2].x, face->edge[2].y, face->z );
		triangle[2]= m_Vec3( face->edge[4].x, face->edge[4].y, face->z );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= face->dir;
			}
		}
	}

	side= phys_mesh_.block_sides.Data();
	count= phys_mesh_.block_sides.Size();
	for( unsigned int k= 0; k< count; k++, side++ )
	{
		n= normals[ side->dir ];

		triangle[0]= m_Vec3( side->edge[0].x, side->edge[0].y, side->z );
		triangle[1]= m_Vec3( side->edge[1].x, side->edge[1].y, side->z );
		triangle[2]= m_Vec3( side->edge[1].x, side->edge[1].y, side->z + 1.0f );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= side->dir;
			}
		}

		triangle[0]= m_Vec3( side->edge[0].x, side->edge[0].y, side->z + 1.0f );
		triangle[1]= m_Vec3( side->edge[0].x, side->edge[0].y, side->z );
		triangle[2]= m_Vec3( side->edge[1].x, side->edge[1].y, side->z + 1.0f );
		if( RayHasIniersectWithTriangle( triangle, n, eye_pos, eye_dir, &candidate_pos ) )
		{
			float candidate_dst= ( candidate_pos - eye_pos ).Length();
			if( candidate_dst < dst )
			{
				dst= candidate_dst;
				intersect_pos= candidate_pos;
				block_dir= side->dir;
			}
		}
	}

	if( block_dir == DIRECTION_UNKNOWN ) return;

	intersect_pos+= normals[ block_dir ] * 0.01f;

	short new_x, new_y, new_z;
	GetHexogonCoord( intersect_pos.xy(), &new_x, &new_y );
	new_z= (short) intersect_pos.z;
	new_z++;

	discret_build_pos_[0]= new_x;
	discret_build_pos_[1]= new_y;
	discret_build_pos_[2]= new_z;

	build_direction_= block_dir;
}

void h_Player::Move( const m_Vec3& delta )
{
	const float c_eps= 0.00001f;
	const float c_vertical_collision_eps= 0.001f;

	m_Vec3 new_pos= pos_ + delta;

	const p_UpperBlockFace* face;
	const p_BlockSide* side;
	unsigned int count;

	face= phys_mesh_.upper_block_faces.Data();
	count= phys_mesh_.upper_block_faces.Size();
	for( unsigned int k= 0; k< count; k++, face++ )
	{
		if( delta.z > c_eps )
		{
			if( face->dir == DOWN )
				if( face->z >= (pos_.z + H_PLAYER_HEIGHT) && face->z < (new_pos.z + H_PLAYER_HEIGHT) )
				{
					if( face->HasCollisionWithCircle( new_pos.xy(), H_PLAYER_RADIUS ) )
						new_pos.z= face->z - H_PLAYER_HEIGHT - c_vertical_collision_eps;
				}
		}
		else if( delta.z < -c_eps )
		{
			if( face->dir == UP )
				if( face->z <= pos_.z && face->z > new_pos.z )
				{
					if( face->HasCollisionWithCircle( new_pos.xy(), H_PLAYER_RADIUS ) )
						new_pos.z= face->z + c_vertical_collision_eps;
				}
		}
	}// upeer faces

	side= phys_mesh_.block_sides.Data();
	count= phys_mesh_.block_sides.Size();
	for( unsigned int k= 0; k< count; k++, side++ )
	{
		if( ( side->z > new_pos.z && side->z < new_pos.z + H_PLAYER_HEIGHT ) ||
			( side->z + 1.0f > new_pos.z && side->z + 1.0f < new_pos.z + H_PLAYER_HEIGHT ) )
		{
			m_Vec2 collide_pos= side->CollideWithCirlce( new_pos.xy(), H_PLAYER_RADIUS );
			new_pos.x= collide_pos.x;
			new_pos.y= collide_pos.y;
		}
	}

	pos_= new_pos;
}

void h_Player::Rotate( const m_Vec3& delta )
{
	view_angle_+= delta;

	if( view_angle_.z < 0.0f ) view_angle_.z+= m_Math::FM_2PI;
	else if( view_angle_.z > m_Math::FM_2PI ) view_angle_.z-= m_Math::FM_2PI;

	if( view_angle_.x > m_Math::FM_PI2 ) view_angle_.x= m_Math::FM_PI2;
	else if( view_angle_.x < -m_Math::FM_PI2 ) view_angle_.x= -m_Math::FM_PI2;
}

void h_Player::SetBuildBlock( h_BlockType block_type )
{
	build_block_= block_type;
}

void h_Player::Tick()
{
	UpdateBuildPos();

	build_pos_.x= float( discret_build_pos_[0] + 1.0f / 3.0f ) * H_SPACE_SCALE_VECTOR_X;
	build_pos_.y= float( discret_build_pos_[1] ) - 0.5f * float(discret_build_pos_[0]&1) + 0.5f;
	build_pos_.z= float( discret_build_pos_[2] ) - 1.0f;
}

void h_Player::Build()
{
	if( build_block_ != BLOCK_UNKNOWN )
	{
		world_->AddBuildEvent(
			discret_build_pos_[0] - world_->Longitude() * H_CHUNK_WIDTH,
			discret_build_pos_[1] - world_->Latitude () * H_CHUNK_WIDTH,
			discret_build_pos_[2], build_block_ );
	}
}

void h_Player::Dig()
{
	if( build_direction_ != DIRECTION_UNKNOWN )
	{
		short dig_pos[3]=
		{
			discret_build_pos_[0],
			discret_build_pos_[1],
			discret_build_pos_[2]
		};

		switch( build_direction_ )
		{
		case UP:
			dig_pos[2]--;
			break;
		case DOWN:
			dig_pos[2]++;
			break;

		case FORWARD:
			dig_pos[1]--;
			break;
		case BACK:
			dig_pos[1]++;
			break;

		case FORWARD_RIGHT:
			dig_pos[1]-= (dig_pos[0]&1);
			dig_pos[0]--;
			break;
		case BACK_RIGHT:
			dig_pos[1]+= ((dig_pos[0]+1)&1);
			dig_pos[0]--;
			break;

		case FORWARD_LEFT:
			dig_pos[1]-= (dig_pos[0]&1);
			dig_pos[0]++;
			break;
		case BACK_LEFT:
			dig_pos[1]+= ((dig_pos[0]+1)&1);
			dig_pos[0]++;
			break;

		default: break;
		};

		world_->AddDestroyEvent(
			dig_pos[0] - world_->Longitude() * H_CHUNK_WIDTH,
			dig_pos[1] - world_->Latitude () * H_CHUNK_WIDTH,
			dig_pos[2] );
	}
}
