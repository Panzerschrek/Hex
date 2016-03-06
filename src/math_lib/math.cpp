#include "math.hpp"

namespace m_Math
{

int DivNonNegativeRemainder( int x, int y )
{
	int mod= x % y;
	int div= x / y;
	if( mod < 0 ) div--;
	return div;
}

int ModNonNegativeRemainder( int x, int y )
{
	int mod= x % y;
	if( mod < 0 ) mod+= y;
	return mod;
}

int IntSqrt( int x )
{
	int result= 0;
	int i= 1;
	int step= 1 << 14;

	while(x > 0)
	{
		while( step > 1 && x <= ( i + step + 1 ) * step )
			step>>= 1;

		x-= ( i + step + 1 ) * step;
		result+= step;
		i+= step << 1;
	}
	return result;
}

unsigned int NearestPowerOfTwoCeil( unsigned int x )
{
	if( x <= 1 ) return 1;

	unsigned int i= 1;
	while( i < x ) i<<=1;
	return i;
}

unsigned int Log2Ceil( unsigned int x )
{
	unsigned int i= 1;
	unsigned int n= 0;
	while( i < x )
	{
		i<<=1;
		n++;
	}
	return n;
}

unsigned int NearestPowerOfTwoFloor( unsigned int x )
{
	if( x <= 1 ) return 0;

	unsigned int i= 0;
	unsigned int y= 1;
	while( (y<<=1) <= x ) i++;

	return 1 << i;
}

unsigned int Log2Floor( unsigned int x )
{
	unsigned int i= 0;
	unsigned int y= 1;
	while( (y<<=1) <= x ) i++;

	return i;
}

} // namespace m_Math

