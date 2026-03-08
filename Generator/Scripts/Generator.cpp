#include <iostream>
#include <fstream>
#include <random> 
#include <limits> 

using namespace std;

class Generator {
public:
    void generar_archivo(ofstream& archivo, string path) {
        archivo.open(path, ios::out | ios::binary);
    }

    void leer_archivo(ifstream& archivo) {
        if (archivo.is_open()) {
            int numero;
            while (archivo.read(reinterpret_cast<char*>(&numero), sizeof(int))) {
                cout << numero;
            }
        }
    }

    void agregar_num_archivo(ofstream& archivo, string size) {
        if (archivo.is_open()) {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<int> dis(0, numeric_limits<int>::max());

            long long cantidad = 0;
            if (size == "SMALL") // 512 MB
                cantidad = (512LL * 1024 * 1024) / sizeof(int);
            else if (size == "MEDIUM") // 1 GB
                cantidad = (1024LL * 1024 * 1024) / sizeof(int);
            else if (size == "LARGE") // 2 GB
                cantidad = (2048LL * 1024 * 1024) / sizeof(int);
            else if (size == "TESTING")
                cantidad = 100; // Solo 100 enteros para pruebas

            for (long long i = 0; i < cantidad; i++) {
                int num = dis(gen);
                archivo.write(reinterpret_cast<const char*>(&num), sizeof(int));
            }
        }
    }

    void limpiar_archivo() {
        ofstream archivo("ejemplo.txt", ios::trunc);
    }
};