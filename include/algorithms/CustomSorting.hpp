#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <utility>
#include <functional>
#include <chrono>
#include <sstream>

/**
 * @brief Custom Sorting Algorithms Suite (CS2305 Course Outcome CO1, with Heapsort supporting CO5).
 * 
 * ALGORITHMS IMPLEMENTED:
 * 1. Custom Merge Sort (Stable, O(n log n) Best/Average/Worst, O(n) auxiliary space).
 * 2. Custom Quicksort (Unstable, Median-of-Three pivot, Lomuto partition,
 *    Tail-call recursion bounded to O(log n) stack space, O(n log n) average, O(n^2) worst-case).
 * 3. Custom Heapsort (Unstable, Strictly in-place O(1) auxiliary space,
 *    O(n log n) Best/Average/Worst).
 * 
 * COMPARISON & METRICS CONVENTION:
 * - Comparisons: Exactly 1 increment per invocation of comparator comp(a, b).
 * - Swaps: Exactly 1 increment per std::swap call.
 * - Moves: Standardized assignment/move count across algorithms:
 *     * Merge Sort: 1 move per assignment into temporary array, and 1 move back to main array.
 *     * Quicksort & Heapsort: 1 element swap corresponds to 3 element moves (temp = a, a = b, b = temp).
 * - Stability: Merge sort preserves the relative order of duplicate/equal keys.
 *              Quicksort and Heapsort perform non-adjacent swaps that violate original sequence order.
 * - Performance Benchmark Note: On tiny datasets (e.g., the 12-record seed dataset), microsecond
 *   execution time is dominated by cache warmth and timer jitter. Algorithmic efficiency
 *   comparisons are authoritatively evaluated using comparison counts and element moves.
 * - Tracing Note: When trace logging is enabled, traceEnabled is set to true. Reporting displays
 *   suppress or label trace duration to prevent conflating logging overhead with pure algorithmic speed.
 */

namespace ds_algorithms {

    struct SortMetrics {
        std::string algorithmName;
        size_t elementCount = 0;
        uint64_t comparisons = 0;
        uint64_t swaps = 0;
        uint64_t moves = 0;
        int64_t durationMicroseconds = 0;
        bool isStable = false;
        bool traceEnabled = false;
        std::string timeComplexityBest;
        std::string timeComplexityAverage;
        std::string timeComplexityWorst;
        std::string spaceComplexityAuxiliary;
    };

    // ========================================================================
    // 1. CUSTOM MERGE SORT (STABLE, O(n log n) GUARANTEE)
    // ========================================================================

    namespace internal {
        template <typename T, typename Comparator>
        void mergeWithMetrics(std::vector<T>& arr, std::vector<T>& temp, size_t left, size_t mid, size_t right,
                              Comparator comp, SortMetrics& metrics, bool enableTrace, std::vector<std::string>* traceLog) {
            size_t i = left;
            size_t j = mid + 1;
            size_t k = left;

            while (i <= mid && j <= right) {
                metrics.comparisons++;
                // Stability: If arr[j] is NOT strictly less than arr[i], pick arr[i] (left sub-array first)
                if (comp(arr[j], arr[i])) {
                    temp[k++] = std::move(arr[j++]);
                } else {
                    temp[k++] = std::move(arr[i++]);
                }
                metrics.moves++;
            }

            while (i <= mid) {
                temp[k++] = std::move(arr[i++]);
                metrics.moves++;
            }
            while (j <= right) {
                temp[k++] = std::move(arr[j++]);
                metrics.moves++;
            }

            for (size_t idx = left; idx <= right; ++idx) {
                arr[idx] = std::move(temp[idx]);
                metrics.moves++;
            }

            if (enableTrace && traceLog && arr.size() <= 16) {
                std::ostringstream oss;
                oss << "[MergeSort Trace] Merged sub-arrays [" << left << ".." << mid << "] and [" 
                    << (mid + 1) << ".." << right << "]";
                traceLog->push_back(oss.str());
            }
        }

        template <typename T, typename Comparator>
        void mergeSortRecursive(std::vector<T>& arr, std::vector<T>& temp, size_t left, size_t right,
                                Comparator comp, SortMetrics& metrics, bool enableTrace, std::vector<std::string>* traceLog) {
            if (left >= right) return;
            size_t mid = left + (right - left) / 2;
            mergeSortRecursive(arr, temp, left, mid, comp, metrics, enableTrace, traceLog);
            mergeSortRecursive(arr, temp, mid + 1, right, comp, metrics, enableTrace, traceLog);
            mergeWithMetrics(arr, temp, left, mid, right, comp, metrics, enableTrace, traceLog);
        }
    } // namespace internal

    /**
     * @brief Stable Merge Sort with detailed operation telemetry.
     */
    template <typename T, typename Comparator = std::less<T>>
    void customMergeSort(std::vector<T>& arr, Comparator comp, SortMetrics& metrics,
                        bool enableTrace = false, std::vector<std::string>* traceLog = nullptr) {
        metrics.algorithmName = "Merge Sort (Stable)";
        metrics.elementCount = arr.size();
        metrics.comparisons = 0;
        metrics.swaps = 0;
        metrics.moves = 0;
        metrics.isStable = true;
        metrics.timeComplexityBest = "O(n log n)";
        metrics.timeComplexityAverage = "O(n log n)";
        metrics.timeComplexityWorst = "O(n log n)";
        metrics.spaceComplexityAuxiliary = "O(n)";
        metrics.traceEnabled = enableTrace;

        if (arr.size() <= 1) {
            metrics.durationMicroseconds = 0;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<T> temp(arr.size());
        internal::mergeSortRecursive(arr, temp, 0, arr.size() - 1, comp, metrics, enableTrace, traceLog);

        auto end = std::chrono::high_resolution_clock::now();
        metrics.durationMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    /**
     * @brief Backward-compatible Merge Sort overload without metrics.
     */
    template <typename T, typename Comparator = std::less<T>>
    void customMergeSort(std::vector<T>& arr, Comparator comp = Comparator()) {
        SortMetrics dummy;
        customMergeSort(arr, comp, dummy, false, nullptr);
    }

    // ========================================================================
    // 2. CUSTOM QUICKSORT (MEDIAN-OF-THREE, TAIL-RECURSION OPTIMIZED)
    // ========================================================================

    namespace internal {
        template <typename T, typename Comparator>
        size_t partitionLomutoMedianOfThree(std::vector<T>& arr, size_t left, size_t right,
                                            Comparator comp, SortMetrics& metrics,
                                            bool enableTrace, std::vector<std::string>* traceLog) {
            size_t mid = left + (right - left) / 2;

            // Median-of-Three pivot selection: sort arr[left], arr[mid], arr[right]
            metrics.comparisons++;
            if (comp(arr[mid], arr[left])) {
                std::swap(arr[left], arr[mid]);
                metrics.swaps++;
                metrics.moves += 3;
            }
            metrics.comparisons++;
            if (comp(arr[right], arr[left])) {
                std::swap(arr[left], arr[right]);
                metrics.swaps++;
                metrics.moves += 3;
            }
            metrics.comparisons++;
            if (comp(arr[right], arr[mid])) {
                std::swap(arr[mid], arr[right]);
                metrics.swaps++;
                metrics.moves += 3;
            }

            // Pivot is now at arr[mid]. Move pivot to arr[right] for Lomuto partitioning.
            if (mid != right) {
                std::swap(arr[mid], arr[right]);
                metrics.swaps++;
                metrics.moves += 3;
            }

            if (enableTrace && traceLog && arr.size() <= 16) {
                std::ostringstream oss;
                oss << "[QuickSort Trace] Partition range [" << left << ".." << right << "] with pivot at index " << right;
                traceLog->push_back(oss.str());
            }

            size_t i = left;
            for (size_t j = left; j < right; ++j) {
                metrics.comparisons++;
                if (comp(arr[j], arr[right])) {
                    if (i != j) {
                        std::swap(arr[i], arr[j]);
                        metrics.swaps++;
                        metrics.moves += 3;
                    }
                    i++;
                }
            }

            if (i != right) {
                std::swap(arr[i], arr[right]);
                metrics.swaps++;
                metrics.moves += 3;
            }

            return i;
        }

        template <typename T, typename Comparator>
        void quickSortIterativeTailCall(std::vector<T>& arr, size_t left, size_t right,
                                        Comparator comp, SortMetrics& metrics,
                                        bool enableTrace, std::vector<std::string>* traceLog) {
            while (left < right) {
                size_t p = partitionLomutoMedianOfThree(arr, left, right, comp, metrics, enableTrace, traceLog);

                // Tail-recursion optimization: recurse into the smaller partition first,
                // and loop over the larger partition. This bounds auxiliary stack depth to O(log n).
                if (p - left < right - p) {
                    if (p > 0) {
                        quickSortIterativeTailCall(arr, left, p - 1, comp, metrics, enableTrace, traceLog);
                    }
                    left = p + 1;
                } else {
                    quickSortIterativeTailCall(arr, p + 1, right, comp, metrics, enableTrace, traceLog);
                    if (p == 0) break;
                    right = p - 1;
                }
            }
        }
    } // namespace internal

    /**
     * @brief Custom Quicksort with Median-of-Three pivot and O(log n) bounded recursion stack.
     */
    template <typename T, typename Comparator = std::less<T>>
    void customQuickSort(std::vector<T>& arr, Comparator comp, SortMetrics& metrics,
                         bool enableTrace = false, std::vector<std::string>* traceLog = nullptr) {
        metrics.algorithmName = "Quicksort (Unstable)";
        metrics.elementCount = arr.size();
        metrics.comparisons = 0;
        metrics.swaps = 0;
        metrics.moves = 0;
        metrics.isStable = false;
        metrics.timeComplexityBest = "O(n log n)";
        metrics.timeComplexityAverage = "O(n log n)";
        metrics.timeComplexityWorst = "O(n^2)";
        metrics.spaceComplexityAuxiliary = "O(log n)";
        metrics.traceEnabled = enableTrace;

        if (arr.size() <= 1) {
            metrics.durationMicroseconds = 0;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        internal::quickSortIterativeTailCall(arr, 0, arr.size() - 1, comp, metrics, enableTrace, traceLog);

        auto end = std::chrono::high_resolution_clock::now();
        metrics.durationMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    template <typename T, typename Comparator = std::less<T>>
    void customQuickSort(std::vector<T>& arr, Comparator comp = Comparator()) {
        SortMetrics dummy;
        customQuickSort(arr, comp, dummy, false, nullptr);
    }

    // ========================================================================
    // 3. CUSTOM HEAPSORT (IN-PLACE, STRICTLY O(1) AUXILIARY SPACE)
    // ========================================================================

    namespace internal {
        template <typename T, typename Comparator>
        void siftDownMaxHeap(std::vector<T>& arr, size_t root, size_t heapSize,
                             Comparator comp, SortMetrics& metrics) {
            while (true) {
                size_t largest = root;
                size_t left = 2 * root + 1;
                size_t right = 2 * root + 2;

                if (left < heapSize) {
                    metrics.comparisons++;
                    // In an ascending sort, max-heap requires that the parent dominate the children:
                    // If arr[largest] < arr[left], left is larger
                    if (comp(arr[largest], arr[left])) {
                        largest = left;
                    }
                }

                if (right < heapSize) {
                    metrics.comparisons++;
                    if (comp(arr[largest], arr[right])) {
                        largest = right;
                    }
                }

                if (largest != root) {
                    std::swap(arr[root], arr[largest]);
                    metrics.swaps++;
                    metrics.moves += 3;
                    root = largest;
                } else {
                    break;
                }
            }
        }
    } // namespace internal

    /**
     * @brief Standalone, in-place Heapsort achieving strict O(1) auxiliary space without copying.
     */
    template <typename T, typename Comparator = std::less<T>>
    void customHeapSort(std::vector<T>& arr, Comparator comp, SortMetrics& metrics,
                        bool enableTrace = false, std::vector<std::string>* traceLog = nullptr) {
        metrics.algorithmName = "Heapsort (Unstable, In-Place)";
        metrics.elementCount = arr.size();
        metrics.comparisons = 0;
        metrics.swaps = 0;
        metrics.moves = 0;
        metrics.isStable = false;
        metrics.timeComplexityBest = "O(n log n)";
        metrics.timeComplexityAverage = "O(n log n)";
        metrics.timeComplexityWorst = "O(n log n)";
        metrics.spaceComplexityAuxiliary = "O(1)";
        metrics.traceEnabled = enableTrace;

        if (arr.size() <= 1) {
            metrics.durationMicroseconds = 0;
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        size_t n = arr.size();

        // Phase 1: In-place Max-Heapify bottom-up in O(n) time
        for (size_t i = n / 2; i > 0; --i) {
            internal::siftDownMaxHeap(arr, i - 1, n, comp, metrics);
        }

        if (enableTrace && traceLog && n <= 16) {
            traceLog->push_back("[HeapSort Trace] Completed in-place max-heap construction");
        }

        // Phase 2: In-place element extraction and root sift-down in O(n log n) time
        for (size_t i = n - 1; i > 0; --i) {
            // Swap root (current maximum of unsorted prefix) with tail element arr[i]
            std::swap(arr[0], arr[i]);
            metrics.swaps++;
            metrics.moves += 3;

            // Sift down new root within the reduced heap prefix [0 .. i-1]
            internal::siftDownMaxHeap(arr, 0, i, comp, metrics);

            if (enableTrace && traceLog && n <= 16) {
                std::ostringstream oss;
                oss << "[HeapSort Trace] Placed max element at index " << i << "; sifted root in prefix [0.." << (i - 1) << "]";
                traceLog->push_back(oss.str());
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        metrics.durationMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    template <typename T, typename Comparator = std::less<T>>
    void customHeapSort(std::vector<T>& arr, Comparator comp = Comparator()) {
        SortMetrics dummy;
        customHeapSort(arr, comp, dummy, false, nullptr);
    }

} // namespace ds_algorithms
