#pragma once
#include <cstdint>
#include <limits>

#include "assert.hpp"

typedef std::int32_t fixed_base_t;
typedef std::int64_t fixed_base_square_t;

#define ASSERT_INVALID_BASE \
	static_assert( base >= 0 && base <= std::numeric_limits<fixed_base_t>::digits, "Invalid fixed base" );

template< int base >
fixed_base_t mFixedMul( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * y ) >> base;
}

template< int base >
fixed_base_t mFixedSquare( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * x ) >> base;
}

template< int base >
int mFixedMulResultToInt( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * y ) >> ( fixed_base_square_t(base) * 2 );
}

template< int base >
fixed_base_t mFixedRound( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( x + (1 << (base - 1)) ) & (~( (1 << base) - 1 ));
}

template< int base >
int mFixedRoundToInt( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( x + (1 << (base - 1)) ) >> base;
}

template< int base >
fixed_base_t mFixedDiv( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	// TODO - check range of result
	H_ASSERT( y != 0 );
	return ( fixed_base_square_t(x) << base ) / y;
}

template< int base >
fixed_base_t mFixedInvert( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( 1 << ( fixed_base_square_t(base) * 2 ) ) / fixed_base_square_t(x);
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

#undef ASSERT_INVALID_BASE
