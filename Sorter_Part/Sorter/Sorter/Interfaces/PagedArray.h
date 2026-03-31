#pragma once
#include <string>
#include <fstream>
#include <atomic>  

using namespace std;

struct Page {
    int* data = nullptr;
    int pageNumber = -1;
    bool loaded = false;
    bool modified = false;
	long long loadOrder = 0; // Para implementar FIFO
};

class PagedArray {
public:
    PagedArray(const std::string& fname, int pageSize, int pageCount);
    ~PagedArray();

    // Acceso lectura/escritura
    int& operator[](int index);
    int  read(int index);

    void flush();
    long long getPageFaults() const;
    long long getPageHits()   const;
    long long size() const { return totalInts; }

    int* getPagePtr(int index, int& pageStart, int& pageEnd);
    int getIntsPerPage() const { return pageSize / (int)sizeof(int); }

private:
    string filename;
    fstream file;
    int pageSize;
    int pageCount;

    atomic<long long> pageFaults;
    atomic<long long> pageHits;

    long long fileSizeBytes;
    long long totalInts;
    long long totalPages;

    Page* memoryPages;
    long long fifoCounter;

    int* pageTableArray;  // Mapeo directo: página → slot
    int pageTableSize;    // tamaño del array = totalPages

    void loadPage(int pageNumber, int slot);
    void savePage(int slot);

    int  findPage(int pageNumber);  
    int  getAvailableSlot();
    int  getSlot(int index, bool markModified);
};