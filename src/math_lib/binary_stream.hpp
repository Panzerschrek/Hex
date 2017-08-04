#pragma once

#include <vector>
#include <cstring>
#include <type_traits>

#include "assert.hpp"

// Simple binary streams over external storage.
// Supports serialization/deserialization for fundamental types.
// Writes binary representation of value in current byte-order.

typedef std::vector<unsigned char> h_BinaryStorage;

class h_BinaryOuptutStream
{
public:
	// Storage cleared after stream constuction.
	explicit h_BinaryOuptutStream( h_BinaryStorage& storage )
		: storage_(storage)
	{
		storage_.clear();
	}

	template<class T>
	h_BinaryOuptutStream& operator<<( const T& t )
	{
		static_assert( std::is_fundamental<T>::value, "Stream supports only fundamental types" );

		storage_.resize( storage_.size() + sizeof(T) );
		std::memcpy( storage_.data() + storage_.size() - sizeof(T), &t, sizeof(T) );

		return *this;
	}

private:
	h_BinaryStorage& storage_;
};

class h_BinaryInputStream
{
public:
	explicit h_BinaryInputStream( const h_BinaryStorage& storage )
		: storage_(storage)
		, pos_(0u)
	{}

	template<class T>
	h_BinaryInputStream& operator>>( T& t )
	{
		static_assert( std::is_fundamental<T>::value, "Stream supports only fundamental types" );

		H_ASSERT( sizeof(T) + pos_ <= storage_.size() );
		std::memcpy( &t, storage_.data() + pos_, sizeof(T) );
		pos_+= sizeof(T);

		return *this;
	}

private:
	const h_BinaryStorage& storage_;
	size_t pos_;
};
