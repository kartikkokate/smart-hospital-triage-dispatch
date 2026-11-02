# Course Outcome (CO) Mapping & Syllabus Alignment Matrix
**Course:** CS2305 — Data Structures-I (B.Tech Computer Engineering, Second Year, Semester 1)  
**Academic Year:** 2026–27  

---

## 1. Syllabus Context & Faculty Confirmation Note

The supplied course curriculum documentation contains two divergent course outcome listings across different administrative sections:
- **Variant A (Unit-Themed Structure):** Maps CO1 through CO6 directly to the six canonical syllabus units:
  * **CO1:** Arrays, Recursion, Searching & Sorting Algorithms.
  * **CO2:** Linear Linked Structures & Hashing Techniques.
  * **CO3:** Stacks & Queues (Linear Abstract Data Types).
  * **CO4:** Non-Linear Hierarchical Structures (Binary Trees & Binary Search Trees).
  * **CO5:** Priority Queues, Binary Heaps & Heap Applications (including Heapsort).
  * **CO6:** Graph Topologies, Network Traversals, Minimum Spanning Trees & Shortest Paths.
- **Variant B (Skill/Taxonomy-Themed Structure):** Focuses on Bloom's taxonomy categories (Knowledge, Application, Analysis).

**Authoritative Policy Adopted:** This codebase implements and demonstrates the comprehensive six-unit structure (Variant A, CO1–CO6), ensuring complete coverage of all fundamental and advanced data structures and algorithms. This mapping is flagged for oral presentation and confirmation with faculty evaluators during viva defense.

---

## 2. Complete Course Outcome Traceability Matrix

| Course Outcome | Academic Topic Description | Primary Source Files / Headers | Callable Application Features | Automated Test & Verification Evidence |
|---|---|---|---|---|
| **CO1** | Arrays, Recursion, and Sorting Algorithms | [`include/algorithms/CustomSorting.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/algorithms/CustomSorting.hpp)<br>[`include/services/ReportingService.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/ReportingService.hpp) | - Custom Recursive Merge Sort (`customMergeSort`)<br>- Custom Quicksort (`customQuickSort`) with Median-of-3 and Lomuto partitioning<br>- Interactive Sorting Menu (`ConsoleApp` Menu 6)<br>- Educational Sorting Telemetry Demo (`DemonstrationService::runSortingComparison`) | `testCustomSortingAlgorithms`<br>`testSortingStability`<br>`testPatientSnapshotAndReportingService`<br>Console App Subtest 8 |
| **CO2** | Linked Lists and Hashing Techniques | [`include/ds/SinglyLinkedList.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/SinglyLinkedList.hpp)<br>[`include/ds/DoublyLinkedList.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/DoublyLinkedList.hpp)<br>[`include/ds/HashTableChaining.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/HashTableChaining.hpp)<br>[`include/ds/HashTableProbing.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/HashTableProbing.hpp)<br>[`include/services/HospitalRegistry.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/HospitalRegistry.hpp) | - Singly Linked List dynamic node management<br>- Doubly Linked List bidirectional audit logging (`getAuditEvents`)<br>- Expected $O(1)$ patient lookup via `HashTableChaining` with dynamic prime rehashing<br>- Open addressing with linear probing and tombstones (`HashTableProbing`)<br>- Educational Hash Collision Demo (`DemonstrationService::runHashCollisionComparison`) | `testSinglyLinkedList`<br>`testDoublyLinkedList`<br>`testHashTableChaining`<br>`testHashTableProbing`<br>`testExceptionSafeCopySemantics`<br>`testHospitalRegistryAndRegistration` |
| **CO3** | Stacks and Queues | [`include/ds/CustomStack.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CustomStack.hpp)<br>[`include/ds/CustomQueue.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CustomQueue.hpp)<br>[`include/services/AppointmentManager.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/AppointmentManager.hpp) | - LIFO Dijkstra path reconstruction (`CustomStack`)<br>- Iterative BST traversal backtracking<br>- Outpatient clinic FIFO queue (`AppointmentManager`)<br>- Calling, viewing, and cancelling appointments<br>- Queue node transfer to emergency triage | `testCustomStack`<br>`testCustomQueue`<br>`testAppointmentFIFOAndCancellationAndConversion`<br>`testPreserveAppointmentOrderOnFailedConversion`<br>Console App Subtest 4 |
| **CO4** | Binary Trees and Binary Search Trees | [`include/ds/BinarySearchTree.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/BinarySearchTree.hpp)<br>[`include/services/HospitalRegistry.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/HospitalRegistry.hpp) | - Patient demographic search via BST (`idBST_`)<br>- 3-Case BST deletion (leaf, single-child, two-children with in-order successor)<br>- Recursive and iterative (`CustomStack`) in-order traversals<br>- Level-order traversal (`CustomQueue`)<br>- Type-safe mirror tree cloning (`MirrorBinaryTree`)<br>- Educational BST Demo (`DemonstrationService::runBSTOperationsDemo`) | `testBinarySearchTree`<br>`testHospitalRegistryAndRegistration`<br>`testEducationalDemonstrations` |
| **CO5** | Priority Queues, Binary Heaps, and Heapsort | [`include/ds/BinaryMinHeap.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/BinaryMinHeap.hpp)<br>[`include/services/TriageManager.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/TriageManager.hpp)<br>[`include/algorithms/CustomSorting.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/algorithms/CustomSorting.hpp) | - Emergency clinical triage min-heap (`TriageMinHeap`)<br>- Priority tie-breaking (Urgency 1–5, then FIFO arrival order)<br>- Dynamic acuity updates (`updateUrgency`) with sequence preservation via `heapPosByHandle`<br>- Standalone in-place Heapsort (`customHeapSort`) achieving strictly $O(1)$ auxiliary space | `testBinaryMinHeapAndTriage`<br>`testEqualUrgencyFIFOAndUrgencyUpdates`<br>`testPendingDispatchOrderingPolicy`<br>`testCustomSortingAlgorithms`<br>Console App Subtest 5 |
| **CO6** | Graphs, Traversals, Shortest Paths, and Minimum Spanning Trees | [`include/ds/CampusGraph.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CampusGraph.hpp)<br>[`include/ds/DisjointSetUnion.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/DisjointSetUnion.hpp)<br>[`include/services/CampusMapManager.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/CampusMapManager.hpp)<br>[`include/services/DispatchManager.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/services/DispatchManager.hpp) | - Dual synchronized Adjacency List & Adjacency Matrix<br>- Breadth-First Search (BFS) reachable wing exploration<br>- Depth-First Search (DFS) cycle detection<br>- Dijkstra shortest path with corridor blockages and rerouting<br>- Kruskal's algorithm with custom Disjoint Set Union (DSU)<br>- Prim's algorithm with priority queue<br>- Spanning Forest (MSF) reporting on disconnected graphs<br>- Educational Graph Demo (`DemonstrationService::runGraphTraversalComparison`, `runDijkstraVsMSTDemo`) | `testCampusGraphAndRouting`<br>`testSimulationLegsCorridorClosureAndRerouting`<br>`testDeterministicResourceSelectionAndTieBreaking`<br>`testEducationalDemonstrations`<br>Console App Subtest 6 & 7 |

---

## 3. Detailed Evidence by Course Outcome

### CO1: Arrays, Recursion & Sorting
- **Code:** Implemented in [`include/algorithms/CustomSorting.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/algorithms/CustomSorting.hpp).
- **Callable Demonstration:** Run `--demo` or choose Menu 6 (Reports) $\to$ Option 1 to sort by any of 5 fields (Age, Arrival Time, Urgency, Billing, Waiting Duration) using Merge Sort or Quicksort.
- **Verification Evidence:** `testCustomSortingAlgorithms` tests empty arrays, 1-element arrays, duplicates, and reverse inputs; `testSortingStability` proves Merge Sort is stable and Quicksort is unstable.

### CO2: Linked Lists & Hashing
- **Code:** Implemented in [`include/ds/SinglyLinkedList.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/SinglyLinkedList.hpp), [`include/ds/DoublyLinkedList.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/DoublyLinkedList.hpp), [`include/ds/HashTableChaining.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/HashTableChaining.hpp), and [`include/ds/HashTableProbing.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/HashTableProbing.hpp).
- **Callable Demonstration:** Run Menu 7 (Educational Demos) $\to$ Option 3 to execute direct hash collision testing with colliding keys `'A'` and `'R'` hashing to bucket 14 mod 17, demonstrating bucket chaining and linear probing tombstones.
- **Verification Evidence:** `testHashTableChaining` verifies dynamic prime rehashing when $\alpha > 0.75$; `testHashTableProbing` proves search continuity past tombstones; `testExceptionSafeCopySemantics` proves zero leaks on throwing copies.

### CO3: Stacks & Queues
- **Code:** Implemented in [`include/ds/CustomStack.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CustomStack.hpp) and [`include/ds/CustomQueue.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CustomQueue.hpp).
- **Callable Demonstration:** Menu 3 (Outpatient Clinic) books patients into the queue, calls them in FIFO order, or cancels appointments. Menu 5 (Routing) uses `CustomStack` to reverse Dijkstra predecessor links.
- **Verification Evidence:** `testCustomStack` and `testCustomQueue` prove safe underflow protection, deep copy order preservation, and clean pointer resets.

### CO4: Trees & Binary Search Trees
- **Code:** Implemented in [`include/ds/BinarySearchTree.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/BinarySearchTree.hpp).
- **Callable Demonstration:** Menu 7 (Educational Demos) $\to$ Option 5 builds a balanced 7-node BST, executes in-order, pre-order, post-order, and level-order traversals, clones an independent type-safe mirror tree, and performs 3-case node deletion.
- **Verification Evidence:** `testBinarySearchTree` verifies recursive in-order matches iterative in-order using `CustomStack`, verifies leaf, 1-child, and 2-children deletions, and confirms non-owning pointer isolation.

### CO5: Priority Heaps & Heapsort
- **Code:** Implemented in [`include/ds/BinaryMinHeap.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/BinaryMinHeap.hpp) and [`include/algorithms/CustomSorting.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/algorithms/CustomSorting.hpp).
- **Callable Demonstration:** Menu 2 (Triage) displays the top-priority emergency patient in $O(1)$ time; allows changing urgency with $O(\log n)$ sift-up/down. Menu 6 Option 3 executes in-place Heapsort.
- **Verification Evidence:** `testBinaryMinHeapAndTriage` confirms arrival order preservation on tie-breaks; `testEqualUrgencyFIFOAndUrgencyUpdates` verifies sequence `P104 -> P107 -> P108 -> P112` after P108 deteriorates to Urgency 1.

### CO6: Graphs, Shortest Paths & Minimum Spanning Trees
- **Code:** Implemented in [`include/ds/CampusGraph.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/CampusGraph.hpp) and [`include/ds/DisjointSetUnion.hpp`](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/include/ds/DisjointSetUnion.hpp).
- **Callable Demonstration:** Menu 5 (Campus Map) queries shortest paths (e.g. Node 0 to 3 takes 270s), blocks corridors (rerouting 0 to 3 via 330s alternate path), computes Kruskal and Prim MSTs (both verifying exact 1280s total weight).
- **Verification Evidence:** `testCampusGraphAndRouting` asserts exact Dijkstra costs, detects cycles in non-root components, validates Kruskal and Prim equivalence on seed data, and verifies disconnected spanning forest handling.
