#pragma once
#include "PagedArray.h"

class Sorter {
public:
    void quickSort(PagedArray& arr, int low, int high);
    void shellSort(PagedArray& arr, int left, int right);
    void mergeSort(PagedArray& arr, int size);
    void radixSort(PagedArray& array, int size);
	void timSort(PagedArray& arr, int left, int right);
private:
    void mergeHelper(PagedArray& arr, int left, int mid, int right,PagedArray& aux);
    void mergeSortRec(PagedArray& arr, int left, int right, PagedArray& aux);
    void countingSort(PagedArray& array, PagedArray& output, int size, int shift);
	void timInsertionSort(PagedArray& arr, int left, int right);
	void timMerge(PagedArray& arr, int left, int mid, int right);
};