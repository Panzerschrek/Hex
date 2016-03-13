#include "test.h"

#include "../math_lib/allocation_free_set.hpp"

struct SetData;
typedef AllocationFreeSet<SetData*> Set;

struct SetData
{
	Set::Node set_node;
	int something;
};

H_TEST(AllocationFreeSetBasicTest)
{
	Set set;
	SetData elements[5];

	H_TEST_EXPECT(set.begin() == set.end());
	H_TEST_EXPECT(set.size() == 0);

	set.insert( &elements[0].set_node, &elements[0] );
	set.insert( &elements[1].set_node, &elements[1] );
	set.insert( &elements[2].set_node, &elements[2] );
	set.insert( &elements[3].set_node, &elements[3] );
	set.insert( &elements[4].set_node, &elements[4] );

	H_TEST_EXPECT( set.size() == 5 );
	{
		size_t iterations= 0;
		for( const auto& value : set )
		{
			(void)value;
			iterations++;
		}
		H_TEST_EXPECT( iterations == set.size() );
	}

	set.erase( set.begin() );
	set.erase( set.begin() );
	set.erase( set.begin() );
	set.erase( set.begin() );
	set.erase( set.begin() );

	H_TEST_EXPECT( set.size() == 0 );
}

H_TEST(AllocationFreeSetUnorderedAccessTest)
{
	Set set;

	const constexpr size_t c_element_count= 5;
	SetData elements[c_element_count];

	for( SetData& element : elements )
		element.something= &element - elements;

	static const int insertion_order[c_element_count]=
	{
		3, 2, 4, 0, 1,
	};

	static const int erase_order[c_element_count]=
	{
		0, 2, 1, 3, 4,
	};

	for( int n : insertion_order )
		set.insert( &elements[n].set_node, &elements[n] );

	H_TEST_EXPECT( set.size() == c_element_count );
	{
		size_t iterations= 0;
		for( auto& value : set )
		{
			H_TEST_EXPECT( value->something == elements[iterations].something );

			value->something= -1;
			iterations++;
		}
		H_TEST_EXPECT( iterations == set.size() );

	}

	// All elements must be iterated.
	for( SetData& element : elements )
		H_TEST_EXPECT( element.something == -1 );

	for( int n : erase_order )
	{
		auto it= set.find( elements + n );
		H_TEST_ASSERT( it != set.end() );

		set.erase( it );
	}

	H_TEST_EXPECT( set.size() == 0 );
}
