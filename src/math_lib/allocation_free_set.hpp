#pragma once

#include <iostream>

#include "assert.hpp"

template<class T>
class AllocationFreeSet
{
public:

typedef T StoredType;

	class Node
	{
		friend class AllocationFreeSet;

		Node* parent;
		Node* left;
		Node* right;
		StoredType value;
	};

	class iterator
	{
		friend class AllocationFreeSet;

		iterator(Node* n)
			: p(n)
		{}

	public:
		iterator& operator=(const iterator& other)
		{
			p= other.p;
		}

		bool operator==(const iterator& other)
		{
			return p == other.p;
		}

		bool operator!=(const iterator& other)
		{
			return !( *this == other);
		}

		StoredType& operator*()
		{
			H_ASSERT(p);
			return p->value;
		}

		void operator++(int) // it++
		{
			H_ASSERT(p);

			if( p->right )
			{
				p= p->right;
				while( p->left ) p= p->left;
			}
			else
			{
				const StoredType& v= p->value;
				while( p && p->value <= v ) p= p->parent;
			}
		}

		void operator++() // ++it
		{
			return (*this)++;
		}

	private:
		Node* p;
	};

	AllocationFreeSet()
		: root_(nullptr)
		, size_(0)
	{
	}

	iterator begin()
	{
		if( !root_ ) return nullptr;

		Node* p= root_;
		while(p->left) p= p->left;
		return iterator(p);
	}

	iterator end()
	{
		return iterator(nullptr);
	}

	iterator find(const StoredType& v)
	{
		Node* p= root_;
		while(p)
		{
			if( v == p->value ) return iterator(p);
			else if( v < p->value ) p= p->left;
			else p= p->right;
		}
		return iterator(nullptr);
	}

	iterator insert(Node* node, const StoredType& v)
	{
		size_++;

		Node* p= root_;
		Node* p_parent= nullptr;
		while(p)
		{
			p_parent= p;
			     if( v < p->value ) p= p->left;
			else if( v > p->value ) p= p->right;
			else return iterator(nullptr);
		}

		if( p_parent )
		{
			if( v < p_parent->value ) p_parent->left= node;
			else p_parent->right= node;
		}
		else root_= node;

		node->parent= p_parent;
		node->left= node->right= nullptr;
		node->value= v;

		CheckSize();

		return iterator(node);
	}

	void erase(iterator it)
	{
		H_ASSERT(it.p);

		size_--;

		Node* deleted_parent= it.p->parent;

		if( !it.p->left && !it.p->right) // leaf
		{
			if(deleted_parent)
			{
				if( deleted_parent->right == it.p ) deleted_parent->right= nullptr;
				else deleted_parent->left= nullptr;
			}
			else
			{
				H_ASSERT(it.p == root_);
				root_= nullptr;
			}
		}
		else if( it.p->left ) // left, and, maybe, right exist
		{
			// move left pointer to parent
			if( deleted_parent )
			{
				if( deleted_parent->right == it.p ) deleted_parent->right= it.p->left;
				else deleted_parent->left= it.p->left;
				if( it.p->right ) it.p->right->parent= deleted_parent;
			}
			else
			{
				H_ASSERT(it.p == root_);
				root_= it.p->left;
				root_->parent= nullptr;
			}

			if( it.p->right )
			{
				Node* most_right= it.p->left;
				while(1)
				{
					if(most_right->right == nullptr)
					{
						most_right->right= it.p->right;
						it.p->right->parent= most_right;
						goto erase_end;
					}
					most_right= most_right->right;
				}
			}
		} // if( it.p->left )
		else // only right
		{
			if( deleted_parent )
			{
				if( deleted_parent->right == it.p ) deleted_parent->right= it.p->right;
				else deleted_parent->left= it.p->right;
				it.p->right->parent= deleted_parent;
			}
			else
			{
				H_ASSERT(it.p == root_);
				root_= it.p->right;
				root_->parent= nullptr;
			}
		}

		erase_end:
		CheckSize();
	}

	iterator find_nearest_less_or_equal(const StoredType& v)
	{
		if( !root_ ) return iterator(nullptr);

		return find_nearest_less_or_equal_r(root_, v);
	}

	void Print() const
	{
		if( root_ ) Print_r(root_, 1);
	}

private:

	void Print_r(const Node* n, int depth) const
	{
		if (n->left) Print_r(n->left, depth + 1);
		for( int i = 0; i < depth*2; i++ )
			std::cout<<"-";
		std::cout<<" "<<reinterpret_cast<unsigned int>(n->value)<<std::endl;

		if (n->right) Print_r(n->right, depth + 1);
	}

	void CheckSize()
	{
		if( root_ )
		{
			H_ASSERT(GetSize_r(root_) == size_);
		}
	}

	size_t GetSize_r(Node* n)
	{
		size_t s= 1;
		if( n->left  ) s+= GetSize_r(n->left );
		if( n->right ) s+= GetSize_r(n->right);
		return s;
	}

	iterator find_nearest_less_or_equal_r(Node* n, const StoredType& v)
	{
		next_iteration:
		if( n->value > v )
		{
			if( n->left )
			{
				n= n->left;
				goto next_iteration;
			}
			else return iterator(nullptr);
		}

		if( n->value == v ) return iterator(n);

		if( n->right )
		{
			iterator c= find_nearest_less_or_equal_r(n->right, v);
			if(c.p) return c;
		}
		return iterator(n);
	}

private:
	Node* root_;
	size_t size_;
};
