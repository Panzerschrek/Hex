#include "player.hpp"
#include "block_collision.hpp"
#include "world.hpp"

h_Player::h_Player( const h_WorldPtr& world  )
	: world_(world)
	, pos_()
	, view_angle_( 0.0f, 0.0f, 0.0f )
	, player_data_mutex_()
	, phys_mesh_()
{
	pos_.x= ( world->Longitude() + world->ChunkNumberX()/2 ) * H_SPACE_SCALE_VECTOR_X * float( H_CHUNK_WIDTH );
	pos_.y= ( world->Latitude() + world->ChunkNumberY()/2 ) * H_SPACE_SCALE_VECTOR_Y  * float( H_CHUNK_WIDTH );
	pos_.z= float(H_CHUNK_HEIGHT/2 + 10);
}

h_Player::~h_Player()
{
}

void h_Player::SetCollisionMesh( h_ChunkPhysMesh* mesh )
{
	phys_mesh_= *mesh;
}

h_Direction h_Player::GetBuildPos( short* x, short* y, short* z )  const
{
	m_Vec3 eye_dir(
	-sin( view_angle_.z ) * cos( view_angle_.x ),
	 cos( view_angle_.z ) * cos( view_angle_.x ),
	 sin( view_angle_.x ) );

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
		m_Vec3( 0.866025f, 0.5f, 0.0 ),  m_Vec3( -0.866025f, -0.5f, 0.0 ),
		m_Vec3( -0.866025f, 0.5f, 0.0 ), m_Vec3( 0.866025f, -0.5f, 0.0 ),
		m_Vec3( 0.0f, 0.0f, 1.0f ), 	 m_Vec3( 0.0f, 0.0f, -1.0f ),
		m_Vec3( 100500.0f, 100500.0f, 100500.0f )
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

	intersect_pos+= normals[ block_dir ] *  0.01f;

	short new_x, new_y, new_z;
	GetHexogonCoord( intersect_pos.xy(), &new_x, &new_y );
	new_z= (short) intersect_pos.z;
	new_z++;

	if( block_dir != DIRECTION_UNKNOWN )
	{
		*x= new_x;
		*y= new_y;
		*z= new_z;
	}
	else
		*x = *y= *z= -1;

	return block_dir;
}

void h_Player::Move( const m_Vec3& delta )
{
	m_Vec3 new_pos= pos_ + delta;

	p_UpperBlockFace* face;
	p_BlockSide* side;
	unsigned int count;

	face= phys_mesh_.upper_block_faces.Data();
	count= phys_mesh_.upper_block_faces.Size();
	for( unsigned int k= 0; k< count; k++, face++ )
	{
		if( delta.z > 0.00001f )
		{
			if( face->dir == DOWN )
				if( face->z > (pos_.z+H_PLAYER_HEIGHT) && face->z < (new_pos.z+H_PLAYER_HEIGHT) )
				{
					if(  face->HasCollisionWithCircle( new_pos.xy(), H_PLAYER_RADIUS ) )
						//new_pos.z= face->z  - 0.0625f * delta.z;
						new_pos.z= face->z- H_PLAYER_HEIGHT - 0.001f;
				}
		}
		else if( delta.z < -0.000001f )
		{
			if( face->dir == UP )
				if( face->z < pos_.z && face->z > new_pos.z )
				{
					if( face->HasCollisionWithCircle( new_pos.xy(), H_PLAYER_RADIUS ) )
						new_pos.z= face->z  + 0.0001f;
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
