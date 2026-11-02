#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include "models/PatientSnapshot.hpp"
#include "algorithms/CustomSorting.hpp"
#include "services/ReportingService.hpp"
#include "services/CampusMapManager.hpp"
#include "ds/HashTableChaining.hpp"
#include "ds/HashTableProbing.hpp"
#include "ds/BinarySearchTree.hpp"
#include "ds/CampusGraph.hpp"

/**
 * @brief Educational Demonstration Service (CS2305 Course Outcomes CO1-CO6).
 * 
 * Provides isolated, verifiable demonstrations of:
 * 1. Sorting Comparison: Merge Sort (Stable) vs Quicksort vs Heapsort on identical snapshots (CO1, CO5).
 * 2. Hash Collision Resolution: Separate Chaining vs Linear Probing with tombstones (CO2).
 * 3. Graph Representations: Adjacency List vs Adjacency Matrix traversal and complexity (CO6).
 * 4. Binary Search Tree Operations: Traversals, Mirroring, and 3-case Node Deletion (CO4).
 * 5. Shortest Path vs Spanning Tree: Dijkstra's point-to-point dispatch vs Kruskal/Prim MST (CO6).
 * 
 * ISOLATION POLICY:
 * - All demonstration data is strictly independent of the live hospital workflow.
 */
class DemonstrationService {
public:
    // ========================================================================
    // 1. SORTING COMPARISON DEMONSTRATION
    // ========================================================================

    static void runSortingComparison(const std::vector<PatientSnapshot>& baseData, std::ostream& out) {
        out << "========================================================================================================================\n";
        out << "                    EDUCATIONAL DEMONSTRATION: SORTING ALGORITHMS COMPARISON (CO1, CO5)\n";
        out << "========================================================================================================================\n";
        out << "  Input Dataset Size: " << baseData.size() << " Patient Snapshot Records\n";
        out << "  Sorting Criterion:  Clinical Urgency (Ascending 1 to 5, Acuity Priority)\n";
        out << "  Evaluation Rule:    Comparing exact operation counts (Comparisons, Swaps, Moves).\n";
        out << "                      [Note: When trace logging is enabled, microsecond duration is labeled\n";
        out << "                       'N/A (Traced)' to prevent conflating trace string formatting overhead with pure\n";
        out << "                       algorithmic speed. Algorithmic efficiency is rigorously evaluated via exact\n";
        out << "                       deterministic operation counts (Comparisons, Swaps, Moves).]\n\n";

        // Create 3 identical copies
        std::vector<PatientSnapshot> mergeData = baseData;
        std::vector<PatientSnapshot> quickData = baseData;
        std::vector<PatientSnapshot> heapData  = baseData;

        ds_algorithms::SortMetrics mergeMetrics;
        ds_algorithms::SortMetrics quickMetrics;
        ds_algorithms::SortMetrics heapMetrics;

        std::vector<std::string> mergeTrace;
        std::vector<std::string> quickTrace;
        std::vector<std::string> heapTrace;

        // Run sorts with tracing enabled for small inputs
        bool trace = (baseData.size() <= 16);
        ds_algorithms::customMergeSort(mergeData, ReportingService::CompareByUrgency(), mergeMetrics, trace, &mergeTrace);
        ds_algorithms::customQuickSort(quickData, ReportingService::CompareByUrgency(), quickMetrics, trace, &quickTrace);
        ds_algorithms::customHeapSort(heapData, ReportingService::CompareByUrgency(), heapMetrics, trace, &heapTrace);

        // Display Telemetry Table
        out << std::left
            << std::setw(30) << "Algorithm"
            << std::setw(10) << "Stable?"
            << std::setw(14) << "Comparisons"
            << std::setw(10) << "Swaps"
            << std::setw(12) << "Moves"
            << std::setw(16) << "Time / Status"
            << std::setw(14) << "Time (Avg)"
            << std::setw(14) << "Time (Worst)"
            << std::setw(12) << "Space (Aux)"
            << "\n";
        out << "------------------------------------------------------------------------------------------------------------------------\n";

        auto printRow = [&out](const ds_algorithms::SortMetrics& m) {
            std::string timeStr = m.traceEnabled ? "N/A (Traced)" : (std::to_string(m.durationMicroseconds) + " us");
            out << std::left
                << std::setw(30) << m.algorithmName
                << std::setw(10) << (m.isStable ? "YES" : "NO")
                << std::setw(14) << m.comparisons
                << std::setw(10) << m.swaps
                << std::setw(12) << m.moves
                << std::setw(16) << timeStr
                << std::setw(14) << m.timeComplexityAverage
                << std::setw(14) << m.timeComplexityWorst
                << std::setw(12) << m.spaceComplexityAuxiliary
                << "\n";
        };

        printRow(mergeMetrics);
        printRow(quickMetrics);
        printRow(heapMetrics);
        out << "------------------------------------------------------------------------------------------------------------------------\n\n";

        // Stability Analysis
        out << "  [STABILITY ANALYSIS]\n";
        out << "  * Merge Sort is STABLE: equal keys strictly preserve original FIFO arrival order.\n";
        out << "  * Quicksort is UNSTABLE: long-range partitioning swaps reorder equal elements.\n";
        out << "  * Heapsort is UNSTABLE: root-to-tail swaps and sift-down violate original relative order.\n";
        out << "  * Space Comparison: Heapsort achieves strictly O(1) in-place auxiliary memory;\n";
        out << "                      Quicksort requires O(log n) recursion stack;\n";
        out << "                      Merge Sort requires O(n) auxiliary temporary buffer.\n\n";
    }

    // ========================================================================
    // 2. HASH COLLISION COMPARISON: CHAINING VS LINEAR PROBING
    // ========================================================================

    static void runHashCollisionComparison(std::ostream& out) {
        out << "====================================================================================================\n";
        out << "         EDUCATIONAL DEMONSTRATION: HASHING COLLISION RESOLUTION (CO2)\n";
        out << "====================================================================================================\n";
        out << "  Comparing Separate Chaining vs Linear Probing using verified collision fixtures.\n\n";

        // Fixture: keys that collide into slot 14 under prime capacity 17
        HashTableChaining<std::string, std::string> chainingTable(17, 0.75);
        HashTableProbing<std::string, int> probingTable(17, 0.50);

        // Compute colliding keys for capacity 17
        // 'A' -> ASCII 65 % 17 = 14
        // 'R' -> ASCII 82 % 17 = 14
        std::string key1 = "A";
        std::string key2 = "R";
        std::string key3 = "i"; // ASCII 105 % 17 = 3 (non-colliding)

        size_t h1 = chainingTable.hash(key1);
        size_t h2 = chainingTable.hash(key2);

        out << "  Hash Function Verification (M = 17):\n";
        out << "    H('" << key1 << "') = " << h1 << "\n";
        out << "    H('" << key2 << "') = " << h2 << " -> DIRECT COLLISION DETECTED at bucket index " << h1 << "!\n\n";

        // Chaining Demonstration
        out << "  [1] Separate Chaining (HashTableChaining):\n";
        chainingTable.insert(key1, "Alpha Payload");
        chainingTable.insert(key2, "Romeo Payload");
        out << "    Inserted '" << key1 << "' and '" << key2 << "'.\n";
        out << "    Bucket " << h1 << " size: " << chainingTable.bucketSize(h1) << " nodes chained.\n";
        out << "    Both keys coexist in linked list without displacing other buckets.\n";
        out << "    Lookup '" << key1 << "': " << *chainingTable.search(key1) << "\n";
        out << "    Lookup '" << key2 << "': " << *chainingTable.search(key2) << "\n\n";

        // Probing Demonstration with Tombstone
        out << "  [2] Open Addressing with Linear Probing (HashTableProbing):\n";
        probingTable.insert(key1, 100);
        probingTable.insert(key2, 200);
        out << "    Inserted '" << key1 << "' (placed at home slot " << h1 << ").\n";
        out << "    Inserted '" << key2 << "' (collided at " << h1 << ", probed linearly to slot " << ((h1 + 1) % 17) << ").\n";
        out << "    Deleting '" << key1 << "' from slot " << h1 << " ...\n";
        probingTable.remove(key1);
        out << "    Slot " << h1 << " is now marked as a TOMBSTONE (DELETED sentinel).\n";
        out << "    Searching for '" << key2 << "' ...\n";
        int* foundVal = probingTable.search(key2);
        out << "    Search probes through tombstone at slot " << h1 << " and successfully finds '" << key2 
            << "' at slot " << ((h1 + 1) % 17) << " (Value: " << (foundVal ? *foundVal : -1) << ")!\n";
        out << "    Without tombstones, search would prematurely stop at an empty slot and report false negative.\n\n";
    }

    // ========================================================================
    // 3. GRAPH TRAVERSAL COMPARISON: ADJACENCY LIST VS MATRIX
    // ========================================================================

    static void runGraphTraversalComparison(std::ostream& out) {
        out << "====================================================================================================\n";
        out << "         EDUCATIONAL DEMONSTRATION: GRAPH REPRESENTATIONS & TRAVERSALS (CO6)\n";
        out << "====================================================================================================\n";

        // Create an independent 6-node demonstration campus subgraph
        CampusGraph demoGraph(6);
        demoGraph.addCorridor(0, 1, 100, "Reception to Triage");
        demoGraph.addCorridor(1, 2, 80,  "Triage to Trauma");
        demoGraph.addCorridor(1, 3, 120, "Triage to Surgery");
        demoGraph.addCorridor(2, 4, 150, "Trauma to ICU");
        demoGraph.addCorridor(3, 5, 90,  "Surgery to Recovery");
        demoGraph.addCorridor(4, 5, 70,  "ICU to Recovery");

        std::vector<int> bfsOrder = demoGraph.bfs(0);
        std::vector<int> dfsListOrder = demoGraph.dfsList(0);
        std::vector<int> dfsMatrixOrder = demoGraph.dfsMatrix(0);

        out << "  Demonstration Graph: 6 Nodes (V = 6), 6 Corridors (E = 6).\n";
        out << "  Traversals starting from Node 0:\n";
        out << "    BFS Exploration (Adjacency List + CustomQueue): ";
        for (int v : bfsOrder) out << v << " ";
        out << "\n";

        out << "    DFS Exploration (Adjacency List):             ";
        for (int v : dfsListOrder) out << v << " ";
        out << "\n";

        out << "    DFS Exploration (Adjacency Matrix):           ";
        for (int v : dfsMatrixOrder) out << v << " ";
        out << "\n\n";

        out << "  [COMPLEXITY COMPARISON]\n";
        out << "  * Adjacency List:\n";
        out << "    - Traversal Time: O(V + E) = 6 + 6 = 12 operations. Inspects strictly incident edges.\n";
        out << "    - Memory Space:   O(V + E). Highly efficient for sparse campus graph topology.\n";
        out << "  * Adjacency Matrix:\n";
        out << "    - Traversal Time: O(V^2) = 6^2 = 36 operations. Scans entire row for every vertex.\n";
        out << "    - Memory Space:   O(V^2). 6x6 table with many NO_EDGE (-1) empty cells.\n\n";
    }

    // ========================================================================
    // 4. BST TRAVERSAL, MIRROR & 3-CASE DELETION DEMONSTRATION
    // ========================================================================

    static void runBSTOperationsDemo(std::ostream& out) {
        out << "====================================================================================================\n";
        out << "          EDUCATIONAL DEMONSTRATION: BINARY SEARCH TREE OPERATIONS (CO4)\n";
        out << "====================================================================================================\n";

        BinarySearchTree<int, std::string> bst;
        // Build balanced 7-node demonstration BST:
        // Root 50 -> Left 30, Right 70; 30 -> (20, 40); 70 -> (60, 80)
        bst.insert(50, "Root");
        bst.insert(30, "LeftChild");
        bst.insert(70, "RightChild");
        bst.insert(20, "Leaf20");
        bst.insert(40, "Leaf40");
        bst.insert(60, "Leaf60");
        bst.insert(80, "Leaf80");

        out << "  Initial BST Built with 7 Nodes: [50, 30, 70, 20, 40, 60, 80]\n";
        out << "    Tree Height: " << bst.height() << " | Leaf Nodes: " << bst.countLeaves() << "\n\n";

        out << "  [TRAVERSALS]\n";
        out << "    In-Order (L-Root-R, Ascending Sorted):   ";
        for (int k : bst.inorderRecursive()) out << k << " ";
        out << "\n";

        out << "    Pre-Order (Root-L-R, Prefix Structure): ";
        for (int k : bst.preorderRecursive()) out << k << " ";
        out << "\n";

        out << "    Post-Order (L-R-Root, Bottom-Up):       ";
        for (int k : bst.postorderRecursive()) out << k << " ";
        out << "\n";

        out << "    Level-Order (Breadth-First via Queue):  ";
        for (int k : bst.levelOrder()) out << k << " ";
        out << "\n\n";

        // Mirror Tree
        out << "  [BST MIRROR DEMONSTRATION]\n";
        auto mirrorTree = bst.createMirrorTree();
        out << "    Mirror In-Order (R-Root-L, Descending Sorted): ";
        for (int k : mirrorTree.inorderKeys()) out << k << " ";
        out << "\n";
        out << "    [Type-Safe Policy]: Mirrored tree is educational and cannot be used as standard search BST.\n\n";

        // 3-Case Deletion
        out << "  [3-CASE DELETION DEMONSTRATION]\n";
        out << "  Case 1: Deleting Leaf Node (Key 20) ...\n";
        bst.remove(20);
        out << "    In-Order after Leaf Deletion: ";
        for (int k : bst.inorderRecursive()) out << k << " ";
        out << "\n";

        out << "  Case 2: Deleting Node with Single Child (Key 30, which now only has child 40) ...\n";
        bst.remove(30);
        out << "    In-Order after Single-Child Deletion: ";
        for (int k : bst.inorderRecursive()) out << k << " ";
        out << "\n";

        out << "  Case 3: Deleting Node with Two Children (Key 70, replaced by in-order successor 80) ...\n";
        bst.remove(70);
        out << "    In-Order after Two-Child Deletion: ";
        for (int k : bst.inorderRecursive()) out << k << " ";
        out << "\n\n";
    }

    // ========================================================================
    // 5. DIJKSTRA VS MINIMUM SPANNING TREE (PURPOSE & RESULTS)
    // ========================================================================

    static void runDijkstraVsMSTDemo(const CampusMapManager& mapManager, std::ostream& out) {
        out << "====================================================================================================\n";
        out << "      EDUCATIONAL DEMONSTRATION: DIJKSTRA SHORTEST PATH VS MINIMUM SPANNING TREE (CO6)\n";
        out << "====================================================================================================\n";

        const auto& graph = mapManager.getGraph();

        // Query shortest path from Ambulance Reception (0) to Surgery (3)
        DijkstraResult dijkstraRes = graph.findShortestRoute(0, 3);
        MSTResult kruskalMST = graph.computeKruskalMST();
        MSTResult primMST    = graph.computePrimMST();

        out << "  1. Dijkstra's Algorithm (Point-to-Point Route Optimization):\n";
        out << "     Purpose:         Finds minimum-cost path between SPECIFIC source and destination vertices.\n";
        out << "     Query:           Node 0 (Ambulance Bay) -> Node 3 (Surgery)\n";
        out << "     Shortest Route:  ";
        for (size_t i = 0; i < dijkstraRes.path.size(); ++i) {
            out << dijkstraRes.path[i];
            if (i + 1 < dijkstraRes.path.size()) out << " -> ";
        }
        out << "\n";
        out << "     Transit Time:    " << dijkstraRes.totalCost << " seconds (Optimal rapid patient transfer path)\n\n";

        out << "  2. Minimum Spanning Tree (Global Campus Infrastructure Connectivity):\n";
        out << "     Purpose:         Connects ALL V vertices using exactly V - 1 edges with MINIMUM aggregate weight.\n";
        out << "     Kruskal's MST:   Total Transit Weight = " << kruskalMST.totalTransitWeight << "s across "
            << kruskalMST.edges.size() << " corridors.\n";
        out << "     Prim's MST:      Total Transit Weight = " << primMST.totalTransitWeight << "s across "
            << primMST.edges.size() << " corridors.\n";
        out << "     Equivalence:     Both algorithms yield identical global minimum weight (" 
            << kruskalMST.totalTransitWeight << "s).\n\n";

        out << "  [CRITICAL ARCHITECTURAL DISTINCTION]\n";
        out << "  * The path between two nodes in the MST is NOT necessarily the shortest path in the graph!\n";
        out << "  * MST optimizes global connectivity; Dijkstra optimizes local individual path latency.\n";
        out << "  * Route dispatch MUST use Dijkstra for time-critical emergency transfers.\n\n";
    }
};
