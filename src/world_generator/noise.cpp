#include "noise.hpp"

/*static int Noise2( int x, int y )
{
	int n = x + y * 57;
	n = (n << 13) ^ n;

	return ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff ) >> 15;
}*/

// returns value in range [0; 65536)
int g_Noise2( int x, int y, int seed )
{
	const int X_NOISE_GEN   =  1619;
	const int Y_NOISE_GEN   = 31337;
	const int Z_NOISE_GEN   =  6971;
	const int SEED_NOISE_GEN=  1013;

	int n= (
		X_NOISE_GEN * x +
		Y_NOISE_GEN * y +
		Z_NOISE_GEN * 0 +
		SEED_NOISE_GEN * seed )
		& 0x7fffffff;

	n= ( n >> 13 ) ^ n;
	return ( ( n * ( n * n * 60493 + 19990303 ) + 1376312589 ) & 0x7fffffff ) >> 15;
}

// returns value in range [0; 65536)
int g_InterpolatedNoise( int x, int y, int seed, int shift )
{
	int X= x>>shift, Y= y>>shift;
	int shift_pow2= 1 << shift;
	int mask= shift_pow2 - 1;

	int dx= x & mask, dy= y & mask;
	int dy1= shift_pow2 - dy;

	int noise[]=
	{
		g_Noise2(X    , Y    , seed ),
		g_Noise2(X + 1, Y    , seed ),
		g_Noise2(X + 1, Y + 1, seed ),
		g_Noise2(X    , Y + 1, seed )
	};

	int interp_x[]=
	{
		noise[3] * dy + noise[0] * dy1,
		noise[2] * dy + noise[1] * dy1
	};

	return ( interp_x[1] * dx + interp_x[0] * (shift_pow2 - dx) ) >> (shift + shift);
}

int g_TriangularInterpolatedNoise( int x, int y, int seed, int shift, int coord_mask )
{
	int X= x>>shift, Y= y>>shift;
	int shift_pow2= 1 << shift;
	int mask= shift_pow2 - 1;

	int dx= x & mask, dy= y & mask;
	int dy1= shift_pow2 - dy, dx1= shift_pow2 - dx;

	int noise[3];
	int result= 0;

	if( Y & 1 )
	{
		// left side: y >= 2 * x
		// right side: y >= shift_pow2 * 2 - 2* x
		/*
			+----------+
			|    /\    |
			|   /  \   |
			|  /    \  | shift_pow2
			| /      \ |
			|/        \|
			+----------+
			 shift_pow2
		*/

		if( dy >= 2 * dx )
		{
			noise[0]= g_Noise2( (X  ) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X  ) & coord_mask, (Y+1) & coord_mask, seed );
			noise[2]= g_Noise2( (X+1) & coord_mask, (Y+1) & coord_mask, seed );

			dx-= (dy1>>1) - (shift_pow2>>1);
			dx1= shift_pow2 - dy1 - dx;

			result= (
					noise[0] * dy1 +
					noise[1] * dx1 +
					noise[2] * dx
				) >> shift;
		}
		else if( dy >= shift_pow2 * 2 - 2 * dx )
		{
			noise[0]= g_Noise2( (X+1) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X+1) & coord_mask, (Y+1) & coord_mask, seed );
			noise[2]= g_Noise2( (X+2) & coord_mask, (Y+1) & coord_mask, seed );

			dx-= (shift_pow2>>1) + (dy1>>1);
			dx1= shift_pow2 - dy1 - dx;

			result= (
					noise[0] * dy1 +
					noise[2] * dx +
					noise[1] * dx1
				) >> shift;
		}
		else
		{
			noise[0]= g_Noise2( (X  ) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X+1) & coord_mask, (Y  ) & coord_mask, seed );
			noise[2]= g_Noise2( (X+1) & coord_mask, (Y+1) & coord_mask, seed );

			dx -= dy >> 1;
			dx1= shift_pow2 - dy - dx;

			result= (
					noise[2] * dy +
					noise[0] * dx1 +
					noise[1] * dx
				) >> shift;
		}
	}
	else
	{
		// left side: y <= shift_pow2 - 2 * x
		// right side: y <= 2 * x - shift_pow2
		/*
			+----------+
			|\        /|
			| \      / |
			|  \    /  | shift_pow2
			|   \  /   |
			|    \/    |
			+----------+
			 shift_pow2
		*/
		if( dy <= shift_pow2 - 2 * dx )
		{
			noise[0]= g_Noise2( (X  ) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X+1) & coord_mask, (Y  ) & coord_mask, seed );
			noise[2]= g_Noise2( (X  ) & coord_mask, (Y+1) & coord_mask, seed );

			dx+= (shift_pow2>>1) - (dy>>1);
			dx1= shift_pow2 - dy - dx;

			result= (
					noise[2] * dy +
					noise[0] * dx1 +
					noise[1] * dx
				) >> shift;
		}
		else if( dy <= 2 * dx - shift_pow2 )
		{

			noise[0]= g_Noise2( (X+1) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X+2) & coord_mask, (Y  ) & coord_mask, seed );
			noise[2]= g_Noise2( (X+1) & coord_mask, (Y+1) & coord_mask, seed );

			dx-= (shift_pow2>>1) + (dy>>1);
			dx1= shift_pow2 - dy - dx;

			result= (
					noise[2] * dy +
					noise[0] * dx1 +
					noise[1] * dx
				) >> shift;
		}
		else
		{
			noise[0]= g_Noise2( (X+1) & coord_mask, (Y  ) & coord_mask, seed );
			noise[1]= g_Noise2( (X  ) & coord_mask, (Y+1) & coord_mask, seed );
			noise[2]= g_Noise2( (X+1) & coord_mask, (Y+1) & coord_mask, seed );

			dx-= dy1 >> 1;
			dx1= shift_pow2 - dy1 - dx;

			result= (
					noise[0] * dy1 +
					noise[1] * dx1 +
					noise[2] * dx
				) >> shift;
		}
	}

	return result;
}

// returns value in range [0; 65536 + 65536/2 + 65536/4 + ... + 65536/(2^(octaves - 1)) )
int g_OctaveNoise( int x, int y, int seed, int octaves )
{
	int r= 0;
	for( int i= 0; i < octaves; i++ )
		r += g_InterpolatedNoise( x, y, seed, octaves - i - 1 ) >> i;

	return r;
}

// returns value in range [0; 65536 + 65536/2 + 65536/4 + ... + 65536/(2^(octaves - 1)) )
int g_TriangularOctaveNoise( int x, int y, int seed, int octaves )
{
	int r= 0;
	for( int i= 0; i < octaves; i++ )
		r += g_TriangularInterpolatedNoise( x, y, seed, octaves - i - 1 ) >> i;

	return r;
}

int g_TriangularOctaveNoiseWraped( int x, int y, int seed, int octaves, int size_log2 )
{
	int r= 0;
	for( int i= 0; i < octaves; i++ )
	{
		int shift= octaves - i - 1;
		r += g_TriangularInterpolatedNoise(
			x, y,
			seed,
			shift,
			(1<<(size_log2 - shift)) - 1 ) >> i;
	}

	return r;
}
