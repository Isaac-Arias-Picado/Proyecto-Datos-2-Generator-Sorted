#include <iostream>
#include <fstream>
#include <string>
#include "Scripts/Generator.cpp"
#include <chrono>

using namespace std;

int main(int argc, char* argv[]) {
    string sizeParam = "";
    string outputPath = "";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-size" && i + 1 < argc) sizeParam = argv[++i];
        else if (arg == "-output" && i + 1 < argc) outputPath = argv[++i];
    }
    if(outputPath.empty()) {
        cout << "Error: Use -output para designar salida." << endl;
        return 1;
    }
    if (!sizeParam.empty()) {
        if (outputPath.find(".bin") == string::npos) {
            outputPath += ".bin";
        }
        Generator gen;
		ofstream out(outputPath, ios::binary); //abre output en modo binario
        if (out.is_open()) {
            auto inicio = std::chrono::high_resolution_clock::now(); //cronometra el inicio
            cout << "Generando archivo: " << outputPath << " (Tamano: " << sizeParam << ")" << endl;
            gen.agregar_num_archivo(out, sizeParam);
            out.close();
			auto fin = std::chrono::high_resolution_clock::now(); //cronometra el fin
            auto duracion = std::chrono::duration_cast<std::chrono::milliseconds>(fin - inicio);
            cout << "Generacion completada: " << duracion.count() << " ms" << endl;
        }
        else {
            cout << "Error: No se pudo crear el archivo " << outputPath << endl;
        }
        return 0; 
    }

    cout << "Error: Use -size para generar (SMALL, MEDIUM, LARGE)." << endl;
    return 1;
}

//comando de ejecucion
// .\Generator.exe -size tamano -output direccion