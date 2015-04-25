#pragma once

template< class T>
inline T min( T a, T b )
{
	return ( a > b ) ? b : a;
}

template< class T>
inline T max( T a, T b )
{
	return ( a < b ) ? b : a;
}
