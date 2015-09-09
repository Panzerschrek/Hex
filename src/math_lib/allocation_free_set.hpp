#include <iostream>

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

		StoredType& operator*()
		{
			return p->value;
		}

		void operator++()
		{
			//TODO
		}

	private:
		Node* p;
	};

	AllocationFreeSet()
		: root_(nullptr)
	{
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

		return iterator(node);
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
		std::cout<<" "<<n->value<<std::endl;

		if (n->right) Print_r(n->right, depth + 1);
	}

private:
	Node* root_;
};
