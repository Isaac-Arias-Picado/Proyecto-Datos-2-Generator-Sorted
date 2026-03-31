#pragma once
#include <atomic>
#include "PagedArray.h"

class Sorter {
public:
    void quickSort(PagedArray& arr, int left, int right);
    void shellSort(PagedArray& arr, int left, int right);
    void timSort(PagedArray& arr, int left, int right);
    void mergeSort(PagedArray& arr, int left, int right);
    void radixSort(PagedArray& arr, int left, int right);

private:
    void mergeHelper(PagedArray& arr, int left, int mid, int right);
    void timInsertionSort(PagedArray& arr, int left, int right);
    void timMerge(PagedArray& arr, int left, int mid, int right);
};