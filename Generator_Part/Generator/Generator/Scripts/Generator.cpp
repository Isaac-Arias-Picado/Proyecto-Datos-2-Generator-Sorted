#include <iostream>
#include <fstream>
#include <random> 
#include <limits> 

using namespace std;

class Generator {
public:
    void agregar_num_archivo(ofstream& archivo, string size) {
        if (archivo.is_open()) {
			// Configuración para generar números aleatorios
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<int> dis(0, numeric_limits<int>::max());

			long long cantidad = (256LL * 1024 * 1024) / sizeof(int); //Usa SMALL por defecto
            if (size == "SMALL")   // 256 MB
                cantidad = (256LL * 1024 * 1024) / sizeof(int);
            else if (size == "MEDIUM")  // 512 MB
                cantidad = (512LL * 1024 * 1024) / sizeof(int);
            else if (size == "LARGE")   // 1 GB
                cantidad = (1024LL * 1024 * 1024) / sizeof(int);
            else if (size == "TESTING") {
				cantidad = 1000; // Solo 1000 enteros para pruebas
            }

            else {
                cout << "Tamano no reconocido, Usando SMALL por defecto" << endl;
            }

            cout << "Generando " << cantidad << " enteros para el tamano " << size << "..." << endl;

            for (long long i = 0; i < cantidad; i++) {
                int num = dis(gen);
                archivo.write(reinterpret_cast<const char*>(&num), sizeof(int));
            }

            cout << "Archivo generado" << endl;
        }
    }
};