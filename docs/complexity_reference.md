# Comprehensive Algorithm & Data Structure Complexity Reference
**Course:** CS2305 — Data Structures-I (AY 2026–27)  

This document provides exact, academically rigorous time and space complexity bounds for every custom data structure and core algorithm implemented in the project.

---

## 1. Custom Linear & Hash-Based Structures (CO2, CO3)

| Data Structure / Operation | Best Case | Average Case | Worst Case | Auxiliary Space | Key Assumptions & Implementation Details |
|---|---|---|---|---|---|
| **SinglyLinkedList<T>** | | | | | |
| - `push_front` / `push_back` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Maintains explicit head and tail pointers. |
| - `pop_front` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Deallocates head node, returns false if empty. |
| - `insert_after` / `remove` | $O(1)$ | $O(n)$ | $O(n)$ | $O(1)$ | Iterative traversal avoids recursion stack. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | 1 forward pointer per node (`Node* next`). |
| **DoublyLinkedList<T>** | | | | | |
| - `push_front` / `push_back` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Bidirectional node links (`prev`, `next`). |
| - `pop_front` / `pop_back` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Popping final element resets head and tail to `nullptr`. |
| - Forward / Reverse Traversal | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(1)$ | Constant auxiliary space per traversal. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | 2 pointers per node (`prev`, `next`). |
| **CustomStack<T>** | | | | | |
| - `push` / `pop` / `top` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Singly-linked LIFO stack with underflow guard. |
| - `clear` | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(1)$ | Iterative loop deallocation. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | 1 pointer per stack frame node. |
| **CustomQueue<T>** | | | | | |
| - `enqueue` / `dequeue` / `front` | $O(1)$ | $O(1)$ | $O(1)$ | $O(1)$ | Singly-linked FIFO queue with head/tail pointers. |
| - `remove` / `contains` | $O(1)$ | $O(n)$ | $O(n)$ | $O(1)$ | Traverses linked chain. |
| - Node Reinsertion (`reinsertNodeAt`) | $O(1)$ | $O(n)$ | $O(n)$ | $O(1)$ | Zero allocation; relinks existing heap node pointers. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | 1 pointer per queue element node. |
| **HashTableChaining<K, V>** | | | | | |
| - `search` (Lookup) | $\Theta(1)$ | **Expected $O(1)$** | $O(n)$ | $O(1)$ | Polynomial rolling hash $H(s) = (\sum s[i] \cdot 31^i) \pmod M$. Worst case occurs if all $n$ keys hash to 1 bucket. |
| - `insert` (Amortized) | $\Theta(1)$ | $\Theta(1)$ amortized | $O(n + M)$ | $O(1)$ | Triggers rehash to prime $M \ge 2M+1$ when load factor $\alpha > 0.75$. |
| - `remove` | $\Theta(1)$ | Expected $O(1)$ | $O(n)$ | $O(1)$ | Unlinks node from bucket list. |
| - Total Storage Space | — | — | $\Theta(n + M)$ | $\Theta(n + M)$ | Array of $M$ bucket pointers + $n$ node structures. |
| **HashTableProbing<K, V>** | | | | | |
| - `search` (Lookup) | $\Theta(1)$ | **Expected $O(1)$** | $O(M)$ | $O(1)$ | Linear probing bounded by at most $M$ iterations. |
| - `insert` (Amortized) | $\Theta(1)$ | $\Theta(1)$ amortized | $O(M)$ | $O(1)$ | Reuses `DELETED` tombstone slots. |
| - `remove` (Tombstone) | $\Theta(1)$ | Expected $O(1)$ | $O(M)$ | $O(1)$ | Marks slot as `DELETED` tombstone sentinel. |
| - `rehash` | $\Theta(n + M)$ | $\Theta(n + M)$ | $O(n^2)$ | $\Theta(M_{new})$ | Worst case occurs under pathological primary clustering. |
| - Total Storage Space | — | — | $\Theta(M)$ | $\Theta(M)$ | Contiguous flat vector of $M$ `Slot` structs. |

---

## 2. Custom Hierarchical & Priority Structures (CO4, CO5)

| Data Structure / Operation | Best Case | Average Case | Worst Case | Auxiliary Space | Key Assumptions & Implementation Details |
|---|---|---|---|---|---|
| **BinarySearchTree<K, V>** | | | | | |
| - `search` / `insert` / `remove` | $O(1)$ | $\Theta(\log n)$ | **$O(h)$, worst-case $O(n)$** | $O(h)$ | $h$ is tree height. Balanced input yields $h = \Theta(\log n)$. Monotonically sorted insertion yields degenerate linear chain $h = n$. |
| - Traversals (inorder, preorder, postorder) | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(h)$ | Visits all $n$ nodes. Iterative in-order uses `CustomStack`. |
| - Level-Order Traversal | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(w)$ | $w$ is maximum tree width; uses `CustomQueue`. |
| - `createMirrorTree` | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(h)$ | Allocates independent `MirrorBinaryTree` with inverted children. |
| - 3-Case Node Deletion | $O(1)$ | $O(h)$ | $O(n)$ | $O(1)$ | In-order successor substitution for 2-children case. Non-owning policy preserves domain entities. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | 2 child pointers (`left`, `right`) per BST node. |
| **BinaryMinHeap<T, Comp>** | | | | | |
| - `peek` (Root priority) | $\Theta(1)$ | $\Theta(1)$ | $\Theta(1)$ | $O(1)$ | Read-only const pointer to array index 0. |
| - `insert` | $\Theta(1)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ | Appends to tail, sifts up to restore min-heap invariant. |
| - `extractMin` | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ | Swaps root with tail, pops tail, sifts down from root. |
| - `buildHeap` (Bottom-Up) | $\Theta(n)$ | $\Theta(n)$ | $\Theta(n)$ | $O(1)$ | Floydean bottom-up construction $\sum_{h=0}^{\log n} \frac{n}{2^{h+1}} O(h) = \Theta(n)$. |
| - Total Storage Space | — | — | $\Theta(n)$ | $\Theta(n)$ | Contiguous flat vector storage without pointer overhead. |
| **TriageMinHeap** | | | | | |
| - `changeUrgency` (Dynamic Acuity) | $\Theta(1)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ | Directly indexes heap position via `heapPosByHandle[h]`; executes sift-up (deterioration) or sift-down (stabilization). Preserves original `arrivalSequence`. |
| - Arbitrary Removal | $O(\log n)$ | $O(\log n)$ | $O(\log n)$ | $O(1)$ | Relocates tail element to target position, updates handle index, sifts. |
| - Total Storage Space | — | — | $\Theta(n + H)$ | $\Theta(n + H)$ | Heap array ($n$) + dense handle-to-position mapping array ($H = \text{maxHandle}$). |

---

## 3. Graph Algorithms & Campus Routing (CO6)

| Algorithm / Operation | Time Complexity | Auxiliary Space | Graph Representation | Key Implementation Details |
|---|---|---|---|---|
| **Graph Edge Inspection** | $O(1)$ | $O(1)$ | Adjacency Matrix | Inspects matrix cell `matrix[u][v]`. |
| **Incident Edge Iteration** | $O(\text{deg}(u))$ | $O(1)$ | Adjacency List | Traverses singly-linked bucket chain of vertex $u$. |
| **Breadth-First Search (BFS)** | $O(V + E)$ | $O(V)$ | Adjacency List | Uses `CustomQueue<int>` and boolean `visited` array. Explores all reachable wings in component. |
| **Depth-First Search (DFS List)** | $O(V + E)$ | $O(V)$ | Adjacency List | Recursive or stack-based traversal exploring incident edges only. |
| **Depth-First Search (DFS Matrix)** | $O(V^2)$ | $O(V)$ | Adjacency Matrix | Scans all $V$ cells per vertex row, demonstrating $O(V^2)$ overhead. |
| **Cycle Detection** | $O(V + E)$ | $O(V)$ | Adjacency List | Undirected cycle detection with explicit parent edge exclusion. Visits all components. |
| **Dijkstra Shortest Path** | $O((V + E) \log V)$ | $O(V)$ | Adjacency List + BinaryMinHeap | Guarded integer arithmetic (`safeDistanceAdd`, saturation at `kInfinity`). Reverse path reconstructed via `CustomStack<int>`. |
| **Kruskal's Algorithm (MST/MSF)** | $O(E \log E) + O(V^2)$ | $O(V + E)$ | Adjacency List + DisjointSetUnion | Scans $V \times V$ matrix to extract $E$ unique undirected edges, sorts via `customMergeSort`, uses path compression and union by rank. Accurately reports Spanning Forest when disconnected. |
| **Prim's Algorithm (MST/MSF)** | $O((V + E) \log V)$ | $O(V)$ | Adjacency List + BinaryMinHeap | Grows minimum cut edges from start vertex. Visits every component to produce full Minimum Spanning Forest on disconnected graphs. |
| **Disjoint Set Union (`find`)** | $O(\alpha(V))$ amortized | $O(1)$ | Array (`parent`) | Uses recursive path compression. $\alpha$ is the Inverse Ackermann function ($\alpha(V) \le 4$ for all practical inputs). |
| **Disjoint Set Union (`unite`)** | $O(\alpha(V))$ amortized | $O(1)$ | Array (`rank`, `parent`) | Uses union by rank to keep tree height bounded. |

---

## 4. Custom Sorting Suite (CO1, CO5)

| Sorting Algorithm | Best Case | Average Case | Worst Case | Stability | Auxiliary Space | Deterministic Telemetry Conventions |
|---|---|---|---|---|---|---|
| **Merge Sort** | $\Theta(n \log n)$ | $\Theta(n \log n)$ | $\Theta(n \log n)$ | **STABLE** | $\Theta(n)$ | Comparisons: 1 per element check; Swaps: 0; Moves: 1 per assignment to/from temporary buffer. Bounded by exact $O(n \log n)$ in all cases. |
| **Quicksort** | $\Theta(n \log n)$ | $\Theta(n \log n)$ | $O(n^2)$ | **UNSTABLE** | $O(\log n)$ | Median-of-Three pivot selection (`first`, `mid`, `last`); Lomuto partitioning; tail-call optimization on smaller partition bounds recursion stack to $O(\log n)$. |
| **Heapsort** | $\Theta(n \log n)$ | $\Theta(n \log n)$ | $\Theta(n \log n)$ | **UNSTABLE** | **$\Theta(1)$** | Strictly in-place array transformation. Floydean bottom-up heap construction in $\Theta(n)$, followed by $n-1$ root-to-tail extractions with sift-down. |
