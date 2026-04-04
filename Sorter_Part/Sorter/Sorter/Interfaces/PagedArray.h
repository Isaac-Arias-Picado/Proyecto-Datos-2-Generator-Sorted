#pragma once
#include <string>
#include <fstream>

struct Page {
    int* data = nullptr;
    int       pageNumber = -1;
    bool      loaded = false;
    bool      modified = false;
    long long loadOrder = 0;
};

class PagedArray {
public:
    PagedArray(const std::string& fname, int pageSize, int pageCount);
    ~PagedArray();

    int& operator[](int index);
    int  read(int index);
    void flush();

    long long getPageFaults() const { return pageFaults; }
    long long getPageHits()   const { return pageHits; }
    long long size()          const { return totalInts; }

    int* getPagePtr(int index, int& pageStart, int& pageEnd);
    int  getIntsPerPage() const { return intsPerPage; }

private:
    std::string  filename;
    std::fstream file;
    int          pageSize;
    int          pageCount;
    int          intsPerPage;
    int          pageShift;   // log2(intsPerPage) si es potencia de 2, -1 si no
    int          pageMask;    // intsPerPage - 1 si potencia de 2

    long long pageFaults;
    long long pageHits;

    long long fileSizeBytes;
    long long totalInts;
    long long totalPages;
    Page* memoryPages;
    long long fifoCounter;
    int* pageTableArray;
    int       pageTableSize = 0;

    // Cache de última página para read() secuencial (TXT)
    int lastReadPage = -1;
    int lastReadSlot = -1;

    void loadPage(int pageNumber, int slot);
    void savePage(int slot);
    int  getAvailableSlot();
};