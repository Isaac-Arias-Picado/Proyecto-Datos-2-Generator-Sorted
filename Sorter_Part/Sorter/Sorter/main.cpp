#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <sys/stat.h>
#include "Interfaces/Sorter.h"
#include "Interfaces/PagedArray.h"

using namespace std;
using namespace std::chrono;

bool fileExists(const string& path) { // Verificar si el archivo existe usando, devuleve el path
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool algoritmoCorrecto(const string& alg) { //verifica si el algoritmo es correcto
    return alg == "QS" || alg == "SS" || alg == "MS" || alg == "TS" || alg == "RS";
}

bool copiarArchivo(const string& src, const string& dst) {
    ifstream in(src, ios::binary);
    ofstream out(dst, ios::binary);
    if (!in.is_open() || !out.is_open()) return false;

    const int BUF = 1 << 20;
    char* buf = new char[BUF];
    bool exito = true;

    while (in.read(buf, BUF) || in.gcount() > 0) {
        out.write(buf, in.gcount());
        if (!out.good()) {
            exito = false;
            break;
        }
    }

    delete[] buf;
    return exito;
}

bool generarArchivoTXT(PagedArray& arr, const string& txtPath, long long totalEnteros) {
    ofstream outTxt(txtPath);

    if (!outTxt.is_open()) {
        cerr << "Error: No se pudo crear el archivo TXT: " << txtPath << endl;
        return false;
    }

    const int BUFFER_SIZE = 10000;
    string buffer;
    buffer.reserve(BUFFER_SIZE * 12);

    for (long long i = 0; i < totalEnteros; i++) {
        int valor = arr.read(i);
        buffer += to_string(valor);

        if (i < totalEnteros - 1) {
            buffer += ", ";
        }

        if ((i + 1) % BUFFER_SIZE == 0 || i == totalEnteros - 1) {
            outTxt << buffer;
            buffer.clear();
        }
    }

    outTxt << "\n";
    outTxt.close();

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
            << "'. Opciones: QS, SS, MS, TS, RS" << endl; return 1;
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
        else if (algoritmo == "RS") sorter.radixSort(arr, totalEnteros);
        else if (algoritmo == "TS") sorter.timSort(arr, 0, totalEnteros - 1);
        else if (algoritmo == "MS") sorter.mergeSort(arr, totalEnteros);
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