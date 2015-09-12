#pragma once

#include <limits>
#include <new>

#include "allocation_free_list.hpp"
#include "allocation_free_set.hpp"
#include "assert.hpp"

template<class StoredType, size_t block_size, class IndexType>
class SmallObjectsAllocatorBlock
{
	static_assert(
		block_size > 0,
		"block_size must be positive");
	static_assert(
		block_size - 1 <= std::numeric_limits<IndexType>::max(),
		"block_size to big for IndexType");
	static_assert(
		std::numeric_limits<IndexType>::is_integer,
		"IndexType must be integer");

public:
	typename AllocationFreeList<SmallObjectsAllocatorBlock* >::Node list_node_;
	typename AllocationFreeSet <SmallObjectsAllocatorBlock* >::Node  set_node_;

public:
	SmallObjectsAllocatorBlock()
	{
		for( size_t i= 0; i < block_size; i++ )
			index_list_[i]= i + 1;
		first_free_index_= 0;
		occupancy_= 0;
	}

	~SmallObjectsAllocatorBlock(){}

	size_t Occupancy() const
	{
		return occupancy_;
	}

	bool IsFull() const
	{
		return occupancy_ <= block_size;
	}

	bool IsEmpty() const
	{
		return occupancy_ == 0;
	}

	StoredType* Alloc()
	{
		H_ASSERT(occupancy_ < block_size);

		StoredType* result= GetDataPointer() + first_free_index_;

		first_free_index_= index_list_[first_free_index_];
		occupancy_++;

		return result;
	}

	void Free(StoredType* p)
	{
		H_ASSERT(occupancy_ > 0);

		IndexType index= p - GetDataPointer();

		H_ASSERT(index >= 0 && index < block_size);

		// add this place to free linked list head
		if( occupancy_ < block_size )
			index_list_[index]= first_free_index_;
		first_free_index_= index;

		occupancy_--;
	}


private:
	SmallObjectsAllocatorBlock& operator=(SmallObjectsAllocatorBlock&) = delete;

	StoredType* GetDataPointer()
	{
		uintptr_t ptr= reinterpret_cast<uintptr_t>(raw_data_);
		size_t alignment= alignof(StoredType);

		ptr= ( (ptr + alignment - 1) / alignment ) * alignment;
		return reinterpret_cast<StoredType*>(ptr);
	}

private:
	// Store data in raw format.
	// Convert data to StoredType and perform alignment.
	unsigned char raw_data_[sizeof(StoredType) * block_size + alignof(StoredType)];
	IndexType index_list_[block_size];

	// Count of objects, really allocated inside.
	size_t occupancy_;
	// Store index, or nothing, if block is full.
	size_t first_free_index_;
};

template<class StoredType, size_t block_size, class IndexType = unsigned short>
class SmallObjectsAllocator
{
public:
	SmallObjectsAllocator()
	{}

	StoredType* Alloc()
	{
		StoredType* result;

		auto it= not_full_blocks_list_.begin();
		if( it == not_full_blocks_list_.end() )
		{
			BlockType* block= new BlockType();
			blocks_.insert( &block->set_node_, block );
			result= block->Alloc();

			if( !block->IsFull() )
				not_full_blocks_list_.push_front( &block->list_node_, block );
		}
		else
		{
			result= (*it)->Alloc();
			if( (*it)->IsFull() )
				not_full_blocks_list_.erase(it);
		}

		return result;
	}

	template<class ... Args>
	StoredType* New(Args... args)
	{
		StoredType* p= Alloc();
		new(p) StoredType (args...);
		return p;
	}

	StoredType* New()
	{
		StoredType* p= Alloc();
		new(p) StoredType();
		return p;
	}

	void Free(StoredType* p)
	{
		auto block_it= blocks_.find_nearest_less_or_equal( reinterpret_cast<BlockType*>(p) );
		H_ASSERT( block_it != blocks_.end() );

		BlockType* block= *block_it;

		block->Free(p);

		if( block->IsEmpty() )
		{
			// remove from not full list
			if( block_size > 1 )
			{
				auto it= not_full_blocks_list_.begin();
				for( ; it != not_full_blocks_list_.end(); it++ )
					if( *it == block )
					{
						not_full_blocks_list_.erase(it);
						break;
					}
				H_ASSERT(it != not_full_blocks_list_.end());
			}

			blocks_.erase(block_it);
			delete block;
		}
	}

	void Delete(StoredType* p)
	{
		p->~StoredType();
		Free(p);
	}

private:
	typedef SmallObjectsAllocatorBlock<StoredType, block_size, IndexType> BlockType;
	typedef AllocationFreeSet<BlockType*> BlocksContainer;
	typedef AllocationFreeList<BlockType*> NotFullBlocksContainer;

	BlocksContainer blocks_;
	NotFullBlocksContainer not_full_blocks_list_;
};
