#pragma once
#include "Allocator.h"
#include <vector>

struct ControlResult {
	void* ptr;
	size_t size;
	unsigned long long int checksum;
};

class TestAllocator
{
public:
	TestAllocator(Allocator* allocator);
	void test(int maxPossibleDataSize, int amountOfActions);
private:
	Allocator* allocator;
	size_t currentDataSize;
	int operationType;
	int amount;
	int successfull;
	void allocate();
	void freeMem();
	void reallocate();
	size_t maxDataSize;
	std::vector<ControlResult> results;
};

