﻿#include <Windows.h>
#include <iostream>
using namespace std;


struct LBlockDescriptor {
	size_t nextFreeBlock;//shift
};
struct PageDescriptor {
	size_t next; //list of pages with the same state
	LBlockDescriptor* firstFree; //free block in state 1
	size_t bsize; // size of lBlock in state 1 or number of blocks in state 2
	char state; //0-free, 1-lblock, 2-mblock
};
class Allocator {
public:
	Allocator(const size_t ms, const size_t ps);
	//return addr on begin of allocated block or NULL
	void* mem_alloc(size_t size);
	//return addr on begin of reallocated block or NULL
	void* mem_realloc(void* addr, size_t size);
	//free block by this address
	void mem_free(void* addr);
	//out blocks characteristic in table on console
	void mem_dump();
private:
	//begin of control information
	size_t* begin;
	//begin of pages in memory
	size_t* pagesBegin;
	size_t size;
	size_t pages;
	size_t pageSize;
	//array of all page descriptors
	PageDescriptor* pageDescriptors;
	//array of pages with state 1
	size_t* lBlocks;
	size_t lBlocksLength;
	size_t firstFreePage;
	//define what type of state rigth for this size
	PageDescriptor defineCategory(size_t s);
	//round s to minimal need size power of 2
	size_t defineBlockSize(size_t s);
	//initial all pages as free
	void initPages();
	// return free block with size bs
	size_t* getFreeLBlock(size_t bs);
	//return index for lBlocks array
	size_t getIndex(size_t s);
	size_t createLBlockPage(size_t bs);
	//size_t getLBlockPage(size_t bs);
	//not use :/
	size_t getFreePage();
	//return page`s address from index of pages array
	size_t* getAbsolutePageAddr(size_t index);
	//set all blocks in page to free state
	void setAllFree(PageDescriptor pd);
	//return big block with length ps*pageSize
	size_t* getFreeMBlock(size_t ps);
	//check is this block alst free in this page
	bool freeLBlockIsLast(PageDescriptor pd);
	//return number of page from her addr
	size_t findPageByAddress(size_t* addr);
	//return number of block from his addr
	size_t findBlockByAddress(size_t* addr, size_t bs);
	//copy data from old pos to new
	void copyData(size_t* from, size_t* to, size_t length);
};

Allocator::Allocator(const size_t n, const size_t ps) {
	pages = n / ps;
	size_t pds = sizeof(PageDescriptor) / sizeof(size_t);
	cout << pds << endl;
	size_t spaseForPD = pds*pages;
	size_t maxbs = ps >> 1;
	size_t minbs = 16;
	lBlocksLength = 1;

	while (maxbs != minbs) {
		maxbs >>= 1;
		lBlocksLength++;
	}

	size_t needControlSpase = lBlocksLength + spaseForPD;
	size_t* mas = new size_t[n + needControlSpase];
	size = n;
	begin = mas;
	pagesBegin = mas + needControlSpase;
	pageSize = ps;
	pageDescriptors = (PageDescriptor*)begin;
	lBlocks = begin + spaseForPD;

	for (int i = 0; i < pages; i++) {
		pageDescriptors[i].state = 0;
	}

	for (int i = 0; i < lBlocksLength; i++) {
		lBlocks[i] = pages + 1;
	}

	initPages();
	cout << begin << endl;
	cout << pagesBegin << endl;
	cout << size << endl;
	cout << pageSize << endl;
	cout << pages << endl;
	cout << lBlocksLength << endl;
	cout << pageDescriptors << endl;
	cout << lBlocks << endl;
}

void* Allocator::mem_alloc(size_t s) {
	PageDescriptor def = defineCategory(s);
	size_t* res;
	if (def.state == 1) {
		res = getFreeLBlock(def.bsize);
	}
	else {
		res = getFreeMBlock(def.bsize);
	}
	return res;
}

PageDescriptor Allocator::defineCategory(size_t s) {
	PageDescriptor pd;
	if (s > (pageSize >> 1)) {
		pd.state = 2;
		pd.bsize = s / pageSize;
		pd.bsize += ((s % pageSize) > 0) ? 1 : 0;
	}
	else {
		pd.state = 1;
		pd.bsize = defineBlockSize(s);
	}
	return pd;
}

size_t Allocator::defineBlockSize(size_t s) {
	size_t res = 16;

	while (s > res) {
		res <<= 1;
	}

	return res;
}

size_t* Allocator::getFreeMBlock(size_t ps) {
	PageDescriptor pd = pageDescriptors[firstFreePage];
	size_t firstPage = firstFreePage;
	size_t thisPage = firstFreePage;
	size_t nextPage;
	size_t counter = 0;

	while (pd.next <= pages && counter < ps) {
		nextPage = pd.next;
		if (nextPage - 1 == thisPage) {
			counter++;
		}
		else {
			counter = 0;
			firstPage = nextPage;
		}
		thisPage = nextPage;
		pd = pageDescriptors[thisPage];
		nextPage = pd.next;
	}

	if (counter == ps) {
		firstFreePage = nextPage;
		return getAbsolutePageAddr(firstFreePage);
	}
	else {
		return NULL;
	}
}

bool Allocator::freeLBlockIsLast(PageDescriptor pd) {
	LBlockDescriptor* desc = pd.firstFree;
	if (desc->nextFreeBlock) {
		return false;
	}
	else {
		return true;
	}
}

size_t* Allocator::getFreeLBlock(size_t bs) {
	size_t numberOfPage = lBlocks[getIndex(bs)];
	size_t index;

	if (numberOfPage > pages) {
		index = createLBlockPage(bs);
	}
	else {
		index = numberOfPage;
	}

	PageDescriptor pd = pageDescriptors[index];
	LBlockDescriptor* numberOfBlock = pd.firstFree;

	if (numberOfBlock->nextFreeBlock == 0) {
		pd.firstFree = NULL;
		lBlocks[index] = pages + 1;
	}
	else {
		pd.firstFree = (LBlockDescriptor*)((size_t*)(numberOfBlock) + numberOfBlock->nextFreeBlock);
	}

	return (size_t*)pd.firstFree;
}

size_t Allocator::createLBlockPage(size_t bs) {
	size_t index = getFreePage();

	if (index <= pages) {
		PageDescriptor pd = pageDescriptors[index];
		pd.state = 1;
		pd.bsize = bs;
		size_t* addr = getAbsolutePageAddr(index);

		for (int i = 0; i < pages / bs; i++) {
			LBlockDescriptor* des = (LBlockDescriptor*)addr[i * pd.bsize];
			des->nextFreeBlock = 1;
			if (i == pages / bs - 1)
			{
				des->nextFreeBlock = 0;
			}
		}

		pd.firstFree = 0;
		pd.next = pages + 1;
		lBlocks[getIndex(bs)] = index;
	}

	return index;
}

size_t* Allocator::getAbsolutePageAddr(size_t index) {
	size_t* res = NULL;

	if (index <= pages) {
		res = &(pagesBegin[pageSize * index]);
	}

	return res;
}

size_t Allocator::getFreePage() {
	PageDescriptor pd = pageDescriptors[firstFreePage];
	firstFreePage = pd.next;
	return pages + 1;
}

void Allocator::setAllFree(PageDescriptor pd) {
	size_t bs = pd.bsize;
	size_t blocks = pageSize / bs;

	for (int i = 0; i < blocks; i++) {

	}
}

void Allocator::initPages() {
	for (int i = 0; i < pages; i++) {
		pageDescriptors[i].state = 0;
		pageDescriptors[i].next = i + 1;
	}
}

size_t Allocator::getIndex(size_t s) {
	size_t counter = 0;

	while (s > 1) {
		counter++;
		s >>= s;
	}

	counter -= 4;
	return counter;
}

void* Allocator::mem_realloc(void* addr, size_t size) {
	if (addr == NULL) {
		return mem_alloc(size);
	}
	size_t* beg = (size_t*)addr;
	size_t pageNumber = findPageByAddress(beg);
	PageDescriptor pd = pageDescriptors[pageNumber];
	size_t usefulMem;
	if (pd.state == 1) {
		usefulMem = pd.bsize;
	}
	else {
		usefulMem = pd.bsize * pageSize;
	}
	if (size == usefulMem) {
		return addr;
	}
	mem_free(addr);
	size_t* newAddr = (size_t*)mem_alloc(size);
	size_t length = min(size, usefulMem);
	copyData(beg, newAddr, length);
	return newAddr;
}

void Allocator::copyData(size_t* from, size_t* to, size_t length) {
	if (from > to) {
		for (int i = 0; i < length; i++) {
			to[i] = from[i];
		}
	}
	else {
		for (int i = length - 1; i <= 0; i++) {
			to[i] = from[i];
		}
	}
}

void Allocator::mem_free(void* addr) {
	size_t pageNumber = findPageByAddress((size_t*)addr);
	PageDescriptor pd = pageDescriptors[pageNumber];

	if (pd.state == 1) {
		size_t block = findBlockByAddress((size_t*)addr, pd.bsize);

		if (pd.firstFree == NULL) {
			pd.firstFree = (LBlockDescriptor*)(addr);
			pd.firstFree->nextFreeBlock = 0;
		}
		else {
			size_t shift = (size_t*)addr - (size_t*)(pd.firstFree);
			pd.firstFree = (LBlockDescriptor*)(addr);
			pd.firstFree->nextFreeBlock = shift;
		}
	}
	else {
		size_t pgs = pd.bsize;

		for (int i = 0; i < pgs; i++) {
			pd = pageDescriptors[pageNumber + i];
			pd.state = 0;
			pd.bsize = 0;
			pd.firstFree = NULL;
			pd.next = firstFreePage;
			firstFreePage = pageNumber + i;
		}
	}
}

size_t Allocator::findPageByAddress(size_t* addr) {
	size_t shiftFromBegin = addr - pagesBegin;
	size_t pageNumber = shiftFromBegin / pageSize;
	return pageNumber;
}

size_t Allocator::findBlockByAddress(size_t* addr, size_t bs) {
	size_t shiftFromBegin = addr - pagesBegin;
	size_t shiftFromPageBegin = shiftFromBegin % pageSize;
	size_t number = shiftFromPageBegin / bs;
	return number;
}

void Allocator::mem_dump() {
	cout << "begin: " << begin << endl;
	cout << "begin of pages : " << pagesBegin << endl;
	cout << "number of pages : " << pages << endl;
	cout << "page size : " << pageSize << endl;
	cout << "page descriptors state : " << endl;
	PageDescriptor pd;
	cout << "(state, size, next, firstFree)" << endl;

	for (int i = 0; i < pages; i++) {
		pd = pageDescriptors[i];
		cout << "[" << i << "] " << pd.state << " " << pd.bsize << " " << pd.next << " " << pd.firstFree << endl;
	}
}

int main() {
	//size_t size = 1024 * 4096;
	//size_t ps = 4096;
	//int n = 300;
	size_t size = 1024 * 4096;
	size_t ps = 4096;
	int n = 300;
	Allocator al(size, ps);
	size_t** addrArray = new size_t * [n];
	cout << "---------test started--------" << endl;

	for (int i = 0; i < n; i++) 
	{
		addrArray[i] = (size_t*)al.mem_alloc(rand());
		if (addrArray[i] == NULL) 
		{
			cout << "error" << endl;
		}
	}

	al.mem_dump();

	for (int i = 0; i <	 n / 3; i++) 
	{
		al.mem_realloc(addrArray[i], rand());
	}

	al.mem_dump();

	for (int i = n / 2; i << n; i++) 
	{
		al.mem_free(addrArray[i]);
	}

	al.mem_dump();
	cout << "--------END----------" << endl;

	return 0;
}