#pragma once

#include <limits>
#include <set>

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
	SmallObjectsAllocatorBlock* next_;

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

	StoredType* New()
	{
		H_ASSERT(occupancy_ < block_size);

		StoredType* result= GetDataPointer() + first_free_index_;

		first_free_index_= index_list_[first_free_index_];
		occupancy_++;

		return result;
	}

	void Delete(StoredType* p)
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
		: blocks_()
		, first_not_full_block_(nullptr)
	{
	}

	StoredType* New()
	{
		StoredType* result;
		if( first_not_full_block_ )
		{
			result= first_not_full_block_->New();
			if (first_not_full_block_->IsFull())
				first_not_full_block_= first_not_full_block_->next_;
		}
		else
		{
			BlockType* new_block= new BlockType();
			new_block->next_= first_not_full_block_;
			first_not_full_block_= new_block;

			blocks_.insert(new_block);

			result= new_block->New();
		}
		return result;
	}

	void Delete(StoredType* p)
	{
		H_ASSERT( !blocks_.empty() );
		auto it= blocks_.lower_bound(reinterpret_cast<BlockType*>(p));

		it--;
	}

private:
	typedef SmallObjectsAllocatorBlock<StoredType, block_size, IndexType> BlockType;
	typedef std::set<BlockType*> BlocksContainer;

	BlocksContainer blocks_;
	BlockType* first_not_full_block_;
};
