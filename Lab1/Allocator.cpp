#include <Windows.h>
#include "Allocator.h"
#include <heapapi.h>
#include <iostream>
#include <stdio.h>

#include <sysinfoapi.h>

enum class SearchMode {
	FirstFit,
	BestFit,
};

static auto searchMode = SearchMode::BestFit;

//Aligning size
inline size_t align(size_t n) {
	return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

//Requesting growable heap from OS
inline HANDLE requestHeapOS() {
	HANDLE growableHeap = HeapCreate(NULL, 0, 0);
	return growableHeap;
}

//Check wether block can be split
inline bool canSplit(Block* block, size_t size) {
	return block->size - size >= 4 * sizeof(word_t);
}

//Check wether block can be merged with next one
bool canMerge(Block* block) {
	return block->next && !block->next->used;
}

//Heap that our process currently using
HANDLE workingHeap = requestHeapOS();


Allocator::Allocator(word_t pgSz)
{
	std::cout << "Welcome to allocation of hell!\n";
	//SYSTEM_INFO info;
	//GetSystemInfo(&info);
	//std::cout << info.dwPageSize;
	//int basePageSize = 4096;

	//Initializing values;
	arenasList[0] = {};
	currentArena = 0;
	lastArena = currentArena; //debugging
	pageSize = pgSz;
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
	}
	size = align(size);
	if (size <= arenasList[currentArena].size) {
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
				nullptr
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
			arenaSize = ceil((float)(sizeof(word_t) * 3 + size) / pageSize) * pageSize;
			break;
		default:
			arenaSize = size + sizeof(word_t) * 3;
			break;
		}

		Arena newArena {
				arenaSize,
				(Block*)createArena(arenaSize),
				nullptr,
				nullptr,
				nullptr
		};
		arenasList[currentArena].next = &newArena;
		currentArena = lastArena + 1;
		if (lastArena < currentArena)lastArena = currentArena;
		arenasList[currentArena] = newArena;
		return allocateOnArena(size);
	}
}

void Allocator::mem_free(void* ptr)
{
	if (ptr != nullptr) {
		Block* block = getHeader((word_t*)ptr);
		while (canMerge(block)) {
			block = merge(block);
		}
		block->used = FALSE;
		/*
		Arena* arena = findArena(block);
		block = arena->heapStart;
		while (block->next != nullptr) {
			if (block->used == TRUE) {
				return;
			}
			block = block->next;
		}
		freeArena(arena);
		*/
	}
}

void* Allocator::mem_realloc(void* ptr, size_t size)
{
	size = align(size);
	Block* block = getHeader((word_t*)ptr);
	block->used = FALSE;
	byte* newDataPtr = (byte *)mem_alloc(size);
	byte* oldDataPtr = (byte*)ptr;
	word_t min = size < block->size ? size : block->size;
	for (int i = 0; i < min; i++) {
		*(newDataPtr + i)= *(oldDataPtr + i);
	}

	if (oldDataPtr != newDataPtr) {
		while (canMerge(block)) {
			block = merge(block);
		}
		block->used = FALSE;
	} else {
		Block* block = getHeader((word_t*)newDataPtr)->next;
		while (canMerge(block)) {
			block = merge(block);
		}
		block->used = FALSE;
	}
	return (void*)newDataPtr;
}

void Allocator::mem_show()
{
	for (int j = 0; j <= lastArena; j++)
	{
		Arena arena = arenasList[j];
		Block* block = arena.heapStart;
		int i = 1;
		std::cout << "\n         Arena " << j+1 << ": size - " << arenasList[j].size << " bytes\n";
		while (block != nullptr)
		{
			std::cout << "Block " << i << " adress: " << block << "; usage: " << block->used << "; size: " << block->size << "\n";
			i++;
			block = block->next;
		}
	}
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
	// Split the larger block, reusing the free part.
	if (canSplit(block, size)) {
		block = split(block, size);
	}

	return block;
}

Block* Allocator::split(Block* block, size_t size)
{
	size_t oldSize = block->size;
	char* blockStart = (char*)block;
	Block* newBlock = (Block*)(blockStart + 3 * sizeof(word_t) + size);
	if (arenasList[currentArena].top == block) {
		arenasList[currentArena].top = newBlock;
	}
	block->size = size;
	block->used = TRUE;
	newBlock->size = oldSize - size - 3 * sizeof(word_t);
	newBlock->used = FALSE;
	newBlock->next = block->next;
	block->next = newBlock;
	
	return block;
}

Block* Allocator::merge(Block* block)
{
	block->size = block->size + block->next->size + 3 * sizeof(word_t);
	block->next = block->next->next;
	return block;
}

//FIX
Arena* Allocator::findArena(Block* targetBlock)
{
	Arena arena = arenasList[0];
	while (arena.arenaStart != nullptr) {
		Block* block = arena.heapStart;
		while (block != nullptr) {
			if (block == targetBlock) {
				return &arena;
			}
			block = block->next;
		}
		arena = *arena.next;
	}
	return nullptr;
}
//FIX
void Allocator::freeArena(Arena* arena)
{
	//MAYBE FIX?
	for (size_t i = 0; i < sizeof(arenasList); i++)	{
		if (&arenasList[i] == arena) {
			if ((i != 0) && (i != sizeof(arenasList))) {
				arenasList[i-1].next = &arenasList[i+1];
			}
			for (size_t j = i; j < sizeof(arenasList)-1; j++)
			{
				arenasList[j] = arenasList[j + 1];
			}
		}
	}
	HeapFree(workingHeap, NULL, arena);
}

word_t* Allocator::allocateOnArena(size_t size)
{
	//Iterate over all arenas
	do {
		//If we dont find block that can fit data of requested size
		//We are trying to create new block of given size on current arena
		if (Block* block = findBlock(size)) {
			block->used = TRUE;
			return block->data;
		}
		size_t currentArenaNewSize = 0;
		Block* block = nullptr;
		if (arenasList[currentArena].heapStart == nullptr) {
			block = (Block*)arenasList[currentArena].arenaStart;
			currentArenaNewSize = sizeof(word_t)*3 + size;
		}
		else {
			block = arenasList[currentArena].heapStart;
			while ((block->next != nullptr)) {
				block = block->next;
			}

			block = (Block*)((char*)block + sizeof(word_t) * 3 + block->size);
			currentArenaNewSize = (size_t)block + sizeof(word_t) * 3 + size - (size_t)arenasList[currentArena].arenaStart;

		}
		if (currentArenaNewSize > arenasList[currentArena].size) {
			if (arenasList[currentArena].next != nullptr) {
				currentArena += 1; //proceed to next arena
				if (lastArena < currentArena)lastArena = currentArena; //debugging
			}
			continue;
		}

		block->size = size;
		block->used = TRUE;

		if (arenasList[currentArena].heapStart == nullptr) {
			arenasList[currentArena].heapStart = block;
		}

		if (arenasList[currentArena].top != nullptr) {
			arenasList[currentArena].top->next = block;
		}

		arenasList[currentArena].top = block;
		return block->data;
	} while (arenasList[currentArena].next != nullptr);
	
	return nullptr;
}

//Function to get block header pointer while having data pointer
Block* Allocator::getHeader(word_t* data)
{
	return (Block*)((char*)data + sizeof(std::declval<Block>().data) -
		sizeof(Block));
}
