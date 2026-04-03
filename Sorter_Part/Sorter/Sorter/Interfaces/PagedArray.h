#pragma once
#include <string>
#include <fstream>
#include <atomic>

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

    long long getPageFaults() const;
    long long getPageHits()   const;
    long long size()          const { return totalInts; }

    int* getPagePtr(int index, int& pageStart, int& pageEnd);
    int  getIntsPerPage() const { return intsPerPage; }

private:
    std::string  filename;
    std::fstream file;
    int          pageSize;
    int          pageCount;
    int          intsPerPage;   // calculado una vez en constructor

    std::atomic<long long> pageFaults;
    std::atomic<long long> pageHits;

    long long fileSizeBytes;
    long long totalInts;
    long long totalPages;

    Page* memoryPages;
    long long fifoCounter;
    int* pageTableArray;
    int       pageTableSize = 0;

    void loadPage(int pageNumber, int slot);
    void savePage(int slot);
    int  findPage(int pageNumber);
    int  getAvailableSlot();
    int  getSlot(int index, bool markModified);
};