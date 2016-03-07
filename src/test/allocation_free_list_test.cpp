#include "test.h"

#include "../math_lib/allocation_free_list.hpp"


struct ListObject
{
	AllocationFreeList<ListObject*>::Node node;
	int data;
};

typedef AllocationFreeList<ListObject*> ListType;

H_TEST(AlocationFreeListTest)
{
	ListObject objects[3];
	objects[0].data= 0;
	objects[1].data= 1;
	objects[2].data= 2;

	ListType list;
	list.push_back( &objects[0].node, &objects[0] );
	list.push_front( &objects[2].node, &objects[2] );
	list.push_front( &objects[1].node, &objects[1] );

	// Size must be equal to number of inserted elements.
	H_TEST_EXPECT( list.size() == 3 );

	{
		unsigned int iter_count= 0;
		for( const auto& value : list )
		{
			(void)value;
			iter_count++;
		}
		// Iteration count must be equal to size.
		H_TEST_EXPECT( iter_count == list.size() );
	}

	// Stored elements must be in right order.
	auto it= list.begin();
	H_TEST_EXPECT( (it++)->data == 1 );
	H_TEST_EXPECT( (it++)->data == 2 );
	H_TEST_EXPECT( (it++)->data == 0 );
}
