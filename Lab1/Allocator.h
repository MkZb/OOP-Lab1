#pragma once
#include <list>
#include <vector>
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
	//temorary helper functions move to private later
	Block* getHeader(word_t* data);
private:
	static const int DEFAULT_ARENA_SIZE = 4096;
	static const int MAXIMUM_ARENAS = 100; //20 bytes per arena header
	word_t pageSize;
	MemoryType memoryType;
	word_t createArena(size_t size);
	Block* firstFit(size_t size);
	Block* bestFit(size_t size);
	Block* findBlock(size_t size);
	Block* listAllocate(Block* block, size_t size);
	Block* split(Block* block, size_t size);
	Block* merge(Block* block);
	Arena* findArena(Block* targetBlock);
	void freeArena(Arena* arena);
	word_t* allocateOnArena(size_t size);
	//Arena* startingArena;
	//Arena* topArena;
	int currentArena;
	int lastArena;
	Arena arenasList[MAXIMUM_ARENAS];
};

