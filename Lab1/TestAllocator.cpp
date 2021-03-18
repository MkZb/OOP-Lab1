#include <iostream>
#include <fstream>
#include "TestAllocator.h"
#include "ConsoleColor.h"

using namespace std;

TestAllocator::TestAllocator(Allocator* alloc)
{
	std::cout << white;
	//time(0)
	srand(0);
	allocator = alloc;
	currentDataSize = 0;
	operationType = 0;
	maxDataSize = 0;
	amount = 0;
	successfull = 0;
}

void TestAllocator::test(int maxPossibleDataSize, int amountOfActions)
{
	maxDataSize = maxPossibleDataSize;
	amount = amountOfActions;
	for (size_t i = 0; i < amountOfActions; i++) {
		currentDataSize = rand() % maxDataSize + 1;
		operationType = rand() % 100 + 1;
		
		if (operationType < 40) {
			allocate();
		} else if (operationType < 80) {
			if (results.size() < 40) {
				allocate();
			} else {
				freeMem();
			}
		} else {
			if (results.size() == 0) {
				allocate();
			} else {
				reallocate();
			}
		}
	}

	while (results.size() != 0) {
		freeMem();
		amount += 1;
	}

	std::cout << blue;
	allocator->mem_show();
	switch (successfull == amount)
	{
	case(TRUE):
		std::cout << green << "TESTS PASSED (" << successfull << "/" << amount << ")\n";
		break;
	case(FALSE):
		std::cout << red << "TESTS PASSED (" << successfull << "/" << amount << ")\n";
	}
	std::cout << white;
}

void TestAllocator::allocate()
{
	void* ptr = allocator->mem_alloc(currentDataSize);
	char* bytePtr = (char*)ptr;
	unsigned long long int checksum = 0;
	for (size_t i = 0; i < currentDataSize; i++) {
		unsigned char randByte = rand() % 256;
		checksum += randByte;
		*(bytePtr + i) = randByte;
	}

	ControlResult result = {
		ptr,
		currentDataSize,
		checksum
	};

	results.push_back(result);
	//std::cout << white << "ALLOCATED " << currentDataSize << " AT PTR: " << ptr << "\n";
	//std::cout << white;
	//allocator->mem_show();
	successfull += 1;
}

void TestAllocator::freeMem()
{
	size_t resultToFree = rand() % results.size();
	unsigned long long int checksum = 0;

	for (size_t i = 0; i < results.at(resultToFree).size; i++) {
		unsigned char currByte = *((char*)results.at(resultToFree).ptr + i);
		checksum += currByte;
	}
	std::cout << white << "FREEING PTR: " << results.at(resultToFree).ptr << "\n";
	std::cout << white << "CREATED CHECKSUM          CHECKSUM BEFORE FREEING" << "\n";
	if (results.at(resultToFree).checksum == checksum) {
		std::cout << green << results.at(resultToFree).checksum << "                         " << checksum << "\n";
		successfull += 1;
	}
	else {
		std::cout << red << results.at(resultToFree).checksum << "                         " << checksum << "\n";
		std::cout << "PTR: " << results.at(resultToFree).ptr << "; -size: " << results.at(resultToFree).size << "\n";
	}
	allocator->mem_free(results.at(resultToFree).ptr);
	//std::cout << white;
	//allocator->mem_show();
	results.erase(results.begin() + resultToFree);
}

void TestAllocator::reallocate()
{
	size_t resultToRealloc = rand() % results.size();
	unsigned long long int checksum = 0;
	for (size_t i = 0; i < results.at(resultToRealloc).size; i++) {
		unsigned char currByte = *((char*)results.at(resultToRealloc).ptr + i);
		checksum += currByte;
	}
	std::cout << white << "REALLOCATING PTR: " << results.at(resultToRealloc).ptr << "\n";
	std::cout << white << "CREATED CHECKSUM          CHECKSUM BEFORE REALLOCATION" << "\n";
	if (results.at(resultToRealloc).checksum == checksum) {
		std::cout << green << results.at(resultToRealloc).checksum << "                         " << checksum << "\n";
		successfull += 1;
	}
	else {
		std::cout << red << results.at(resultToRealloc).checksum << "                         " << checksum << "\n";
		std::cout << "PTR: " << results.at(resultToRealloc).ptr << "; -size: " << results.at(resultToRealloc).size << "\n";
	}
	void* newPtr = allocator->mem_realloc(results.at(resultToRealloc).ptr, results.at(resultToRealloc).size);
	//std::cout << white;
	//allocator->mem_show();
	results.at(resultToRealloc).ptr = newPtr;
}
