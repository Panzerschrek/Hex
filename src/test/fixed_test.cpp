#include "test.h"

#include "../math_lib/fixed.hpp"

H_TEST(FixedMulTest)
{
	constexpr int base= 10;

	// 2 * 2 = 4
	H_TEST_EXPECT( mFixedMul<base>( 2 << base, 2 << base ) == (4 << base) );
	// x * y = y * x
	H_TEST_EXPECT( mFixedMul<base>( -42 << base, 24 <<base ) == mFixedMul<base>( 24 << base, -42 << base ) );
	// x * 1 = x
	H_TEST_EXPECT( mFixedMul<base>( 123456 << base, 1 << base ) == (123456 << base) );
	// x * 0 = 0
	H_TEST_EXPECT( mFixedMul<base>( -654321 << base, 0 ) == 0 );
	// fractional mumbers
	H_TEST_EXPECT( mFixedMul<base>( (1<<base) / 4, 8 << base ) == (2 << base) );
}

H_TEST(FixedMulTestZeroBase)
{
	constexpr int base= 0;

	fixed_base_t max_int= std::numeric_limits<fixed_base_t>::max() >> base;

	H_TEST_EXPECT( mFixedMul<base>( 1 << base, 42 << base ) == (42 << base ) );
	H_TEST_EXPECT( mFixedMul<base>( max_int << base, 1 << base ) == (max_int << base) );
}

H_TEST(FixedMulTestSmallBase)
{
	constexpr int base= 2;

	fixed_base_t max_int= std::numeric_limits<fixed_base_t>::max() >> base;

	H_TEST_EXPECT( mFixedMul<base>( 1 << base, 42 << base ) == (42 << base ) );
	H_TEST_EXPECT( mFixedMul<base>( max_int << base, 1 << base ) == (max_int << base) );
	H_TEST_EXPECT( mFixedMul<base>( -(1 << base) / 2, -148562 << base ) == (74281 << base) );
}

H_TEST(FixedMulTestLargeBase)
{
	constexpr int base= 29;

	fixed_base_t max_int= std::numeric_limits<fixed_base_t>::max() >> base;

	H_TEST_EXPECT( mFixedMul<base>( 1 << base, 5 << base ) == (5 << base ) );
	H_TEST_EXPECT( mFixedMul<base>( max_int << base, 1 << base ) == (max_int << base) );

	H_TEST_EXPECT( mFixedMul<base>( 2 << base, -1 << (base - 13)) == (-2 << (base - 13)) );
}

H_TEST(FixedMulTestMaxBase)
{
	constexpr int base= 31;

	H_TEST_EXPECT( mFixedMul<base>( 1 << (base - 1), 1 << (base - 14) ) == (1 << (base - 15)) );
	H_TEST_EXPECT( mFixedMul<base>( -1 << base, 1 << base ) == (-1 << base) );
	H_TEST_EXPECT( mFixedMul<base>( -1 << base, -1 << base ) == (1 << base) );
}

H_TEST(FixedSquareTest)
{
	constexpr int base= 16;

	H_TEST_EXPECT( mFixedSquare<base>( 14789 << base ) == mFixedMul<base>( 14789 << base, 14789 << base ) );
	H_TEST_EXPECT( mFixedSquare<base>( 1 << base ) == (1 << base ) );
	H_TEST_EXPECT( mFixedSquare<base>( -1 << base ) == (1 << base ) );
	H_TEST_EXPECT( mFixedSquare<base>( 2 << base ) == (4 << base ) );
}

H_TEST(FixedMulResultToIntTest)
{
	constexpr int base= 20;

	H_TEST_EXPECT( mFixedMulResultToInt<base>( 14 << base, 88 << base ) == 14 * 88 );
	H_TEST_EXPECT( mFixedMulResultToInt<base>( -733 << base, 924 << base ) == -733 * 924 );
	H_TEST_EXPECT( mFixedMulResultToInt<base>( 1 << (base + 10), -5 << (base + 7) ) == (-5 << (10+7)) );
}

H_TEST(FixedRoundTest)
{
	constexpr int base= 13;

	H_TEST_EXPECT( mFixedRound<base>( (9871 << base) + (13 << (base - 5)) ) == (9871 << base) );
	H_TEST_EXPECT( mFixedRound<base>( (521 << base) + (3<<base) / 4 ) == (522 << base) );
	H_TEST_EXPECT( mFixedRound<base>( (-521 << base) + (3<<base) / 4 ) == -(520 << base) );
	H_TEST_EXPECT( mFixedRound<base>( (521 << base) + (7<<base) / 64 ) == (521 << base) );
	H_TEST_EXPECT( mFixedRound<base>( (-521 << base) + (7<<base) / 64 ) == -(521 << base) );
}

H_TEST(FixedRoundToIntTest)
{
	constexpr int base= 17;

	H_TEST_EXPECT( mFixedRoundToInt<base>( (365 << base) + (13 << (base - 5)) ) == 365 );
	H_TEST_EXPECT( mFixedRoundToInt<base>( (711 << base) + (3<<base) / 4 ) == 712 );
	H_TEST_EXPECT( mFixedRoundToInt<base>( (-711 << base) + (3<<base) / 4 ) == -710 );
	H_TEST_EXPECT( mFixedRoundToInt<base>( (711 << base) + (7<<base) / 64 ) == 711  );
	H_TEST_EXPECT( mFixedRoundToInt<base>( (-711 << base) + (7<<base) / 64 ) == -711 );
}

H_TEST(FixedDivTest)
{
	constexpr int base= 8;

	H_TEST_EXPECT( mFixedDiv<base>( 1 << base, 1 << base ) == (1 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( -1 << base, -1 << base ) == (1 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( 0 << base, 1 << base ) == 0 );
	H_TEST_EXPECT( mFixedDiv<base>( 0 << base, -1 << base ) == 0 );
	H_TEST_EXPECT( mFixedDiv<base>( 1 << base, -1 << base ) == -(1 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( 14567 << base, 1 << base ) == (14567 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( -14567 << base, 1 << base ) == -(14567 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( 75875 << base, 75875 << base ) == (1 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( -75875 << base, 75875 << base ) == -(1 << base) );

	H_TEST_EXPECT( mFixedDiv<base>( 45 << base, 5 << base ) == (9 << base) );
	H_TEST_EXPECT( mFixedDiv<base>( -245 << base, 1 << (base-1) ) == -2 * (245 << base) );

	H_TEST_EXPECT( mFixedDiv<base>( -755 << base, 2 << base ) == (-755 << base) / 2 );
	H_TEST_EXPECT( mFixedDiv<base>( 5111 << base, 8 << base ) == (5111 << base) / 8 );
}

H_TEST(FixedInvertTest)
{
	constexpr int base= 15;

	H_TEST_EXPECT( mFixedInvert<base>( 1 << base ) == (1 << base) );
	H_TEST_EXPECT( mFixedInvert<base>( -1 << base ) == (-1 << base) );
	H_TEST_EXPECT( mFixedInvert<base>( 2 << base ) == (1 << (base-1)) );

	H_TEST_EXPECT( mFixedInvert<base>( 1 ) == ( 1 << (base + base) ) );

	H_TEST_EXPECT( mFixedInvert<base>( 3 << base ) == (1 << base) / 3 );
}
