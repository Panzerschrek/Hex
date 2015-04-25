#include "block_collision.hpp"

void GetHexogonCoord( m_Vec2 pos, short* x, short* y )
{
	short nearest_x, nearest_y;

	nearest_x= short( floor( pos.x / H_SPACE_SCALE_VECTOR_X ) );
	nearest_y= short( floor( pos.y  - 0.5f * float((nearest_x+1)&1) )  );

	short candidate_cells[]= {
		nearest_x, nearest_y,
		short(nearest_x - 1), short( nearest_y - (nearest_x&1 )),//lower left
		short(nearest_x - 1), short( nearest_y + ((nearest_x+1)&1) )
	};//upper left

	m_Vec2 center_pos[3];
	center_pos[0].x= float( nearest_x ) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE;
	center_pos[0].y= float( nearest_y ) + 0.5f * ((nearest_x+1)&1) + 0.5f;

	center_pos[1].x= center_pos[2].x= center_pos[0].x - H_SPACE_SCALE_VECTOR_X;
	center_pos[1].y= center_pos[0].y - 0.5f;
	center_pos[2].y= center_pos[0].y + 0.5f;

	unsigned int nearest_cell= 0;
	float dst_min= 16.0f;
	for( unsigned int i= 0; i< 3; i++ )
	{
		float dst= ( center_pos[i] - pos ).SquareLength();
		if( dst < dst_min )
		{
			dst_min= dst;
			nearest_cell= i;
		}
	}

	*x= candidate_cells[nearest_cell*2];
	*y= candidate_cells[nearest_cell*2 + 1];
	return;
}


bool RayHasIniersectWithTriangle( m_Vec3* triangle, m_Vec3& normal, m_Vec3& point, m_Vec3& dir, m_Vec3* intersect_pos )
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


bool IsPointInTriangle( m_Vec2* triangle, m_Vec2 point )
{
	float cross_z[3];
	m_Vec2 tmp_v1, tmp_v2;

	tmp_v1= point - triangle[0];
	tmp_v2= triangle[1] - triangle[0];
	cross_z[0]= tmp_v1.x * tmp_v2.y - tmp_v1.y * tmp_v2.x;
	tmp_v1= point - triangle[1];
	tmp_v2= triangle[2] - triangle[1];
	cross_z[1]= tmp_v1.x * tmp_v2.y - tmp_v1.y * tmp_v2.x;
	tmp_v1= point - triangle[2];
	tmp_v2= triangle[0] - triangle[2];
	cross_z[2]= tmp_v1.x * tmp_v2.y - tmp_v1.y * tmp_v2.x;

	float dot[2];
	dot[0]= cross_z[0] * cross_z[1];
	dot[1]= cross_z[0] * cross_z[2];

	return dot[0] >= 0.0f && dot[1] >= 0.0f;
}

bool IsPointInTriangle( m_Vec3* triangle, m_Vec3 point )
{
	m_Vec3 cross[3];
	m_Vec3 tmp_v1, tmp_v2;

	tmp_v1= point - triangle[0];
	tmp_v2= triangle[1] - triangle[0];
	cross[0]= mVec3Cross( tmp_v1, tmp_v2 );
	tmp_v1= point - triangle[1];
	tmp_v2= triangle[2] - triangle[1];
	cross[1]= mVec3Cross( tmp_v1, tmp_v2 );
	tmp_v1= point - triangle[2];
	tmp_v2= triangle[0] - triangle[2];
	cross[2]= mVec3Cross( tmp_v1, tmp_v2 );

	float dot[2];
	dot[0]= cross[0] * cross[1];
	dot[1]= cross[0] * cross[2];

	return dot[0] >= 0.0f && dot[1] >= 0.0f;
}


void p_UpperBlockFace::Gen( short x, short y, short z, h_Direction dir )
{
	this->z= float(z);
	this->dir= dir;
	if( dir == DOWN )
		this->z -= 1.0f;

	edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X;
	edge[1].x= edge[5].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
	edge[2].x= edge[4].x= edge[0].x + 1.5f * H_HEXAGON_EDGE_SIZE;
	edge[3].x= edge[0].x + 2.0f * H_HEXAGON_EDGE_SIZE;

	edge[0].y= edge[3].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
	edge[1].y= edge[2].y= edge[0].y + 0.5f;
	edge[4].y= edge[5].y= edge[0].y - 0.5f;
}

bool p_UpperBlockFace::HasCollisionWithCircle( m_Vec2 pos, float radius ) const
{
	m_Vec2 center
	(
		edge[0].x + H_HEXAGON_EDGE_SIZE,
		edge[0].y
	);

	float dst= ( pos - center ).Length();

	if( dst  < radius + H_HEXAGON_EDGE_SIZE )
		return true;
	else return false;//temporal hack - replase hexagon by outer circle


	if( dst < 0.5f )// if center of circle inside inner hexagon cirlce
		return true;
	else if( dst > radius + H_HEXAGON_EDGE_SIZE )
		return false;

	//calculate distance from center of circle to vertices of hexagon
	for( unsigned int i= 0; i< 6; i++ )
	{
		/*float dst_to_edge;
		m_Vec2 v= pos - edge[i];
		m_Vec2 n= edge[(i+1)%6] - edge[i];
		n.Normalize();
		m_Vec2 v2= float( n * v ) * n;
		m_Vec2 vec_to_edge= v - v2;*/
		if( ( edge[i] - pos ).Length() < radius )
			return true;
	}

	return false;
}

void p_BlockSide::Gen( short x, short y, short z, h_Direction dir )
{
	this->z= float(z);
	this->dir= dir;

	switch( dir )
	{
	case FORWARD:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + 1.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x - H_HEXAGON_EDGE_SIZE;
		edge[0].y= edge[1].y= float(y) + 0.5f * float( (x+1)&1 ) + 1.0f;
		break;

	case BACK:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x + H_HEXAGON_EDGE_SIZE;
		edge[0].y= edge[1].y= float(y) + 0.5f * float( (x+1)&1 );
		break;

	case FORWARD_RIGHT:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE * 2.0f;
		edge[1].x= edge[0].x - 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
		edge[1].y= edge[0].y + 0.5f;
		break;

	case BACK_LEFT:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X;
		edge[1].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 0.5f;
		edge[1].y= edge[0].y - 0.5f;
		break;

	case FORWARD_LEFT:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE * 0.5f;
		edge[1].x= edge[0].x - 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 ) + 1.0f;
		edge[1].y= edge[0].y - 0.5f;
		break;

	case BACK_RIGHT:
		edge[0].x= float(x) * H_SPACE_SCALE_VECTOR_X  + 1.5f * H_HEXAGON_EDGE_SIZE;
		edge[1].x= edge[0].x + 0.5f * H_HEXAGON_EDGE_SIZE;
		edge[0].y= float(y) + 0.5f * float( (x+1)&1 );
		edge[1].y= edge[0].y + 0.5f;
		break;

	}
}


bool p_BlockSide::HasCollisionWithCircle( m_Vec2 pos,  float radius ) const
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

m_Vec2 p_BlockSide::CollideWithCirlce( m_Vec2 pos, float radius ) const
{
	m_Vec2 edge_vec= edge[0]- edge[1];
	edge_vec.Normalize();

	m_Vec2 vec_to_circle= pos - edge[0];

	m_Vec2 vec_projection= ( vec_to_circle * edge_vec ) * edge_vec;
	m_Vec2 nearest_vec_to_edge= vec_to_circle - vec_projection;// || normal
	if( nearest_vec_to_edge.Length() < radius )// if distance to line less than circle radius
	{
		m_Vec2 nearest_point_on_edge= vec_projection + edge[0];
		m_Vec2 v[2]= {  edge[0] - nearest_point_on_edge, edge[1] - nearest_point_on_edge };
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
