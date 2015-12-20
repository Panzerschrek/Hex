#include "m_math.h"

const float m_Math:: FM_PI=	m_Math::Pi();
const float m_Math:: FM_2PI= FM_PI * 2.0;
const float m_Math:: FM_PI2= FM_PI * 0.5;
const float m_Math:: FM_PI4= FM_PI * 0.25;
const float m_Math:: FM_PI8= FM_PI * 0.125;

const float m_Math:: FM_TODEG=	180.0 / FM_PI;
const float m_Math:: FM_TORAD=	FM_PI / 180.0;

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
