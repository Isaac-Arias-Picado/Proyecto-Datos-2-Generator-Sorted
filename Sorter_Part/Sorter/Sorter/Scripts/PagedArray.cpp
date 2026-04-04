/*
 * PAGED ARRAY (Array Paginado):
 *   Un array virtual que es más grande que la memoria RAM disponible.
 *   Los datos residen en disco pero se accede a ellos como si estuvieran
 *   completamente en memoria. Solo una parte (ventana) está en RAM.
 *
 * PAGE (Página):
 *   Bloque contiguo de datos del tamaño pageSize bytes. Es la unidad mínima
 *   de transferencia entre disco y memoria. Cada página contiene intsPerPage
 *   números enteros.
 *
 * SLOT (Ranura/Hueco):
 *   Posición en el array memoryPages[] que representa un marco de página
 *   en memoria RAM. Cada slot puede contener UNA página a la vez.
 *   El número de slots = pageCount (memoria física disponible).
 *
 * PAGE TABLE (Tabla de Páginas):
 *   Array pageTableArray[] que mapea números de página lógicos a slots físicos.
 *   - Índice = número de página (0, 1, 2...)
 *   - Valor = slot en memoria donde está cargada (-1 si no está cargada)
 *
 * OFFSET (Desplazamiento):
 *   Posición de un int DENTRO de una página. Se calcula como index % intsPerPage.  
 */


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
	file.open(filename, ios::binary | ios::in | ios::out);  //abre el archivo en modo binario para lectura y escritura
    if (!file.is_open()) {
        cerr << "Error: No se pudo abrir " << filename << endl;
        exit(1);
    }

	file.seekg(0, ios::end); //mueve el puntero al final del archivo para obtener su tamaño
	fileSizeBytes = static_cast<long long>(file.tellg()); //obtiene el tamaño del archivo en bytes
	file.seekg(0, ios::beg); //mueve el puntero al inicio del archivo para futuras operaciones de lectura/escritura

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

	pageTableArray = new int[pageTableSize]; //crea array de paginas, indice es numero de pagina, valor es slot en memoria o -1 si no esta cargada
    for (int i = 0; i < pageTableSize; i++)
		pageTableArray[i] = -1; //inicializa con -1 para indicar que no hay pagina cargada

    memoryPages = new Page[pageCount]; //crea array de slots de memoria, para las paginas cargadas
	for (int i = 0; i < pageCount; i++) { //inicializa cada slot de memoria con datos vacios
        memoryPages[i].pageNumber = -1;
        memoryPages[i].loaded = false;
        memoryPages[i].modified = false;
        memoryPages[i].loadOrder = 0;
        memoryPages[i].data = new int[intsPerPage]();
    }
}

PagedArray::~PagedArray() {
	flush(); //guarda cualquier pagina modificada antes de cerrar
	if (file.is_open()) file.close(); //libera todos los recursos asociados al archivo
    for (int i = 0; i < pageCount; i++) 
        delete[] memoryPages[i].data;
    delete[] memoryPages;
    delete[] pageTableArray;
}

int& PagedArray::operator[](int index) { //me devuelve una referencia al entero en la posicion index
    int slot = getSlot(index, true);
    int offset = index % intsPerPage;
    return memoryPages[slot].data[offset]; 
}

int PagedArray::getSlot(int index, bool markModified) {
    int pageNeeded = index / intsPerPage; //redondea hacia abajo
	int slot = findPage(pageNeeded); //busca si la pagina ya esta cargada en memoria y devuelve su slot, o -1 si no esta cargada

    if (slot != -1) {
		pageHits.fetch_add(1, memory_order_relaxed); //si esta cargada, incrementa contador de page hits
    }
    else {
		pageFaults.fetch_add(1, memory_order_relaxed); //si no esta cargada, incrementa contador de page faults
		slot = getAvailableSlot(); //obtiene un slot disponible en memoria, o a reemplazar
        loadPage(pageNeeded, slot); //carga la pagina
    }

    if (markModified) memoryPages[slot].modified = true; //marca modificado dependiendo del parametro
    return slot;
}

int PagedArray::findPage(int pageNumber) {
    if (pageNumber < 0 || pageNumber >= pageTableSize) return -1;
    int slot = pageTableArray[pageNumber];
    if (slot >= 0 && slot < pageCount &&memoryPages[slot].loaded &&
        memoryPages[slot].pageNumber == pageNumber)
        return slot;
    if (slot >= 0) pageTableArray[pageNumber] = -1;
    return -1;
}


void PagedArray::loadPage(int pageNumber, int slot) {
    long long pos = (long long)pageNumber * pageSize;
    int       bytesToRead = (int)min((long long)pageSize, fileSizeBytes - pos);

    file.seekg((streampos)pos); 
    file.read(reinterpret_cast<char*>(memoryPages[slot].data), bytesToRead);
	if (file.eof()) file.clear(); //limpia el estado de EOF para futuras operaciones

    if (bytesToRead < pageSize)
        memset(reinterpret_cast<char*>(memoryPages[slot].data) + bytesToRead,
			0, pageSize - bytesToRead); //rellena con ceros el resto de la pagina si el archivo no tiene suficientes bytes

    memoryPages[slot].pageNumber = pageNumber;
    memoryPages[slot].loaded = true;
    memoryPages[slot].modified = false;
    memoryPages[slot].loadOrder = fifoCounter++;
	pageTableArray[pageNumber] = slot; //actualiza la tabla de paginas para indicar que esta pagina ahora esta cargada en este slot
}

void PagedArray::savePage(int slot) {
    if (!memoryPages[slot].loaded || !memoryPages[slot].modified) return;
    int pageNum = memoryPages[slot].pageNumber;
    if (pageNum < 0) return;

	long long pos = (long long)pageNum * pageSize; //calcula la posicion en bytes del inicio de la pagina en el archivo
    int       bytesToWrite = (int)min((long long)pageSize, fileSizeBytes - pos); 

	file.seekp((streampos)pos); //mueve el puntero de escritura a la posicion calculada
	file.write(reinterpret_cast<char*>(memoryPages[slot].data), bytesToWrite); //escribe los datos de la pagina en el archivo
	memoryPages[slot].modified = false; //marca la pagina como no modificada ya que los cambios se han guardado en el archivo
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

int PagedArray::read(int index) { //hace lo mismo que operator[] pero sin marcar modificado
    int slot = getSlot(index, false);
    int offset = index % intsPerPage; 
    return memoryPages[slot].data[offset];
}

int* PagedArray::getPagePtr(int index, int& pageStart, int& pageEnd) {
    int slot = getSlot(index, true); //marca modificado
    int pageNum = memoryPages[slot].pageNumber; 
    pageStart = pageNum * intsPerPage;
	pageEnd = pageStart + intsPerPage - 1; //se resta 1 por que el indice es 0-based
    return memoryPages[slot].data;
}

void PagedArray::flush() {
	for (int i = 0; i < pageCount; i++) //guarda todas las paginas modificadas
        savePage(i);
	if (file.is_open()) file.flush(); //asegura que los datos se escriban en disco
}

long long PagedArray::getPageFaults() const { return pageFaults.load(memory_order_relaxed); }
long long PagedArray::getPageHits()   const { return pageHits.load(memory_order_relaxed); }