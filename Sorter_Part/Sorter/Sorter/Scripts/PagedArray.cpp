#include "../Interfaces/PagedArray.h"
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

PagedArray::PagedArray(const std::string& fname, int pSize, int pCount)
    : filename(fname), pageSize(pSize), pageCount(pCount),
    pageFaults(0), pageHits(0), fileSizeBytes(0), totalInts(0),
    totalPages(0), fifoCounter(0), pageTableSize(0)
{
    file.open(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir " << filename << endl;
        exit(1);
    }

    file.seekg(0, ios::end);
    fileSizeBytes = static_cast<long long>(file.tellg());
    file.seekg(0, ios::beg);

    if (fileSizeBytes <= 0) {
        cerr << "Error: Archivo vacio" << endl;
        exit(1);
    }
    if (pageSize % (int)sizeof(int) != 0) {
        cerr << "Error: pageSize no es multiplo de sizeof(int)" << endl;
        exit(1);
    }

    intsPerPage = pageSize / (int)sizeof(int);
    totalInts = fileSizeBytes / sizeof(int);
    totalPages = (fileSizeBytes + pageSize - 1) / pageSize;
    pageTableSize = (int)totalPages;

    pageTableArray = new int[pageTableSize];
    for (int i = 0; i < pageTableSize; i++)
        pageTableArray[i] = -1;

    memoryPages = new Page[pageCount];
    for (int i = 0; i < pageCount; i++) {
        memoryPages[i].pageNumber = -1;
        memoryPages[i].loaded = false;
        memoryPages[i].modified = false;
        memoryPages[i].loadOrder = 0;
        memoryPages[i].data = new int[intsPerPage]();
    }
}

PagedArray::~PagedArray() {
    flush();
    if (file.is_open()) file.close();
    for (int i = 0; i < pageCount; i++)
        delete[] memoryPages[i].data;
    delete[] memoryPages;
    delete[] pageTableArray;
}

void PagedArray::loadPage(int pageNumber, int slot) {
    long long pos = (long long)pageNumber * pageSize;
    int       bytesToRead = (int)min((long long)pageSize, fileSizeBytes - pos);

    file.seekg((streampos)pos);
    file.read(reinterpret_cast<char*>(memoryPages[slot].data), bytesToRead);
    if (file.eof()) file.clear();

    if (bytesToRead < pageSize)
        memset(reinterpret_cast<char*>(memoryPages[slot].data) + bytesToRead,
            0, pageSize - bytesToRead);

    memoryPages[slot].pageNumber = pageNumber;
    memoryPages[slot].loaded = true;
    memoryPages[slot].modified = false;
    memoryPages[slot].loadOrder = fifoCounter++;
    pageTableArray[pageNumber] = slot;
}

void PagedArray::savePage(int slot) {
    if (!memoryPages[slot].loaded || !memoryPages[slot].modified) return;
    int pageNum = memoryPages[slot].pageNumber;
    if (pageNum < 0) return;

    long long pos = (long long)pageNum * pageSize;
    int       bytesToWrite = (int)min((long long)pageSize, fileSizeBytes - pos);

    file.seekp((streampos)pos);
    file.write(reinterpret_cast<char*>(memoryPages[slot].data), bytesToWrite);
    memoryPages[slot].modified = false;
}

int PagedArray::findPage(int pageNumber) {
    if (pageNumber < 0 || pageNumber >= pageTableSize) return -1;
    int slot = pageTableArray[pageNumber];
    if (slot >= 0 && slot < pageCount &&
        memoryPages[slot].loaded &&
        memoryPages[slot].pageNumber == pageNumber)
        return slot;
    if (slot >= 0) pageTableArray[pageNumber] = -1;
    return -1;
}

int PagedArray::getAvailableSlot() {
    for (int i = 0; i < pageCount; i++) {
        if (!memoryPages[i].loaded) {
            if (memoryPages[i].pageNumber >= 0)
                pageTableArray[memoryPages[i].pageNumber] = -1;
            return i;
        }
    }

    int oldest = 0;
    for (int i = 1; i < pageCount; i++)
        if (memoryPages[i].loadOrder < memoryPages[oldest].loadOrder)
            oldest = i;

    int oldPageNum = memoryPages[oldest].pageNumber;
    if (oldPageNum >= 0) pageTableArray[oldPageNum] = -1;

    savePage(oldest);
    memoryPages[oldest].loaded = false;
    return oldest;
}

int PagedArray::getSlot(int index, bool markModified) {
    int pageNeeded = index / intsPerPage;
    int slot = findPage(pageNeeded);

    if (slot != -1) {
        pageHits.fetch_add(1, memory_order_relaxed);
    }
    else {
        pageFaults.fetch_add(1, memory_order_relaxed);
        slot = getAvailableSlot();
        loadPage(pageNeeded, slot);
    }

    if (markModified) memoryPages[slot].modified = true;
    return slot;
}

int& PagedArray::operator[](int index) {
    int slot = getSlot(index, true);
    int offset = index % intsPerPage;
    return memoryPages[slot].data[offset];
}

int PagedArray::read(int index) {
    int slot = getSlot(index, false);
    int offset = index % intsPerPage;
    return memoryPages[slot].data[offset];
}

int* PagedArray::getPagePtr(int index, int& pageStart, int& pageEnd) {
    int slot = getSlot(index, true);
    int pageNum = memoryPages[slot].pageNumber;
    pageStart = pageNum * intsPerPage;
    pageEnd = pageStart + intsPerPage - 1;
    return memoryPages[slot].data;
}

void PagedArray::flush() {
    for (int i = 0; i < pageCount; i++)
        savePage(i);
    if (file.is_open()) file.flush();
}

long long PagedArray::getPageFaults() const { return pageFaults.load(memory_order_relaxed); }
long long PagedArray::getPageHits()   const { return pageHits.load(memory_order_relaxed); }