#pragma once

#include <cassert>

template<typename E>
class Queue
{
public:

	Queue() = default;
	~Queue()
	{
		Node* current = first;
		while (current)
		{
			Node* next = current->next;
			delete current;
			current = next;
		}
	}

	// Just to make everything easier...
	Queue(Queue& other) = delete;
	Queue& operator=(Queue& other) = delete;

	void Push(E element)
	{
		if (last)
		{
			last->next = new Node(element);
			last = last->next;
		}
		else
		{
			// Note the order! Since IsEmpty queries 'first' we must make sure that
			// the Queue is always valid to use if 'first' is not null (considering
			// how it's used by TextureSystem and ModelSystem, that is).
			last = new Node(element);
			first = last;
		}
	}

	E Pop()
	{
		assert(first);
		Node* firstNode = first;
		first = firstNode->next;
		if (first == nullptr) last = nullptr;

		E element = firstNode->element;
		delete firstNode;
		return element;
	}

	const E& Peek() const
	{
		assert(first);
		return first->element;
	}

	bool IsEmpty() const
	{
		return first == nullptr;
	}

private:

	struct Node
	{
		E element;
		Node* next = nullptr;

		Node(const E& element) : element(element) {}
	};

	Node* first = nullptr;
	Node* last = nullptr;

};
