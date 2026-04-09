#include "../Interfaces/Sorter.h"
#include "../Interfaces/PagedArray.h"
#include <climits>
#include <cstdio>

using namespace std;

int partitionHoare(PagedArray& arr, int low, int high) {
    // Elegir pivote (mediana de tres para mejor rendimiento)
    int mid = low + (high - low) / 2;

    // Mediana de tres
    int pivot;
    int pivotIdx;

    int a = arr[low];
    int b = arr[mid];
    int c = arr[high];

    if ((a - b) * (c - a) >= 0) {
        pivot = a;
        pivotIdx = low;
    }
    else if ((b - a) * (c - b) >= 0) {
        pivot = b;
        pivotIdx = mid;
    }
    else {
        pivot = c;
        pivotIdx = high;
    }

    // Mover pivote al inicio
    if (pivotIdx != low) {
        int temp = arr[low];
        arr[low] = arr[pivotIdx];
        arr[pivotIdx] = temp;
    }

    // Partición Hoare
    int i = low + 1;
    int j = high;

    while (true) {
        // Mover i hacia la derecha mientras arr[i] <= pivot
        while (i <= high && arr[i] < pivot) {
            i++;
        }

        // Mover j hacia la izquierda mientras arr[j] > pivot
        while (j > low && arr[j] >= pivot) {
            j--;
        }

        if (i >= j) {
            break;
        }

        // Intercambiar elementos
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;

        i++;
        j--;
    }

    // Colocar pivote en su posición final
    int temp = arr[low];
    arr[low] = arr[j];
    arr[j] = temp;

    return j;
}

void Sorter::quickSort(PagedArray& arr, int left, int right) {
    if (left < right) {
        int pivotIdx = partitionHoare(arr, left, right);
        quickSort(arr, left, pivotIdx - 1);  
        quickSort(arr, pivotIdx + 1, right);
    }
}
//shell
void Sorter::shellSort(PagedArray& arr, int left, int right) {
    int n = right - left + 1;
    static const int ciura[] = {
        1, 4, 10, 23, 57, 132, 301, 701, 1750, 3937, 8858, 19930,
        44842, 100894, 227011, 510774, 1149241, 2585792, 5818032,
        13090572, 29453787, 66271020, 149109795, 335497038, 754868335,
        1698453753
    };
    const int numGaps = sizeof(ciura) / sizeof(ciura[0]);
    int gaps[32], ng = 0;
    for (int k = 0; k < numGaps; k++)
        if (ciura[k] < n) gaps[ng++] = ciura[k];

    for (int idx = ng - 1; idx >= 0; idx--) {
        int gap = gaps[idx];
        for (int i = left + gap; i <= right; i++) {
            int temp = arr[i];
            int j = i;
            while (j >= left + gap && arr[j - gap] > temp) {
                arr[j] = arr[j - gap];
                j -= gap;
            }
            arr[j] = temp;
        }
    }
}
//timsor
void Sorter::timInsertionSort(PagedArray& arr, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void Sorter::timMerge(PagedArray& arr, int left, int mid, int right) {
    const int BUFFER_SIZE = 4096; // fijo, no crece
    int buffer[BUFFER_SIZE];

    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid || j <= right) {
        // Llenar buffer desde el lado izquierdo
        int count = 0;
        while (count < BUFFER_SIZE && i <= mid) {
            buffer[count++] = arr[i++];
        }

        // Merge del buffer con el lado derecho, escribiendo desde k
        int bufIdx = 0;
        while (bufIdx < count && j <= right) {
            if (buffer[bufIdx] <= arr[j]) {
                arr[k++] = buffer[bufIdx++];
            }
            else {
                arr[k++] = arr[j++];
            }
        }

        // Vaciar lo que quede del buffer
        while (bufIdx < count) {
            arr[k++] = buffer[bufIdx++];
        }
    }
}
void Sorter::timSort(PagedArray& arr, int left, int right) {
    int n = right - left + 1;
    const int RUN = 128;

    // Insertion sort para runs pequeños
    for (int i = left; i <= right; i += RUN) {
        if (i + RUN - 1 - right < 0) {
            timInsertionSort(arr, i, i + RUN - 1);
        }
        else
        {
            timInsertionSort(arr, i, right);
        }
    }

    quickSort(arr, left, right);
}
//Merge Sort
void Sorter::mergeHelper(PagedArray& arr, int left, int mid, int right,
    PagedArray& aux) {
    // Copia el rango completo al auxiliar
    for (int i = left; i <= right; i++)
        aux[i] = arr[i];

    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right)
        arr[k++] = (aux[i] <= aux[j]) ? aux[i++] : aux[j++];
    while (i <= mid)
        arr[k++] = aux[i++];
    while (j <= right)
        arr[k++] = aux[j++];
    // Si j sobra, ya están en su lugar en arr
}

void Sorter::mergeSortRec(PagedArray& arr, int left, int right, PagedArray& aux) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    mergeSortRec(arr, left, mid, aux);
    mergeSortRec(arr, mid + 1, right, aux);
    mergeHelper(arr, left, mid, right, aux);
}

void Sorter::mergeSort(PagedArray& arr, int size) {
    string pathTemp = "temp_merge.bin";

    ofstream tempFile(pathTemp, ios::binary);
    int cero = 0;
    for (int i = 0; i < size; i++)
        tempFile.write((char*)&cero, sizeof(int));
    tempFile.close();

    {
        PagedArray aux(pathTemp, 4 * 1024 * 1024, 256);
        mergeSortRec(arr, 0, size - 1, aux);
    }

    if (remove(pathTemp.c_str()) != 0)
        perror("Error al eliminar el archivo temporal");
}


//Radix Sort
void Sorter::countingSort(PagedArray& array, PagedArray& output, int size, int shift) {
    const int BASE = 256;
    int count[BASE] = { 0 };

    for (int i = 0; i < size; i++) {
        int byte = ((unsigned int)array[i] >> shift) & 0xFF;
        count[byte]++;
    }

    int acumulado = 0;
    for (int i = 0; i < BASE; i++) {
        int temp = count[i];
        count[i] = acumulado;
        acumulado += temp;
    }

    for (int i = 0; i < size; i++) {
        int byte = ((unsigned int)array[i] >> shift) & 0xFF;
        output[count[byte]++] = array[i];
    }
}

void Sorter::radixSort(PagedArray& array, int size) {
    string pathTemp = "temp_radix.bin";

    ofstream tempFile(pathTemp, ios::binary);
    int cero = 0;
    for (int i = 0; i < size; i++) {
        tempFile.write((char*)&cero, sizeof(int));
    }
    tempFile.close(); 

    {
        PagedArray aux(pathTemp, 4 * 1024 * 1024, 256);

        for (int shift = 0; shift < 32; shift += 8) {
            countingSort(array, aux, size, shift);

            for (int i = 0; i < size; i++) {
                array[i] = aux[i];
            }
        }
    } 

    if (remove(pathTemp.c_str()) != 0) {
        perror("Error al eliminar el archivo temporal");
    }
}