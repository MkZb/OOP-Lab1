#include <iostream>
#include "Allocator.h"
#include "TestAllocator.h"
using namespace std;

void test_mem() {
	Allocator test = Allocator::Allocator(0);
	auto p1 = test.mem_alloc(3);
	auto p6 = test.mem_alloc(24);
	auto p7 = test.mem_alloc(4084);

	std::cout << "           Memory snap 1           " << "\n";
	test.mem_show();

	test.mem_free(p6);

	std::cout << "\n\n           Memory snap 2           " << "\n";
	test.mem_show();

	auto p8 = test.mem_alloc(1);

	std::cout << "\n\n           Memory snap 3           " << "\n";
	test.mem_show();

	int* p9 = (int*)test.mem_alloc(246);

	std::cout << "\n\n           Memory snap 3           " << "\n";
	test.mem_show();

	auto p10 = test.mem_alloc(10000);

	std::cout << "\n\n           Memory snap 4           " << "\n";
	test.mem_show();

	*p9 = 1000;
	*(p9 + 50) = 1000;
	p9 = (int*)test.mem_realloc(p9, 500);
	std::cout << "\n\n           Memory snap 5           " << "\n";
	test.mem_show();

	test.mem_free(p9);
	void* p11 = test.mem_alloc(142);
	void* p12 = test.mem_alloc(16);
	std::cout << "\n\n           Memory snap 6           " << "\n";
	test.mem_show();

	test.mem_free(p12);
	std::cout << "\n\n           Memory snap 7           " << "\n";
	test.mem_show();

	test.mem_free(p7);
	std::cout << "\n\n           Memory snap 8           " << "\n";
	test.mem_show();

	test.mem_realloc(p11, 100);
	std::cout << "\n\n           Memory snap 9           " << "\n";
	test.mem_show();

	test.mem_free(p10);
	test.mem_free(p11);
	test.mem_free(p1);
	test.mem_free(p8);
	std::cout << "\n\n           Memory snap 10           " << "\n";
	test.mem_show();
}

int main() {
	
	//test_mem();
	Allocator allocator = Allocator::Allocator(0);
	TestAllocator tester = TestAllocator::TestAllocator(&allocator);
	tester.test(3000, 100000);
	return 0;
}