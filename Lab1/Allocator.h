#pragma once
#include <list>
using word_t = intptr_t;

struct Block {
	size_t size;
	bool used;
	Block* next;
	word_t data[1];
};

struct Arena {
	size_t size;
	Block* arenaStart;
	Block* heapStart;
	Block* top;
	Arena* next;
	unsigned int usedBlocks;
};

enum class MemoryType {
	NonPaged,
	Paged
};

class Allocator
{
public:
	Allocator(word_t pgSz);
	~Allocator();
	void* mem_alloc(size_t size);
	void mem_free(void* ptr);
	void* mem_realloc(void* ptr, size_t size);
	void mem_show();
private:
	static const int DEFAULT_ARENA_SIZE = 4096;
	static const int MAXIMUM_ARENAS = 1000;
	static const size_t BLOCK_HEADER_SIZE = sizeof(word_t) + sizeof(bool) + sizeof(size_t);
	word_t pageSize;
	MemoryType memoryType;
	word_t createArena(size_t size);
	Block* firstFit(size_t size);
	Block* bestFit(size_t size);
	Block* findBlock(size_t size);
	Block* listAllocate(Block* block, size_t size);
	Block* getHeader(word_t* data);
	bool canSplit(Block* block, size_t size);
	Block* split(Block* block, size_t size);
	Block* merge(Block* block);
	size_t findArena(Block* targetBlock);
	void freeArena(size_t idx);
	word_t* allocateOnArena(size_t size);
	int currentArena;
	int lastArena;
	Arena arenasList[MAXIMUM_ARENAS];
};

