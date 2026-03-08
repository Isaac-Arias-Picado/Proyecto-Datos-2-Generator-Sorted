#include <iostream>
#include <fstream>
#include <string>
#include "Scripts/Generator.cpp"

using namespace std;

int main(int argc, char* argv[]) {
    string sizeParam = "";
    string outputPath = "";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-size" && i + 1 < argc) sizeParam = argv[++i];
        else if (arg == "-output" && i + 1 < argc) outputPath = argv[++i];
    }

    // === MODO GENERADOR ===
    if (!sizeParam.empty()) {
        if (outputPath.empty()) outputPath = "datos_generados.bin";

        Generator gen;
        ofstream out(outputPath, ios::binary);
        if (out.is_open()) {
            cout << "Generando archivo: " << outputPath << " (Tamano: " << sizeParam << ")" << endl;
            gen.agregar_num_archivo(out, sizeParam);
            out.close();
            cout << "Generacion completada" << endl;
        }
        else {
            cout << "Error: No se pudo crear el archivo " << outputPath << endl;
        }
        return 0; 
    }

    cout << "Error: Use -size para generar." << endl;
    return 1;
}

//comando de ejecucion
// .\Generator.exe -size tamano -output direccion