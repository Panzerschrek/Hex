#pragma once
#include "assert.hpp"

typedef int fixed_base_t;

template< int base >
fixed_base_t m_FixedMul( fixed_base_t x, fixed_base_t y )
{
	return ( ((long long int)x) * y ) >> base;
}

template< int base >
int m_FixedMulResultToInt( fixed_base_t x, fixed_base_t y )
{
	return ( ((long long int)x) * y ) >> (base + base);
}

template< int base >
fixed_base_t m_FixedRound( fixed_base_t x )
{
	return ( x + (1 << (base - 1)) ) & (~( (1 << base) - 1 ));
}

template< int base >
int m_FixedRoundToInt( fixed_base_t x )
{
	return ( x + (1 << (base - 1)) ) >> base;
}

template< int base >
fixed_base_t m_FixedDiv( fixed_base_t x, fixed_base_t y )
{
	// TODO - check range of result
	H_ASSERT( y != 0 );
	return ( ((long long int)x) << base ) * y;
}

template< int base >
fixed_base_t m_FixedInvert( fixed_base_t x )
{
	return (1 << (base + base)) / ((long long int)x);
}

// If you wish - you can add fixedXX_t and additional direct functions.
typedef fixed_base_t fixed16_t;
typedef fixed_base_t fixed8_t;

inline fixed16_t m_Fixed16Mul( fixed16_t x, fixed16_t y )
{
	return m_FixedMul<16>( x, y );
}

inline int m_Fixed16MulResultToInt( fixed16_t x, fixed16_t y )
{
	return m_FixedMulResultToInt<16>( x, y );
}

inline fixed16_t m_Fixed16Round( fixed16_t x )
{
	return m_FixedRound<16>( x );
}

inline int m_Fixed16RoundToInt( fixed16_t x )
{
	return m_FixedRoundToInt<16>( x );
}

inline fixed16_t m_Fixed16Div( fixed16_t x, fixed16_t y )
{
	return m_FixedDiv<16>( x, y );
}

inline fixed16_t m_Fixed16Invert( fixed16_t x )
{
	return m_FixedInvert<16>( x );
}


