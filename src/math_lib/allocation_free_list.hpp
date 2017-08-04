#pragma once
#include <cstddef>

#include "assert.hpp"

/*
Double-linked list with external storage for internal structures.
Does not perform any memory allocation.
Because storage for internal structures and stored data provides user, cantainer does not deallocate any data on destruction.

UNFINISHED - you can add some members, if you need.
*/

template<class T>
class AllocationFreeList
{
public:
	typedef T StoredType;

	class Node
	{
		friend class AllocationFreeList;

		Node* prev;
		Node* next;
		StoredType value;
	};

	class iterator;

public:
	AllocationFreeList();

	size_t size() const;

	iterator begin();
	iterator end();

	void push_back(Node* node, const StoredType& v);
	void push_front(Node* node, const StoredType& v);
	void erase(iterator it);

private:
	void insert_first(Node* node, const StoredType& v);

private:
	Node* first_;
	Node* last_;

	size_t size_;
};

template<class T>
class AllocationFreeList<T>::iterator
{
	friend class AllocationFreeList;

	iterator(Node* n)
		: p(n)
	{}

public:
	iterator& operator=(const iterator& other)
	{
		p= other.p;
		return *this;
	}

	bool operator==(const iterator& other)
	{
		return p == other.p;
	}

	bool operator!=(const iterator& other)
	{
		return !( *this == other);
	}

	StoredType& operator*() const
	{
		H_ASSERT(p);
		return p->value;
	}

	StoredType& operator->() const
	{
		H_ASSERT(p);
		return p->value;
	}

	iterator operator++(int) // it++
	{
		H_ASSERT(p);
		iterator result= *this;
		p= p->next;
		return result;
	}

	iterator operator++() // ++it
	{
		p= p->next;
		return *this;
	}

	iterator operator--(int) // it--
	{
		H_ASSERT(p);
		iterator result= *this;
		p= p->prev;
		return result;
	}

	iterator operator--() // --it
	{
		p= p->next;
		return *this;
	}

private:
	Node* p;
};

template<class T>
AllocationFreeList<T>::AllocationFreeList()
	: first_(nullptr)
	, last_(nullptr)
	, size_(0)
{}

template<class T>
size_t AllocationFreeList<T>::size() const
{
	return size_;
}

template<class T>
typename AllocationFreeList<T>::iterator AllocationFreeList<T>::begin()
{
	return iterator(first_);
}

template<class T>
typename AllocationFreeList<T>::iterator AllocationFreeList<T>::end()
{
	return iterator(nullptr);
}

template<class T>
void AllocationFreeList<T>::push_front(Node* node, const StoredType& v)
{
	H_ASSERT( (first_ && last_) || (!first_ && !last_) );

	size_++;

	if( first_ == nullptr )
	{
		insert_first(node, v);
		return;
	}

	H_ASSERT(first_->prev == nullptr);
	first_->prev= node;
	node->next= first_;
	node->prev= nullptr;
	node->value= v;
	first_= node;
}

template<class T>
void AllocationFreeList<T>::erase(iterator it)
{
	size_--;

	H_ASSERT(it.p);

	Node* const next= it.p->next;
	Node* const prev= it.p->prev;

	if( prev ) prev->next= next;
	else first_= next;
	if( next ) next->prev= prev;
	else last_= prev;
}

template<class T>
void AllocationFreeList<T>::push_back(Node* node, const StoredType& v)
{
	H_ASSERT( (first_ && last_) || (!first_ && !last_) );

	size_++;

	if( first_ == nullptr )
	{
		insert_first(node, v);
		return;
	}

	H_ASSERT(last_->next == nullptr);
	last_->next= node;
	node->prev= last_;
	node->next= nullptr;
	node->value= v;
	last_= node;
}

template<class T>
void AllocationFreeList<T>::insert_first(Node* node, const StoredType& v)
{
	H_ASSERT( !first_ && !last_ );

	first_= last_= node;
	node->next= node->prev= nullptr;
	node->value= v;
}
