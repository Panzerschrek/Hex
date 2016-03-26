#include <cmath>
#include <limits>

#include "math_lib/assert.hpp"

#include "block_collision.hpp"

static bool IsPointInTriangle( const m_Vec2* triangle, const m_Vec2& point )
{
	float cross_z[3];

	cross_z[0]= mVec2Cross( point - triangle[0], triangle[1] - triangle[0] );
	cross_z[1]= mVec2Cross( point - triangle[1], triangle[2] - triangle[1] );
	cross_z[2]= mVec2Cross( point - triangle[2], triangle[0] - triangle[2] );

	float dot[2];
	dot[0]= cross_z[0] * cross_z[1];
	dot[1]= cross_z[0] * cross_z[2];

	return dot[0] >= 0.0f && dot[1] >= 0.0f;
}

static bool IsPointInTriangle( const m_Vec3* triangle, const m_Vec3& point )
{
	m_Vec3 cross[3];

	cross[0]= mVec3Cross( point - triangle[0], triangle[1] - triangle[0] );
	cross[1]= mVec3Cross( point - triangle[1], triangle[2] - triangle[1] );
	cross[2]= mVec3Cross( point - triangle[2], triangle[0] - triangle[2] );

	float dot[2];
	dot[0]= cross[0] * cross[1];
	dot[1]= cross[0] * cross[2];

	return dot[0] >= 0.0f && dot[1] >= 0.0f;
}

static bool TriangleIntersectWithCircle( const m_Vec2* triangle, const m_Vec2& center, float radius )
{
	if( IsPointInTriangle( triangle, center ) )
		return true;

	float square_radius= radius * radius;

	// If triangle vertex is inside circle
	for( unsigned int i= 0; i < 3; i++ )
		if( (triangle[i] - center).SquareLength() <= square_radius )
			return true;

	// Check distance from circle to triangle edges
	for( unsigned int i= 0; i < 3; i++ )
	{
		const m_Vec2& v0= triangle[i];
		const m_Vec2& v1= triangle[ i == 2 ? 0 : i + 1 ];

		m_Vec2 edge_vec= v1 - v0;
		m_Vec2 v0_to_pos_vec= center - v0;

		float edge_square_length= edge_vec * edge_vec;
		float dot= edge_vec * v0_to_pos_vec;

		float square_dist_to_edge;
		if( dot < 0.0f )
			square_dist_to_edge= v0_to_pos_vec.SquareLength();
		else if( dot > edge_square_length )
			square_dist_to_edge= (center - v1).SquareLength();
		else
			square_dist_to_edge= (v0_to_pos_vec - edge_vec * (dot / edge_square_length)).SquareLength();

		if( square_dist_to_edge <= square_radius )
			return true;
	}

	return false;
}

/*
------------------p_UpperBlockFace--------------------
*/

p_UpperBlockFace::p_UpperBlockFace( short x, short y, float in_z, h_Direction in_dir )
	: z( float(in_z) )
	, dir(in_dir)
{
	H_ASSERT( in_dir == h_Direction::Up || in_dir == h_Direction::Down );

	SetupEdges( x, y );
}

bool p_UpperBlockFace::HasCollisionWithCircle( const m_Vec2& pos, float radius ) const
{
	m_Vec2 center
	(
		edge[0].x + H_HEXAGON_EDGE_SIZE,
		edge[0].y
	);

	float dst= ( pos - center ).Length();

	// Not hit outer circle. Has no collision.
	if( dst >= radius + H_HEXAGON_EDGE_SIZE )
		return false;
	// Hit inner circle. Has collision.
	if( dst <= radius + H_HEXAGON_INNER_RADIUS )
		return true;

	// Check collision with hexagon traigles
	m_Vec2 triangle[3];
	triangle[0]= center;
	for( unsigned int i= 0; i < 6; i++ )
	{
		triangle[1]= edge[i];
		triangle[2]= edge[ i == 5 ? 0 : i + 1 ];

		if( TriangleIntersectWithCircle( triangle, pos, radius ) )
			return true;
	}
	return false;
}

void p_UpperBlockFace::SetupEdges( short x, short y )
{
	edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X;
	edge[1].x= edge[5].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
	edge[2].x= edge[4].x= edge[0].x + 1.5f * H_HEXAGON_EDGE_SIZE;
	edge[3].x= edge[0].x + 2.0f * H_HEXAGON_EDGE_SIZE;

	edge[0].y= edge[3].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
	edge[1].y= edge[2].y= edge[0].y + 0.5f;
	edge[4].y= edge[5].y= edge[0].y - 0.5f;
}

/*
------------------p_BlockSide--------------------
*/

p_BlockSide::p_BlockSide( short x, short y, short in_z, h_Direction in_dir )
	: z0(in_z)
	, z1(in_z + 1.0f)
	, dir(in_dir)
{
	SetupEdge( x, y );
}

p_BlockSide::p_BlockSide( short x, short y, float in_z0, float in_z1, h_Direction in_dir )
	: z0(in_z0)
	, z1(in_z1)
	, dir(in_dir)
{
	SetupEdge( x, y );
}

bool p_BlockSide::HasCollisionWithCircle( const m_Vec2& pos, float radius ) const
{
	m_Vec2 edge_vec= edge[0]- edge[1];
	edge_vec.Normalize();

	m_Vec2 vec_to_circle= pos - edge[0];

	m_Vec2 vec_projection= ( vec_to_circle * edge_vec ) * edge_vec;
	m_Vec2 nearest_vec_to_edge= vec_to_circle - vec_projection;
	if( nearest_vec_to_edge.Length() < radius )// if distance to line less than circle radius
	{
		float r2= radius * radius;
		if( ( pos - edge[0] ).SquareLength() < r2 )
			return true;
		if( ( pos - edge[1] ).SquareLength() < r2 )
			return true;

		m_Vec2 nearest_point_on_edge= pos - nearest_vec_to_edge;
		m_Vec2 v[2]= {  edge[0] - nearest_point_on_edge, edge[1] - nearest_point_on_edge };
		if( v[0] * v[1] < 0.0f )
			return true;
		else return false;
	}
	else return false;
}

m_Vec2 p_BlockSide::CollideWithCirlce( const m_Vec2& pos, float radius ) const
{
	m_Vec2 edge_vec= edge[0]- edge[1];
	edge_vec.Normalize();

	m_Vec2 vec_to_circle= pos - edge[0];

	m_Vec2 vec_projection= ( vec_to_circle * edge_vec ) * edge_vec;
	m_Vec2 nearest_vec_to_edge= vec_to_circle - vec_projection;// || normal
	if( nearest_vec_to_edge.Length() < radius )// if distance to line less than circle radius
	{
		m_Vec2 nearest_point_on_edge= vec_projection + edge[0];
		m_Vec2 v[2]= { edge[0] - nearest_point_on_edge, edge[1] - nearest_point_on_edge };
		if( v[0] * v[1] < 0.0f )// collision with line
		{
			nearest_vec_to_edge.Normalize();
			return nearest_point_on_edge +  nearest_vec_to_edge * radius;
		}

		//collision with points
		float r2= radius * radius;
		m_Vec2 to_edge_vec= pos - edge[0];
		if( to_edge_vec.SquareLength() < r2 )
		{
			to_edge_vec.Normalize();
			return edge[0] + to_edge_vec * radius;
		}
		to_edge_vec= pos - edge[1];
		if( to_edge_vec.SquareLength() < r2 )
		{
			to_edge_vec.Normalize();
			return edge[1] + to_edge_vec * radius;
		}

	}
	return pos;
}

void p_BlockSide::SetupEdge( short x, short y )
{
	switch( dir )
	{
	case h_Direction::Forward:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + 1.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x - H_HEXAGON_EDGE_SIZE;
		edge[0].y= edge[1].y= float(y) + 0.5f * float( (x+1)&1 ) + 1.0f;
		break;

	case h_Direction::Back:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x + H_HEXAGON_EDGE_SIZE;
		edge[0].y= edge[1].y= float(y) + 0.5f * float( (x+1)&1 );
		break;

	case h_Direction::ForwardRight:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE * 2.0f;
		edge[1].x= edge[0].x - 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
		edge[1].y= edge[0].y + 0.5f;
		break;

	case h_Direction::BackLeft:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X;
		edge[1].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
		edge[1].y= edge[0].y - 0.5f;
		break;

	case h_Direction::ForwardLeft:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE * 0.5f;
		edge[1].x= edge[0].x - 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 1.0f;
		edge[1].y= edge[0].y - 0.5f;
		break;

	case h_Direction::BackRight:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X  + 1.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 );
		edge[1].y= edge[0].y + 0.5f;
		break;

	default:
		H_ASSERT(false);
		break;
	}
}

bool pRayHasIniersectWithTriangle(
	const m_Vec3* triangle, const m_Vec3& normal,
	const m_Vec3& point, const m_Vec3& dir,
	m_Vec3* intersect_pos )
{
	m_Vec3 vec_to_plane= triangle[0] - point;
	float dst_to_plane= vec_to_plane * normal;

	float e= normal * dir;
	if( e == 0.0f )
		return false;

	m_Vec3 dir_vec_to_plane= dir * ( dst_to_plane / e );
	if( dir_vec_to_plane * dir <= 0.0f )
		return false;
	m_Vec3 intersect_point= point + dir_vec_to_plane;

	m_Vec3 cross[3], tmp_v1, tmp_v2;

	tmp_v1= intersect_point - triangle[0];
	tmp_v2= triangle[1]- triangle[0];
	cross[0]= mVec3Cross( tmp_v1, tmp_v2 );
	tmp_v1= intersect_point - triangle[1];
	tmp_v2= triangle[2]- triangle[1];
	cross[1]= mVec3Cross( tmp_v1, tmp_v2 );
	tmp_v1= intersect_point - triangle[2];
	tmp_v2= triangle[0]- triangle[2];
	cross[2]= mVec3Cross( tmp_v1, tmp_v2 );

	float dot[2];
	dot[0]= cross[0] * cross[1];
	dot[1]= cross[0] * cross[2];
	if( dot[0] >= 0.0f && dot[1] >= 0.0f  )
	{
		*intersect_pos= intersect_point;
		return true;
	}
	else return false;
}

void pGetHexogonCoord( const m_Vec2& pos, short* x, short* y )
{
	float transformed_x= pos.x / H_SPACE_SCALE_VECTOR_X;
	float floor_x= std::floor( transformed_x );
	short nearest_x= short( floor_x );
	float dx= transformed_x - floor_x;

	float transformed_y= pos.y - 0.5f * float( (nearest_x^1) & 1 );
	float floor_y= std::floor( transformed_y );
	short nearest_y= short( floor_y );
	float dy= transformed_y - floor_y;

	// Upper part   y=  0.5 + 1.5 * x
	// Lower part   y=  0.5 - 1.5 * x
	/*
	_____________
	|  /         |\
	| /          | \
	|/           |  \
	|\           |  /
	| \          | /
	|__\_________|/
	*/
	if( dy > 0.5f + 1.5f * dx )
	{
		*x= nearest_x - 1;
		*y= nearest_y + ((nearest_x^1)&1);
	}
	else
	if( dy < 0.5f - 1.5f * dx )
	{
		*x= nearest_x - 1;
		*y= nearest_y - (nearest_x&1);
	}
	else
	{
		*x= nearest_x;
		*y= nearest_y;
	}
}
