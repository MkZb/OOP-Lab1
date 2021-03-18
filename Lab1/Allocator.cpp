#include <Windows.h>
#include "Allocator.h"
#include <heapapi.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

Allocator::Allocator(word_t pgSz)
{
	std::cout << "Welcome to my allocator!\n";
	//Heap that our process currently using
	workingHeap = requestHeapOS();

	//Initializing values;
	arenasList[0] = {};
	currentArena = 0;
	lastArena = currentArena;
	pageSize = pgSz;
	searchMode = SearchMode::BestFit;
	switch (pageSize)
	{
	case(0):
		memoryType = MemoryType::NonPaged;
		break;
	default:
		memoryType = MemoryType::Paged;
		break;
	}
}

Allocator::~Allocator()
{
	//Giving all memory back to OS on allocator destroy
	HeapDestroy(workingHeap);
	printf("Allocator Destroyed ;(\n");
}


void* Allocator::mem_alloc(size_t size)
{
	//If exceeded maximum set arenas value or requested size is 0
	//returning nullptr
	if ((lastArena + 1 > MAXIMUM_ARENAS) || (size == 0)) {
		return nullptr;
	}

	//Creating starting arena if we didnt have any before
	if (arenasList[0].arenaStart == nullptr) {
		size_t arenaSize;
		switch (memoryType)
		{
		case(MemoryType::Paged): 			
			arenaSize = (DEFAULT_ARENA_SIZE/pageSize)*pageSize;
			break;
		default:
			arenaSize = DEFAULT_ARENA_SIZE;
			break;
		}
		arenasList[0].size = arenaSize;
		arenasList[0].arenaStart = (Block* )createArena(arenaSize);
		arenasList[0].heapStart = nullptr;
		arenasList[0].top = nullptr;
		arenasList[0].next = nullptr;
		arenasList[0].usedBlocks = 0;
	}
	size = align(size);
	if (size <= DEFAULT_ARENA_SIZE) {
		currentArena = 0;
		word_t* ptr = allocateOnArena(size);
		if (ptr == nullptr) {
			//Need new arena
			size_t arenaSize;
			switch (memoryType)
			{
			case(MemoryType::Paged):
				arenaSize = (DEFAULT_ARENA_SIZE / pageSize) * pageSize;
				break;
			default:
				arenaSize = DEFAULT_ARENA_SIZE;
				break;
			}
			Arena newArena {
				arenaSize,
				(Block*)createArena(arenaSize),
				nullptr,
				nullptr,
				nullptr,
				0
			};
			arenasList[currentArena].next = &newArena;
			currentArena += 1;
			if (lastArena < currentArena)lastArena = currentArena; //debugging
			arenasList[currentArena] = newArena;
			return allocateOnArena(size);
		}
		else {
			return ptr;
		}
	} else {
		//Need new bigger arena
		size_t arenaSize;
		switch (memoryType)
		{
		case(MemoryType::Paged):
			arenaSize = ceil((float)(align(BLOCK_HEADER_SIZE) + size) / pageSize) * pageSize;
			break;
		default:
			arenaSize = size + align(BLOCK_HEADER_SIZE);
			break;
		}

		Arena newArena {
				arenaSize,
				(Block*)createArena(arenaSize),
				nullptr,
				nullptr,
				nullptr,
				0
		};
		arenasList[currentArena].next = &newArena;
		lastArena += 1;
		currentArena = lastArena;
		arenasList[currentArena] = newArena;
		return allocateOnArena(size);
	}
}

void Allocator::mem_free(void* ptr)
{
	if (ptr != nullptr) {
		Block* block = getHeader((word_t*)ptr);
		size_t arenaIdx = findArena(block);
		currentArena = arenaIdx;
		while (canMerge(block)) {
			block = merge(block);
		}
		block->used = FALSE;

		arenasList[arenaIdx].usedBlocks -= 1;
		if (arenasList[arenaIdx].usedBlocks == 0) {
			freeArena(arenaIdx);
		}
	}
}

void* Allocator::mem_realloc(void* ptr, size_t size)
{
	size = align(size);
	Block* block = getHeader((word_t*)ptr);
	block->used = FALSE;
	char* newDataPtr = (char*)mem_alloc(size);
	char* oldDataPtr = (char*)ptr;

	//If reallocating on same arena we should subtratct 1 from 
	//Used blocks in arena (because allocateOnArena() adds 1
	//every time we allocate a new block
	int oldArenaIdx = findArena(getHeader((word_t*)oldDataPtr));
	int newArenaIdx = findArena(getHeader((word_t*)newDataPtr));
	if (oldArenaIdx == newArenaIdx) {
		arenasList[newArenaIdx].usedBlocks -= 1;
	} else {
		arenasList[oldArenaIdx].usedBlocks -= 1;
	}
	
	//Moving data if its different block
	if (newDataPtr != oldDataPtr) {
		word_t min = size < block->size ? size : block->size;
		for (int i = 0; i < min; i++) {
			*(newDataPtr + i)= *(oldDataPtr + i);
		}
	}

	currentArena = oldArenaIdx;
	//Trying to merge leftovers from old block or new leftovers in case of same block allocation
	if (oldDataPtr != newDataPtr) {
		while (canMerge(block)) {
			block = merge(block);
		}
		block->used = FALSE;
	} else {
		Block* block = getHeader((word_t*)newDataPtr)->next;
		if (block != nullptr && !block->used) {
			while (canMerge(block)) {
				block = merge(block);
			}
			block->used = FALSE;
		}
	}
	return (void*)newDataPtr;
}

void Allocator::mem_show()
{
	if (!lastArena && !arenasList[lastArena].size) {
		std::cout << "\nMemory currently free" << "\n";
		return;
	}
	for (int j = 0; j <= lastArena; j++)
	{
		Arena arena = arenasList[j];
		Block* block = arena.heapStart;
		int i = 1;
		std::cout << "\n         Arena " << j+1 << ": size - " << arenasList[j].size << " bytes\n";
		
		while (block != nullptr)
		{	
			std::cout << "Block " << i << " data adress: " << block->data << "; usage: " << block->used << "; size: " << block->size << "\n";
			i++;
			block = block->next;
		}
	}
}


//Aligning size
inline size_t Allocator::align(size_t n)
{
	return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

//Requesting growable heap from OS
inline HANDLE Allocator::requestHeapOS()
{
	HANDLE growableHeap = HeapCreate(NULL, 0, 0);
	return growableHeap;
}

//Allocate memory from heap (grow it) to get new arena
word_t Allocator::createArena(size_t size)
{
	word_t ptr = (word_t)HeapAlloc(workingHeap, NULL, size);
	return ptr;
}

Block* Allocator::firstFit(size_t size)
{
	Block* block = arenasList[currentArena].heapStart;

	while (block != nullptr) {
		if (block->used || block->size < size) {
			block = block->next;
			continue;
		}
		return block;
	}

	return nullptr;
}

Block* Allocator::bestFit(size_t size)
{
	Block* block = arenasList[currentArena].heapStart;
	Block* bestFitBlock = firstFit(size);
	if (bestFitBlock == nullptr) {
		return nullptr;
	}
	size_t bestFitSize = bestFitBlock->size;
	block = bestFitBlock;

	while (block != nullptr) {
		if (block->used || block->size < size) {
			block = block->next;
			continue;
		}
		if (bestFitSize > block->size) {
			bestFitSize = block->size;
			bestFitBlock = block;
		}
		block = block->next;
	}
	return bestFitBlock;
}

//Try to find unused block that can fit requested size
Block* Allocator::findBlock(size_t size)
{
	Block* block;
	switch (searchMode) {
	case SearchMode::FirstFit:
		block = firstFit(size);
		if (block != nullptr) {
			listAllocate(block, size);
			return(block);
		}
		break;
	case SearchMode::BestFit:
		block = bestFit(size);
		if (block != nullptr) {
			listAllocate(block, size);
			return(block);
		}
		break;
	}

	return nullptr;
}

Block* Allocator::listAllocate(Block* block, size_t size)
{
	if (canSplit(block, size)) {
		block = split(block, size);
	}

	return block;
}

//Check wether block can be split
bool Allocator::canSplit(Block* block, size_t size)
{
	return block->size - size >= align(BLOCK_HEADER_SIZE + sizeof(word_t));
}

//Check wether block can be merged with next one
bool Allocator::canMerge(Block* block)
{
	return block->next && !block->next->used;
}

//Split block into two
Block* Allocator::split(Block* block, size_t size)
{
	size_t oldSize = block->size;
	char* blockStart = (char*)block;
	Block* newBlock = (Block*)(blockStart + align(BLOCK_HEADER_SIZE) + size);
	if (arenasList[currentArena].top == block) {
		arenasList[currentArena].top = newBlock;
	}
	block->size = size;
	block->used = TRUE;
	newBlock->size = oldSize - size - align(BLOCK_HEADER_SIZE);
	newBlock->used = FALSE;
	newBlock->next = block->next;
	block->next = newBlock;
	
	return block;
}

//Merge two blocks into one
Block* Allocator::merge(Block* block)
{
	if (arenasList[currentArena].top == block->next) {
		arenasList[currentArena].top = block;
	}
	block->size = block->size + block->next->size + align(BLOCK_HEADER_SIZE);
	block->next = block->next->next;
	return block;
}

//Find arena targetBlock belongs to
size_t Allocator::findArena(Block* targetBlock)
{
	for (size_t i = 0; i <= (size_t)lastArena; i++) {
		Arena arena = arenasList[i];
		Block* block = arena.heapStart;
		while (block != nullptr) {
			if (block == targetBlock) {
				return i;
			}
			block = block->next;
		}
	}

	return NULL; //Should be impossible
}

//Free arena
void Allocator::freeArena(size_t idx)
{
	HeapFree(workingHeap, NULL, arenasList[idx].heapStart);

	if (idx != lastArena) {
		for (size_t i = idx; i < lastArena; i++) {
			arenasList[i] = arenasList[i + 1];
			if (idx != 0) {
				arenasList[i - 1].next = &arenasList[i];
			}
			arenasList[i].next = &arenasList[i + 1];
		}
		arenasList[lastArena - 1].next = nullptr;
	} else {
		if (idx != 0) {
			arenasList[idx - 1].next = nullptr;
		}
	}

	arenasList[lastArena] = {};
	if (lastArena != 0) {
		lastArena -= 1;
	}
}

word_t* Allocator::allocateOnArena(size_t size)
{
	//Iterate over all arenas
	for (currentArena = 0; currentArena <= lastArena; currentArena++) {
		//If we dont find block that can fit data of requested size
		//We are trying to create new block of given size on current arena
		if (Block* block = findBlock(size)) {
			arenasList[findArena(block)].usedBlocks += 1;
			block->used = TRUE;
			return block->data;
		}
		size_t currentArenaNewSize = 0;
		Block* block = nullptr;
		if (arenasList[currentArena].heapStart == nullptr) {
			block = (Block*)arenasList[currentArena].arenaStart;
			currentArenaNewSize = align(BLOCK_HEADER_SIZE + size);
		}
		else {
			block = arenasList[currentArena].heapStart;
			while ((block->next != nullptr)) {
				block = block->next;
			}

			block = (Block*)((char*)block + align(BLOCK_HEADER_SIZE) + block->size);
			currentArenaNewSize = (size_t)block + align(BLOCK_HEADER_SIZE) + size - (size_t)arenasList[currentArena].arenaStart;

		}
		if (currentArenaNewSize > arenasList[currentArena].size) {
			continue;
		}

		block->size = size;
		block->used = TRUE;
		block->next = nullptr;

		if (arenasList[currentArena].heapStart == nullptr) {
			arenasList[currentArena].heapStart = block;
		}

		if (arenasList[currentArena].top != nullptr) {
			arenasList[currentArena].top->next = block;
		}

		arenasList[currentArena].top = block;
		arenasList[currentArena].usedBlocks += 1;
		return block->data;
	}
	currentArena -= 1;
	return nullptr;
}

//Function to get block header pointer while having data pointer
Block* Allocator::getHeader(word_t* data)
{
	return (Block*)((char*)data + sizeof(std::declval<Block>().data) -
		sizeof(Block));
}
