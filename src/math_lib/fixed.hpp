#pragma once
#include "assert.hpp"

typedef int fixed_base_t;

template< int base >
fixed_base_t mFixedMul( fixed_base_t x, fixed_base_t y )
{
	return ( ((long long int)x) * y ) >> base;
}

template< int base >
fixed_base_t mFixedSquare( fixed_base_t x )
{
	return ( ((long long int)x) * x ) >> base;
}

template< int base >
int mFixedMulResultToInt( fixed_base_t x, fixed_base_t y )
{
	return ( ((long long int)x) * y ) >> (base + base);
}

template< int base >
fixed_base_t mFixedRound( fixed_base_t x )
{
	return ( x + (1 << (base - 1)) ) & (~( (1 << base) - 1 ));
}

template< int base >
int mFixedRoundToInt( fixed_base_t x )
{
	return ( x + (1 << (base - 1)) ) >> base;
}

template< int base >
fixed_base_t mFixedDiv( fixed_base_t x, fixed_base_t y )
{
	// TODO - check range of result
	H_ASSERT( y != 0 );
	return ( ((long long int)x) << base ) / y;
}

template< int base >
fixed_base_t mFixedInvert( fixed_base_t x )
{
	return (1 << (base + base)) / ((long long int)x);
}

// If you wish - you can add fixedXX_t and additional direct functions.
typedef fixed_base_t fixed16_t;
typedef fixed_base_t fixed8_t;

inline fixed16_t mFixed16Mul( fixed16_t x, fixed16_t y )
{
	return mFixedMul<16>( x, y );
}

inline fixed16_t mFixed16Square( fixed16_t x )
{
	return mFixedSquare<16>( x );
}

inline int mFixed16MulResultToInt( fixed16_t x, fixed16_t y )
{
	return mFixedMulResultToInt<16>( x, y );
}

inline fixed16_t mFixed16Round( fixed16_t x )
{
	return mFixedRound<16>( x );
}

inline int mFixed16RoundToInt( fixed16_t x )
{
	return mFixedRoundToInt<16>( x );
}

inline fixed16_t mFixed16Div( fixed16_t x, fixed16_t y )
{
	return mFixedDiv<16>( x, y );
}

inline fixed16_t mFixed16Invert( fixed16_t x )
{
	return mFixedInvert<16>( x );
}


