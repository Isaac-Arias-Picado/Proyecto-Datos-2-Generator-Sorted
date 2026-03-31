#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <sys/stat.h>
#include <climits>
#include "Interfaces/Sorter.h"
#include "Interfaces/PagedArray.h"

using namespace std;
using namespace std::chrono;

bool fileExists(const string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool algoritmoCorrecto(const string& alg) {
    return alg == "QS" || alg == "SS" || alg == "TS" || alg == "MS" || alg == "RS";
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

    std::cout << "\nGenerando archivo TXT legible: " << txtPath << " ..." << endl;

    const int BUFFER_SIZE = 10000;
    string buffer;
    buffer.reserve(BUFFER_SIZE * 12);

    long long progresoTXT = 0;
    auto inicioTXT = steady_clock::now();

    for (long long i = 0; i < totalEnteros; i++) {
        int valor = arr.read(i);
        buffer += to_string(valor);

        if (i < totalEnteros - 1) {
            buffer += ", ";
        }

        if ((i + 1) % BUFFER_SIZE == 0 || i == totalEnteros - 1) {
            outTxt << buffer;
            buffer.clear();

            progresoTXT += BUFFER_SIZE;
            if (progresoTXT % (BUFFER_SIZE * 10) == 0) {
                double pct = (i + 1) * 100.0 / totalEnteros;
                auto ahora = steady_clock::now();
                double elapsed = duration<double>(ahora - inicioTXT).count();
                std::cout << "  TXT: " << (i + 1) << "/" << totalEnteros
                    << " (" << fixed << setprecision(1) << pct << "%)"
                    << " - " << setprecision(1) << elapsed << "s\r" << flush;
            }
        }
    }

    outTxt << "\n";
    outTxt.close();

    auto finTXT = steady_clock::now();
    double tiempoTXT = duration<double>(finTXT - inicioTXT).count();

    std::cout << "\nArchivo TXT generado en " << setprecision(1) << tiempoTXT << " segundos" << endl;
    return true;
}

int main(int argc, char* argv[]) {
    string inputPath = "";
    string outputPath = "";
    string algoritmo = "QS";
    int tamanoPagina = 16384;
    int cantidadPaginas = 512;
    auto inicio_total = steady_clock::now();

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-input" && i + 1 < argc) {
            inputPath = argv[++i];
        }
        else if (arg == "-output" && i + 1 < argc) {
            outputPath = argv[++i];
        }
        else if (arg == "-alg" && i + 1 < argc) {
            algoritmo = argv[++i];
        }
        else if (arg == "-pageCount" && i + 1 < argc) {
            cantidadPaginas = stoi(argv[++i]);
        }
        else if (arg == "-pageSize" && i + 1 < argc) {
            tamanoPagina = stoi(argv[++i]);
        }
    }

    if (inputPath.empty()) {
        cerr << "Error: falta argumento -input" << endl;
        return 1;
    }

    if (outputPath.empty()) {
        cerr << "Error: falta argumento -output" << endl;
        return 1;
    }

    if (algoritmo.empty() || !algoritmoCorrecto(algoritmo)) {
        cerr << "Error: algoritmo no reconocido '" << algoritmo
            << "'. Opciones: QS, SS, TS, MS, RS" << endl;
        return 1;
    }

    if (tamanoPagina <= 0 || tamanoPagina % (int)sizeof(int) != 0) {
        cerr << "Error: pageSize debe ser positivo y multiplo de " << sizeof(int) << endl;
        return 1;
    }

    if (cantidadPaginas <= 0) {
        cerr << "Error: pageCount debe ser mayor que 0" << endl;
        return 1;
    }

    if (!fileExists(inputPath)) {
        cerr << "Error: El archivo de entrada no existe: " << inputPath << endl;
        return 1;
    }

    string nombreBase = outputPath;
    // Eliminar extensión .bin si existe
    if (nombreBase.size() >= 4 && nombreBase.substr(nombreBase.size() - 4) == ".bin") {
        nombreBase = nombreBase.substr(0, nombreBase.size() - 4);
    }
    string binPath = nombreBase + ".bin";
    string txtPath = nombreBase + ".txt";

    // Obtener tamaño del archivo
    long long totalBytes = 0;
    long long totalEnterosLL = 0;
    {
        ifstream in(inputPath, ios::binary | ios::ate);
        if (!in.is_open()) {
            cerr << "Error: No se pudo abrir " << inputPath << endl;
            return 1;
        }
        totalBytes = in.tellg();
        if (totalBytes == 0) {
            cerr << "Error: El archivo de entrada esta vacio" << endl;
            return 1;
        }
        totalEnterosLL = totalBytes / sizeof(int);
    }

    // Verificar que cabe en int
    if (totalEnterosLL > INT_MAX) {
        cerr << "Error: Archivo demasiado grande para int" << endl;
        return 1;
    }

    int totalEnteros = static_cast<int>(totalEnterosLL);

    // Mostrar configuración 
    std::cout << "\n==========================================" << endl;
    std::cout << "   PAGED ARRAY SORTER" << endl;
    std::cout << "==========================================" << endl;
    std::cout << "Input: " << inputPath << endl;
    std::cout << "Output binario: " << binPath << endl;
    std::cout << "Output TXT   : " << txtPath << endl;
    std::cout << "Algoritmo: " << algoritmo << endl;
    std::cout << "pageSize: " << tamanoPagina << " bytes ("
        << (tamanoPagina / 1024) << "KB)" << endl;
    std::cout << "pageCount: " << cantidadPaginas << " paginas" << endl;
    std::cout << "Cache total: " << (tamanoPagina * cantidadPaginas) / (1024 * 1024)
        << " MB" << endl;
    std::cout << "Archivo: " << totalEnteros << " enteros ("
        << (totalBytes / (1024 * 1024)) << " MB)" << endl;
    std::cout << "==========================================\n" << endl;

    // PASO 1: Copiar archivo original a la ubicación de salida (binario)
    std::cout << "PASO 1: Copiando " << inputPath << " -> " << binPath << " ..." << endl;
    if (!copiarArchivo(inputPath, binPath)) {
        cerr << "Error: No se pudo copiar el archivo" << endl;
        return 1;
    }
    std::cout << "Copia completada." << endl;

    // PASO 2: Ordenar el archivo
    std::cout << "\nPASO 2: Ordenando archivo..." << endl;

    PagedArray arr(binPath, tamanoPagina, cantidadPaginas);
    Sorter sorter;

    auto inicio_algoritmo = steady_clock::now();

    try {
        if (algoritmo == "QS") {
            sorter.quickSort(arr, 0, totalEnteros - 1);
        }
        else if (algoritmo == "SS") {
            sorter.shellSort(arr, 0, totalEnteros - 1);
        }
        else if (algoritmo == "TS") {
            sorter.timSort(arr, 0, totalEnteros - 1);
        }
        else if (algoritmo == "MS") {
            sorter.mergeSort(arr, 0, totalEnteros - 1);
        }
        else if (algoritmo == "RS") {
            sorter.radixSort(arr, 0, totalEnteros - 1);
        }
    }
    catch (const exception& e) {
        cerr << "Error durante ordenamiento: " << e.what() << endl;
        return 1;
    }

    auto fin_algoritmo = steady_clock::now();
    double tiempoalgoritmo = duration<double>(fin_algoritmo - inicio_algoritmo).count();

    // PASO 3: Guardar cambios en disco
    std::cout << "\nPASO 3: Guardando cambios en disco..." << endl;
    arr.flush();

    long long hits = arr.getPageHits();
    long long faults = arr.getPageFaults();
    long long totalAccesos = hits + faults;

    // PASO 4: Generar archivo TXT legible (usando txtPath)
    generarArchivoTXT(arr, txtPath, totalEnterosLL);

    auto fin_total = steady_clock::now();
    double tiempoReal = duration<double>(fin_total - inicio_total).count();

    // Mostrar resultados del ordenamiento
    std::cout << "\n======================================================" << endl;
    std::cout << "  ORDENAMIENTO COMPLETADO" << endl;
    std::cout << "  Algoritmo        : " << algoritmo << endl;
    std::cout << "  Configuración    : pageSize=" << tamanoPagina << ", pageCount=" << cantidadPaginas << endl;
    std::cout << "  Tiempo total     : " << fixed << setprecision(2) << tiempoReal << " segundos" << endl;
    std::cout << "  Tiempo ordenamiento: " << fixed << setprecision(2) << tiempoalgoritmo << " segundos" << endl;
    std::cout << "  Page Hits        : " << hits << endl;
    std::cout << "  Page Faults      : " << faults << endl;
    std::cout << "======================================================" << endl;

    return 0;
}