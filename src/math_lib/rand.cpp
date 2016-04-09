#include <cstring>

#include "rand.hpp"

const constexpr unsigned int m_Rand::max_rand;

unsigned int mLongRandGetState( const m_LongRand& long_rand )
{
	// This method is hack. We can not directly get state from std::linear_congruential_engine,
	// without it modification. But, we can just copy this variable directly.
	static_assert( sizeof(m_LongRand) == sizeof(unsigned int), "This hack does not work on your compiler." );

	unsigned int result;
	std::memcpy( &result, &long_rand, sizeof(unsigned int) );
	return result;
}

void mLongRandSetState( m_LongRand& long_rand, unsigned int state )
{
	long_rand.seed( state );
}
