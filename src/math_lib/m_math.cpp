#include "m_math.h"

int m_Math::IntSqrt( int x )
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
