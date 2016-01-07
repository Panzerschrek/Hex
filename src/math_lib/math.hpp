#pragma once

// Simple math functions and constants junk yard.
namespace m_Math
{
	int DivNonNegativeRemainder( int x, int y );
	int ModNonNegativeRemainder( int x, int y );

	// Integer square root. Returns floor(sqrt(x)).
	// Result undefined, if x < 0.
	int IntSqrt( int x );

	unsigned int NearestPowerOfTwoCeil( unsigned int x );
	unsigned int Log2Ceil( unsigned int x );

	unsigned int NearestPowerOfTwoFloor( unsigned int x );
	unsigned int Log2Floor( unsigned int x );

	// Pi value - from WindowsXP calc.exe
	const constexpr float pi= 3.1415926535897932384626433832795f;

	const constexpr float two_pi= pi * 2.0f;
	const constexpr float pi_2= pi * 0.5f;
	const constexpr float pi_4= pi * 0.25f;
	const constexpr float pi_6= pi / 3.0f;
	const constexpr float pi_8= pi * 0.125f;

	const constexpr float rad2deg= 180.0f / pi;
	const constexpr float deg2rad= pi / 180.0f;
}
