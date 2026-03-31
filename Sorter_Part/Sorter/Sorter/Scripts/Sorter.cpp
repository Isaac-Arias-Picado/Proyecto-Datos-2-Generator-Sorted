#include "../Interfaces/Sorter.h"
#include "../Interfaces/PagedArray.h"
#include <algorithm>
#include <climits>
#include <cstring>

using namespace std;

void Sorter::quickSort(PagedArray& arr, int left, int right) {
    if (left >= right) return;

    int i = left, j = right;
    int pivot = arr[left + (right - left) / 2];

    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) {
            swap(arr[i], arr[j]);
            i++;
            j--;
        }
    }

    if (left < j) quickSort(arr, left, j);
    if (i < right) quickSort(arr, i, right);
}

void Sorter::shellSort(PagedArray& arr, int left, int right) {
    int n = right - left + 1;

    static const int ciuraBase[] = { 1, 4, 10, 23, 57, 132, 301, 701 };
    const int numBase = sizeof(ciuraBase) / sizeof(ciuraBase[0]);

    int gaps[64];
    int numGaps = 0;

    for (int k = 0; k < numBase; k++) {
        gaps[numGaps++] = ciuraBase[k];
    }
    while (true) {
        long long next = (long long)gaps[numGaps - 1] * 9 / 4; 
        if (next >= n || numGaps >= 64) break;
        gaps[numGaps++] = (int)next;
    }

    int startGap = -1;
    for (int k = numGaps - 1; k >= 0; k--) {
        if (gaps[k] < n) {
            startGap = k;
            break;
        }
    }

    if (startGap == -1) startGap = 0; 

    for (int k = startGap; k >= 0; k--) {
        int gap = gaps[k];
        for (int i = left + gap; i <= right; i++) {
            int temp = arr[i];
            int j;
            for (j = i; j >= left + gap && arr[j - gap] > temp; j -= gap) {
                arr[j] = arr[j - gap];
            }
            arr[j] = temp;
        }
    }
}

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
    int len1 = mid - left + 1;
    int len2 = right - mid;

    int* leftArr = new int[len1];
    int* rightArr = new int[len2];

    for (int i = 0; i < len1; i++) leftArr[i] = arr[left + i];
    for (int i = 0; i < len2; i++) rightArr[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2) {
        if (leftArr[i] <= rightArr[j]) arr[k++] = leftArr[i++];
        else                           arr[k++] = rightArr[j++];
    }
    while (i < len1) arr[k++] = leftArr[i++];
    while (j < len2) arr[k++] = rightArr[j++];

    delete[] leftArr;
    delete[] rightArr;
}

void Sorter::timSort(PagedArray& arr, int left, int right) {
    int n = right - left + 1;

    const int RUN = 32;

    for (int i = left; i <= right; i += RUN) {
        int runRight = min(i + RUN - 1, right);
        timInsertionSort(arr, i, runRight);
    }

    for (int size = RUN; size < n; size *= 2) {
        for (int i = left; i <= right; i += 2 * size) {
            int mid = min(i + size - 1, right);
            int runRight = min(i + 2 * size - 1, right);

            if (mid < runRight) {
                timMerge(arr, i, mid, runRight);
            }
        }
    }
}

void Sorter::mergeHelper(PagedArray& arr, int left, int mid, int right) {
    int len1 = mid - left + 1;
    int len2 = right - mid;

    int* leftArr = new int[len1];
    int* rightArr = new int[len2];

    for (int i = 0; i < len1; i++) leftArr[i] = arr[left + i];
    for (int i = 0; i < len2; i++) rightArr[i] = arr[mid + 1 + i];

    int i = 0, j = 0, k = left;
    while (i < len1 && j < len2) {
        if (leftArr[i] <= rightArr[j]) arr[k++] = leftArr[i++];
        else arr[k++] = rightArr[j++];
    }
    while (i < len1) arr[k++] = leftArr[i++];
    while (j < len2) arr[k++] = rightArr[j++];

    delete[] leftArr;
    delete[] rightArr;
}

void Sorter::mergeSort(PagedArray& arr, int left, int right) {
    if (left >= right) return;

    int mid = left + (right - left) / 2;
    mergeSort(arr, left, mid);
    mergeSort(arr, mid + 1, right);
    mergeHelper(arr, left, mid, right);
}

void Sorter::radixSort(PagedArray& arr, int left, int right) {
    int n = right - left + 1;
    if (n <= 1) return;

    const int BASE = 256;       
    const int PASSES = 4;       

    int* output = new int[n];

    for (int i = 0; i < n; i++) {
        output[i] = arr[left + i];
    }

    int* src = output;
    int* dst = new int[n];

    for (int pass = 0; pass < PASSES; pass++) {
        int count[BASE] = { 0 };
        int shift = pass * 8;

        for (int i = 0; i < n; i++) {
            unsigned int val = (unsigned int)src[i];
            int byte = (val >> shift) & 0xFF;
            count[byte]++;
        }

        if (pass == PASSES - 1) {          
            int prefixSum = 0;

            for (int b = 128; b < BASE; b++) {
                int c = count[b];
                count[b] = prefixSum;
                prefixSum += c;
            }
            for (int b = 0; b < 128; b++) {
                int c = count[b];
                count[b] = prefixSum;
                prefixSum += c;
            }
        }
        else {
            int prefixSum = 0;
            for (int b = 0; b < BASE; b++) {
                int c = count[b];
                count[b] = prefixSum;
                prefixSum += c;
            }
        }

        for (int i = 0; i < n; i++) {
            unsigned int val = (unsigned int)src[i];
            int byte = (val >> shift) & 0xFF;
            dst[count[byte]++] = src[i];
        }

        int* tmp = src;
        src = dst;
        dst = tmp;
    }

    for (int i = 0; i < n; i++) {
        arr[left + i] = src[i];
    }

    delete[] output;
    delete[] dst;
}