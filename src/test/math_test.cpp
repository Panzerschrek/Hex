#include "test.h"

#include "../math_lib/math.hpp"

H_TEST(DivNonNegativeRemainderTest)
{
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 0, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 1, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 2, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 16, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 17, 17 ) == 1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 18, 17 ) == 1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -1, 17 ) == -1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -2, 17 ) == -1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -17, 17 ) == -1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -18, 17 ) == -2 );

	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 0, 1 ) == 0 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 1, 1 ) == 1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( 2, 1 ) == 2 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -1, 1 ) == -1 );
	H_TEST_EXPECT( m_Math::DivNonNegativeRemainder( -2, 1 ) == -2 );
}

H_TEST(ModNonNegativeRemainderTest)
{
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 0, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 1, 17 ) == 1 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 2, 17 ) == 2 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 16, 17 ) == 16 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 17, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 18, 17 ) == 1 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -1, 17 ) == 16 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -2, 17 ) ==  15 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -17, 17 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -18, 17 ) == 16 );

	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 0, 1 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 1, 1 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( 2, 1 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -1, 1 ) == 0 );
	H_TEST_EXPECT( m_Math::ModNonNegativeRemainder( -2, 1 ) == 0 );
}

H_TEST(IntSqrtTest)
{
	H_TEST_EXPECT( m_Math::IntSqrt(0) == 0 );
	H_TEST_EXPECT( m_Math::IntSqrt(1) == 1 );
	H_TEST_EXPECT( m_Math::IntSqrt(2) == 1 );
	H_TEST_EXPECT( m_Math::IntSqrt(3) == 1 );
	H_TEST_EXPECT( m_Math::IntSqrt(4) == 2 );
	H_TEST_EXPECT( m_Math::IntSqrt(5) == 2 );
	H_TEST_EXPECT( m_Math::IntSqrt(6) == 2 );
	H_TEST_EXPECT( m_Math::IntSqrt(7) == 2 );
	H_TEST_EXPECT( m_Math::IntSqrt(8) == 2 );
	H_TEST_EXPECT( m_Math::IntSqrt(9) == 3 );
	H_TEST_EXPECT( m_Math::IntSqrt(10) == 3 );

	H_TEST_EXPECT( m_Math::IntSqrt( 0x9000000 ) == 0x3000 );
	H_TEST_EXPECT( m_Math::IntSqrt( 46340 * 46340 ) == 46340 );
}

H_TEST(NearestPowerOfTwoCeilTest)
{
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(1) == 1 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(2) == 2 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(3) == 4 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(4) == 4 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(5) == 8 );

	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(32767) == 32768 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(32768) == 32768 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoCeil(32769) == 65536 );
}

H_TEST(Log2CeilTest)
{
	H_TEST_EXPECT( m_Math::Log2Ceil(1) == 0 );
	H_TEST_EXPECT( m_Math::Log2Ceil(2) == 1 );
	H_TEST_EXPECT( m_Math::Log2Ceil(3) == 2 );
	H_TEST_EXPECT( m_Math::Log2Ceil(4) == 2 );
	H_TEST_EXPECT( m_Math::Log2Ceil(5) == 3 );

	H_TEST_EXPECT( m_Math::Log2Ceil(32767) == 15 );
	H_TEST_EXPECT( m_Math::Log2Ceil(32768) == 15 );
	H_TEST_EXPECT( m_Math::Log2Ceil(32769) == 16 );
}

H_TEST(NearestPowerOfTwoFloorTest)
{
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(1) == 0 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(2) == 2 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(3) == 2 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(4) == 4 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(5) == 4 );

	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(32767) == 16384 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(32768) == 32768 );
	H_TEST_EXPECT( m_Math::NearestPowerOfTwoFloor(32769) == 32768 );
}

H_TEST(Log2FloorTest)
{
	H_TEST_EXPECT( m_Math::Log2Floor(1) == 0 );
	H_TEST_EXPECT( m_Math::Log2Floor(2) == 1 );
	H_TEST_EXPECT( m_Math::Log2Floor(3) == 1 );
	H_TEST_EXPECT( m_Math::Log2Floor(4) == 2 );
	H_TEST_EXPECT( m_Math::Log2Floor(5) == 2 );

	H_TEST_EXPECT( m_Math::Log2Floor(32767) == 14 );
	H_TEST_EXPECT( m_Math::Log2Floor(32768) == 15 );
	H_TEST_EXPECT( m_Math::Log2Floor(32769) == 15 );
}
