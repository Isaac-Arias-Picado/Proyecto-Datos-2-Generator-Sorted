#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sys/stat.h>
#include <climits>
#include "Interfaces/Sorter.h"
#include "Interfaces/PagedArray.h"
#include <windows.h>

using namespace std;
using namespace std::chrono;

bool fileExists(const string& path) { // Verificar si el archivo existe usando, devuleve el path
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool algoritmoCorrecto(const string& alg) { //verifica si el algoritmo es correcto
    return alg == "QS" || alg == "SS" || alg == "TS" || alg == "MS" || alg == "RS";
}

bool copiarArchivo(const string& src, const string& dst) { //copia el archivo usando la libraria de windows
    return CopyFileA(src.c_str(), dst.c_str(), FALSE) != 0;
}

bool generarArchivoTXT(PagedArray& arr, const string& txtPath, long long totalEnteros) {

    HANDLE hFile = CreateFileA(txtPath.c_str(), GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS,                           // Sobrescribe si ya existe
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,  // Optimización para escritura secuencial
        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Error: No se pudo crear " << txtPath << endl;
        return false;                           
    }
          // Inicia medición de tiempo de ejecución
    const int BUF = 1 << 23;                     // Buffer de (16 MB)
    char* buf = new char[BUF + 32];              // +32 bytes extra por seguridad (para números grandes)
    int pos = 0;                              
    DWORD written;                               // Variable para almacenar bytes escritos (requerida por WriteFile)

    auto writeInt = [&](int val) {
        if (val < 0) { buf[pos++] = '-'; val = -val; }
        char tmp[12];
        int len = 0;
        do { tmp[len++] = '0' + (val % 10); val /= 10; } while (val);
        for (int i = len - 1; i >= 0; i--) buf[pos++] = tmp[i];
        };

    for (long long i = 0; i < totalEnteros; i++) {
        writeInt(arr.read(i));
        if (i < totalEnteros - 1) {
            buf[pos++] = ',';
            buf[pos++] = ' ';
        }
        if (pos >= BUF - 32) {
            WriteFile(hFile, buf, (DWORD)pos, &written, NULL);
            pos = 0;
        }
    }

    buf[pos++] = '\n';                           // Añade salto de línea al final
    WriteFile(hFile, buf, (DWORD)pos, &written, NULL);  
    CloseHandle(hFile);                          
    delete[] buf;                                // Libera la memoria del buffer
 
    return true;                                
}

int main(int argc, char* argv[]) {
    string inputPath;
    string outputPath;
    string algoritmo = "";
    int tamanoPagina = 0;
    int cantidadPaginas = 0;
    auto inicio_total = steady_clock::now();

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-input" && i + 1 < argc) inputPath = argv[++i];
        else if (arg == "-output" && i + 1 < argc) outputPath = argv[++i];
        else if (arg == "-alg" && i + 1 < argc) algoritmo = argv[++i];
        else if (arg == "-pageSize" && i + 1 < argc) tamanoPagina = stoi(argv[++i]);
        else if (arg == "-pageCount" && i + 1 < argc) cantidadPaginas = stoi(argv[++i]);
    }

    tamanoPagina = tamanoPagina * (int)sizeof(int);

    if (inputPath.empty()) {
        cerr << "Error: falta argumento -input" << endl; return 1;
    }
    if (outputPath.empty()) {
        cerr << "Error: falta argumento -output" << endl; return 1;
    }
    if (!algoritmoCorrecto(algoritmo)) {
        cerr << "Error: algoritmo no reconocido '" << algoritmo
            << "'. Opciones: QS, SS, TS, MS, RS" << endl; return 1;
    }
    if (tamanoPagina <= 0 || tamanoPagina % (int)sizeof(int) != 0) {
        cerr << "Error: pageSize debe ser positivo y multiplo de " << sizeof(int) << endl; return 1;
    }
    if (cantidadPaginas <= 0) {
        cerr << "Error: pageCount debe ser mayor que 0" << endl; return 1;
    }
    if (!fileExists(inputPath)) {
        cerr << "Error: El archivo de entrada no existe: " << inputPath << endl; return 1;
    }

    // Construir paths de salida
    string nombreBase = outputPath;
    if (nombreBase.size() >= 4 && nombreBase.substr(nombreBase.size() - 4) == ".bin")
        nombreBase = nombreBase.substr(0, nombreBase.size() - 4);
    string binPath = nombreBase + ".bin";
    string txtPath = nombreBase + ".txt";

    // Tamaño del archivo
    long long totalEnterosLL = 0;
    {
		ifstream in(inputPath, ios::binary | ios::ate); //abre archivo en modo binario y se posiciona al final para obtener su tamaño
        if (!in.is_open()) {
            cerr << "Error: No se pudo abrir " << inputPath << endl; return 1;
        }
        long long totalBytes = in.tellg();
        if (totalBytes == 0) {
            cerr << "Error: El archivo de entrada esta vacio" << endl; return 1;
        }
        totalEnterosLL = totalBytes / sizeof(int);
    }

    if (totalEnterosLL > INT_MAX) {
        cerr << "Error: Archivo demasiado grande" << endl; return 1;
    }
    int totalEnteros = static_cast<int>(totalEnterosLL);

    if (!copiarArchivo(inputPath, binPath)) {
        cerr << "Error: No se pudo copiar el archivo" << endl; return 1;
    }

    PagedArray arr(binPath, tamanoPagina, cantidadPaginas);
    Sorter sorter;

    auto inicio_algoritmo = steady_clock::now();
    try {
        if (algoritmo == "QS") sorter.quickSort(arr, 0, totalEnteros - 1);
        else if (algoritmo == "SS") sorter.shellSort(arr, 0, totalEnteros - 1);
        else if (algoritmo == "TS") sorter.timSort(arr, 0, totalEnteros - 1);
        else if (algoritmo == "MS") sorter.mergeSort(arr, 0, totalEnteros - 1);
        else if (algoritmo == "RS") sorter.radixSort(arr, 0, totalEnteros - 1);
    }
    catch (const exception& e) {
        cerr << "Error durante ordenamiento: " << e.what() << endl; return 1;
    }
    auto fin_algoritmo = steady_clock::now();
    double tiempoAlgoritmo = duration<double>(fin_algoritmo - inicio_algoritmo).count();

    arr.flush();

    long long hits = arr.getPageHits();
    long long faults = arr.getPageFaults();

    generarArchivoTXT(arr, txtPath, totalEnterosLL);

    double tiempoReal = duration<double>(steady_clock::now() - inicio_total).count();

    cout << "  ORDENAMIENTO COMPLETADO" << endl;
    cout << "  Algoritmo          : " << algoritmo << endl;
    cout << "  pageSize           : " << tamanoPagina << " bytes ("
        << tamanoPagina / (int)sizeof(int) << " ints)" << endl;
    cout << "  pageCount          : " << cantidadPaginas << endl;
    cout << "  Tiempo total       : " << fixed << setprecision(2) << tiempoReal << " s" << endl;
    cout << "  Tiempo ordenamiento: " << fixed << setprecision(2) << tiempoAlgoritmo << " s" << endl;
    cout << "  Page Hits          : " << hits << endl;
    cout << "  Page Faults        : " << faults << endl;

    return 0;
}