#include "../Interfaces/PagedArray.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <climits>

using namespace std;

static long long ceilPages(long long fileSizeLL, int pageSize) { //calcula la cantidasdde paginas
    return (fileSizeLL + pageSize - 1) / pageSize;
}

PagedArray::PagedArray(const std::string& fname, int pSize, int pCount)
    : filename(fname), pageSize(pSize), pageCount(pCount),
    pageFaults(0), pageHits(0), fileSizeBytes(0), totalInts(0),
    totalPages(0), fifoCounter(0)
{
    file.open(filename, ios::binary | ios::in | ios::out);
    if (!file.is_open()) {
        file.open(filename, ios::binary | ios::in);
        if (!file.is_open()) {
            cerr << "Error: No se pudo abrir " << filename << endl;
            exit(1);
        }
        cerr << "Advertencia: Archivo en modo solo lectura" << endl;
    }

    file.seekg(0, ios::end);
    fileSizeBytes = static_cast<long long>(file.tellg());
    file.seekg(0, ios::beg);

    if (fileSizeBytes <= 0) {
        cerr << "Error: Archivo vacio" << endl;
        exit(1);
    }

    totalInts = fileSizeBytes / sizeof(int);
    totalPages = ceilPages(fileSizeBytes, pageSize);

    std::cout << "Archivo abierto: " << fileSizeBytes << " bytes" << endl;
    std::cout << "pageSize: " << pageSize << " bytes" << endl;
    std::cout << "Total paginas en archivo: " << totalPages << endl;
    std::cout << "pageCount (cache): " << pageCount << " paginas" << endl;

    if (pageSize % sizeof(int) != 0) {
        cerr << "Error: pageSize no es multiplo de 4" << endl;
        exit(1);
    }

    // Inicializar pageTableArray
    pageTableArray = new int[totalPages];
    for (long long i = 0; i < totalPages; i++) {
        pageTableArray[i] = -1;
    }

    memoryPages = new Page[pageCount];
    int intsPerPage = pageSize / sizeof(int);
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
    if (pageNumber < 0 || pageNumber >= (int)totalPages) {
        cerr << "Error: pagina invalida " << pageNumber << endl;
        exit(1);
    }

    long long pos = static_cast<long long>(pageNumber) * pageSize;
    int bytesALeer = (int)min((long long)pageSize, fileSizeBytes - pos);

    file.seekg(static_cast<streampos>(pos));
    if (!file.good()) {
        cerr << "Error: seekg fallo cargando pagina " << pageNumber << endl;
        exit(1);
    }

    memset(memoryPages[slot].data, 0, pageSize);
    file.read(reinterpret_cast<char*>(memoryPages[slot].data), bytesALeer);
    if (!file.good() && !file.eof()) {
        cerr << "Error: read fallo en pagina " << pageNumber << endl;
        exit(1);
    }
    if (file.eof()) file.clear();

    memoryPages[slot].pageNumber = pageNumber;
    memoryPages[slot].loaded = true;
    memoryPages[slot].modified = false;
    memoryPages[slot].loadOrder = fifoCounter++;

    pageTableArray[pageNumber] = slot;
}

void PagedArray::savePage(int slot) {
    if (!memoryPages[slot].loaded || !memoryPages[slot].modified) return;
    int pageNum = memoryPages[slot].pageNumber;
    if (pageNum < 0 || !file.is_open()) return;

    long long pos = static_cast<long long>(pageNum) * pageSize;
    int bytesAEscribir = (int)min((long long)pageSize, fileSizeBytes - pos);

    file.seekp(static_cast<streampos>(pos));
    if (!file.good()) {
        cerr << "Error: seekp fallo guardando pagina " << pageNum << endl;
        exit(1);
    }

    file.write(reinterpret_cast<char*>(memoryPages[slot].data), bytesAEscribir);
    if (!file.good()) {
        cerr << "Error: write fallo en pagina " << pageNum << endl;
        exit(1);
    }

    memoryPages[slot].modified = false;
}

int PagedArray::findPage(int pageNumber) {
    if (pageNumber < 0 || pageNumber >= totalPages) return -1;

    int slot = pageTableArray[pageNumber];

    if (slot >= 0 && slot < pageCount &&
        memoryPages[slot].loaded &&
        memoryPages[slot].pageNumber == pageNumber) {
        return slot;
    }

    if (slot >= 0) {
        pageTableArray[pageNumber] = -1;
    }
    return -1;
}

int PagedArray::getAvailableSlot() {
    for (int i = 0; i < pageCount; i++) {
        if (!memoryPages[i].loaded) {
            if (memoryPages[i].pageNumber >= 0) {
                pageTableArray[memoryPages[i].pageNumber] = -1;
            }
            return i;
        }
    }

    int oldest = 0;
    long long minOrder = memoryPages[0].loadOrder;

    for (int i = 1; i < pageCount; i++) {
        if (memoryPages[i].loadOrder < minOrder) {
            minOrder = memoryPages[i].loadOrder;
            oldest = i;
        }
    }

    int oldPageNum = memoryPages[oldest].pageNumber;
    if (oldPageNum >= 0) {
        pageTableArray[oldPageNum] = -1;
    }

    savePage(oldest);
    memoryPages[oldest].loaded = false;

    return oldest;
}

int PagedArray::getSlot(int index, bool markModified) {
    int intsPerPage = pageSize / sizeof(int);

    if (index >= totalInts) {
        static bool warned = false;
        if (!warned) {
            cerr << "ADVERTENCIA: indice fuera de rango" << endl;
            warned = true;
        }
        index = totalInts - 1;
    }

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

    if (markModified) {
        memoryPages[slot].modified = true;
    }

    return slot;
}

int& PagedArray::operator[](int index) {
    int intsPerPage = pageSize / sizeof(int);
    int slot = getSlot(index, true);
    int offset = index % intsPerPage;
    return memoryPages[slot].data[offset];
}

int PagedArray::read(int index) {
    int intsPerPage = pageSize / sizeof(int);
    int slot = getSlot(index, false);
    int offset = index % intsPerPage;
    return memoryPages[slot].data[offset];
}

int* PagedArray::getPagePtr(int index, int& pageStart, int& pageEnd) {
    int intsPerPage = pageSize / sizeof(int);
    int slot = getSlot(index, true);
    int pageNum = memoryPages[slot].pageNumber;
    pageStart = pageNum * intsPerPage;
    pageEnd = (pageNum + 1) * intsPerPage - 1;
    return memoryPages[slot].data;
}

void PagedArray::flush() {
    for (int i = 0; i < pageCount; i++) {
        if (memoryPages[i].loaded && memoryPages[i].modified) {
            savePage(i);
        }
    }
    if (file.is_open()) {
        file.flush();
    }
}

long long PagedArray::getPageFaults() const {
    return pageFaults.load(memory_order_relaxed);
}

long long PagedArray::getPageHits() const {
    return pageHits.load(memory_order_relaxed);
}