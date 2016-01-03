#pragma once
#include <math.h>

#define MATH_FUNC_INLINE inline
#define MASMx86 0

class m_Math
{
public:

	static int DivNonNegativeRemainder( int x, int y );
	static int ModNonNegativeRemainder( int x, int y );

	// Integer square root. Returns floor(sqrt(x)).
	// Result undefined, if x < 0.
	static int IntSqrt( int x );

	static float F16_to_f32( unsigned short x );
	/*float functions*/
	static float Abs ( float x );
	static float Round( float x );
	static float Sqrt( float x );

	static float Pow(float x );
	static float Pow(float,float);
	static float Exp(float);
	static float Exp2(float);

	static float Log(float,float);
	static float Log2(float);
	static float Ln(float);

	static float Sin( float x );
	static void SinCos( float x,float* s,float* c );
	static float Cos( float x );
	static float Tan( float x );

	static float Asin	( float x );
	static float Acos	( float x );
	static float Atan	( float x);
	static float Actan	( float x );

	/*integer functions*/
	static unsigned int NearestPOT( unsigned int x );
	static unsigned int NearestPOTLog2( unsigned int x );



	/*double functions*/
	static double DAbs  ( double x );
	static double DRound( double x );
	static double DSqrt ( double x );

	static double DPow( double x );
	static double DPow( double,double);
	static double DExp(double);
	static double DExp2(double);

	static double DLog(double,double);
	static double DLog2(double);
	static double DLn(double);

	static double DSin( double x );
	static void   DSinCos( double x,double* s,double* c );
	static double DCos( double x );
	static double DTan( double x );

	static double DAsin	( double x );
	static double DAcos	( double x );
	static double DAtan	( double x );
	static double DActan( double x );

	static const constexpr float  FM_PI= 3.1415926535f;
	static const constexpr float FM_2PI= FM_PI * 2.0f;
	static const constexpr float FM_PI2= FM_PI * 0.5f;
	static const constexpr float FM_PI4= FM_PI * 0.25f;
	static const constexpr float FM_PI8= FM_PI * 0.125f;

	static const constexpr float FM_TODEG= 180.0f / FM_PI;
	static const constexpr float FM_TORAD= FM_PI / 180.0f;

	static float Pi();
	static double DPi();
};


MATH_FUNC_INLINE int m_Math::DivNonNegativeRemainder( int x, int y )
{
	int div= x/y;
	if( div * y > x ) div--;
	return div;
}

MATH_FUNC_INLINE int m_Math::ModNonNegativeRemainder( int x, int y )
{
	int div= x/y;
	if( div * y >x ) div--;
	return x - div * y;
}

MATH_FUNC_INLINE float m_Math::Abs(float x)
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		fabs
		fstp r
	}
#else
	r= fabs(x);
#endif
	return r;
}
MATH_FUNC_INLINE float m_Math::Sqrt(float x)
{
	return sqrtf( x );
}

MATH_FUNC_INLINE float m_Math::Pow(float x, float y)
{
	return powf(x,y);
}

MATH_FUNC_INLINE float m_Math::Exp(float x)
{
	return expf(x);
}
//inline float Exp2(float);

//inline float Log(float,float);
//inline float Log2()float;
MATH_FUNC_INLINE float m_Math::Ln(float x)
{
	return logf( x );
}

MATH_FUNC_INLINE float m_Math::Sin(float x)
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		fsin
		fstp r
	}
#else
	r= sin(x);
#endif
	return r;
	//return sinf(x);
}

MATH_FUNC_INLINE float m_Math::Cos(float x)
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		fcos
		fstp r
	}
#else
	r= cos(x);
#endif
	return r;
	//return cosf(x);
}

MATH_FUNC_INLINE void m_Math::SinCos(float x,float* s,float* c)
{
#if MASMx86
	__asm
	{
		mov ebx,c
		mov edx,s

		fld x
		fsincos
		fstp dword ptr [ebx]
		fstp dword ptr [edx]
	}
#else
	*s= sin(x);
	*c= cos(x);
#endif
}

MATH_FUNC_INLINE float m_Math::Tan(float x)
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		fptan
		fstp r
		fstp r
	}
#else
	r= tan(x);
#endif
	return r;
	//return tanf(x);
}

MATH_FUNC_INLINE float m_Math::Asin(float x)
{
	return asinf(x);
}
MATH_FUNC_INLINE float m_Math::Acos(float x)
{
	return acosf(x);
}
MATH_FUNC_INLINE float m_Math::Atan(float x)
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		fld1
		fpatan
		fstp r
	}
#else
	r= atan(x);
#endif
	return r;
	//return atanf(x);
}

MATH_FUNC_INLINE float m_Math::Actan( float x )
{
	float r;
#if MASMx86
	__asm
	{
		fld1
		fld x
		fpatan
		fstp r
	}
#else
	r= 1.0 / atan(x);
#endif
	return r;
}


MATH_FUNC_INLINE float m_Math::Round( float x )
{
	float r;
#if MASMx86
	__asm
	{
		fld x
		frndint
		fstp r
	}
#else
	r= round(x);
#endif
	return r;
}


MATH_FUNC_INLINE unsigned int m_Math::NearestPOT( unsigned int x )
{
	unsigned int i= 1;
	while( i < x )
		i<<=1;
	return i;
}

MATH_FUNC_INLINE unsigned int m_Math::NearestPOTLog2( unsigned int x )
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








MATH_FUNC_INLINE double m_Math::DAbs( double x)
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		fabs
		fstp r
	}
#else
	r= fabs(x);
#endif
	return r;
}
MATH_FUNC_INLINE double m_Math::DSqrt(double x)
{
	return sqrt( x );
}

MATH_FUNC_INLINE  double m_Math::DPow( double x,  double y)
{
	return pow(x,y);
}

MATH_FUNC_INLINE  double m_Math::DExp( double x)
{
	return exp(x);
}
//inline float Exp2(float);

//inline float Log(float,float);
//inline float Log2()float;
MATH_FUNC_INLINE  double m_Math::DLn( double x)
{
	return log( x );
}

MATH_FUNC_INLINE  double m_Math::DSin( double x)
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		fsin
		fstp r
	}
#else
	r= sin(x);
#endif
	return r;
	//return sin(x);
}

MATH_FUNC_INLINE  double m_Math::DCos( double x)
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		fcos
		fstp r
	}
#else
	r= cos(x);
#endif
	return r;
	//return cos(x);
}

MATH_FUNC_INLINE void m_Math::DSinCos( double x, double* s, double* c)
{
#if MASMx86
	__asm
	{
		mov ebx,c
		mov edx,s

		fld x
		fsincos
		fstp qword ptr [ebx]
		fstp qword ptr [edx]
	}
#else
	*s= sin(x);
	*c= cos(x);
#endif
}

MATH_FUNC_INLINE  double m_Math::DTan( double x)
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		fptan
		fstp r
		fstp r
	}
#else
	r= tan(x);
#endif
	return r;
	//return tan(x);
}

MATH_FUNC_INLINE  double m_Math::DAsin( double x)
{
	return asin(x);
}
MATH_FUNC_INLINE double m_Math::DAcos( double x)
{
	return acos(x);
}
MATH_FUNC_INLINE  double m_Math::DAtan( double x)
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		fld1
		fpatan
		fstp r
	}
#else
	r= atan(x);
#endif
	return r;
	//return atan(x);
}

MATH_FUNC_INLINE  double m_Math::DActan(  double x )
{
	double r;
#if MASMx86
	__asm
	{
		fld1
		fld x
		fpatan
		fstp r
	}
#else
	r= 1.0 / atan(x);
#endif
	return r;
}


MATH_FUNC_INLINE double m_Math::DRound( double x )
{
	double r;
#if MASMx86
	__asm
	{
		fld x
		frndint
		fstp r
	}
#else
	r= round(x);
#endif
	return r;
}






MATH_FUNC_INLINE float m_Math::Pi()
{
	float r;
#if MASMx86
	__asm
	{
		fldpi
		fstp r
	}
#else
	r= 3.1415926535f;
#endif
	return r;
}

MATH_FUNC_INLINE double m_Math::DPi()
{
	double r;
#if MASMx86
	__asm
	{
		fldpi
		fstp r
	}
#else
	r= 3.1415926535897932384626433832795f;
#endif
	return r;
}
