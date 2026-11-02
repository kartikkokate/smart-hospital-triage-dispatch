# Data Structures-I Viva Defense & Oral Examination Guide
**Course:** CS2305 — Data Structures-I  
**Audience:** Second-Year B.Tech Computer Engineering Students  

This guide provides concise, intuitive explanations, architectural justifications, edge cases, and direct model answers to top viva questions asked by external examiners.

---

## 1. Core Architectural Concepts & Academic Justifications

### Q1: Why did you implement custom data structures instead of using C++ STL containers?
> **Model Answer:**
> "The primary learning objective of CS2305 is to understand and implement fundamental memory layouts, pointer manipulation, and algorithmic invariants from first principles. STL containers like `std::priority_queue` or `std::unordered_map` abstract away internal node structures and do not provide necessary features for our hospital simulation:
> 1. `std::priority_queue` does not support updating priorities in-place without re-inserting and causing stale duplicate entries. Our custom `TriageMinHeap` uses direct handle position tracking (`heapPosByHandle`) to update urgency in $O(\log n)$ time while strictly preserving the initial FIFO arrival order.
> 2. `std::list` and `std::queue` do not permit zero-allocation node transfers, whereas our `CustomQueue` allows detaching a waiting appointment node and converting it to emergency triage without dynamic memory allocations during an emergency.
> 3. Standard containers make it impossible to instrument exact operation telemetry (comparisons, swaps, moves, and tombstone collisions) for oral viva demonstration."

---

### Q2: Why maintain both an Adjacency List and an Adjacency Matrix for the campus graph?
> **Model Answer:**
> "The hospital campus graph is sparse ($V = 12$ wings, $E = 15$ corridors). 
> - An **Adjacency List** uses $\Theta(V + E)$ memory and allows Dijkstra's algorithm and Kruskal's edge extraction to iterate strictly over existing physical corridors without inspecting nonexistent links.
> - An **Adjacency Matrix** provides $O(1)$ edge existence and weight lookup (`matrix[u][v]`), but requires $\Theta(V^2)$ memory and $\Theta(V^2)$ traversal time.
> We maintain both synchronized representations to demonstrate the concrete trade-off in our educational module (Menu 7, Demo 3): traversing the 6-node demo graph takes 12 operations on the Adjacency List versus 36 operations on the Matrix."

---

### Q3: What is the fundamental difference between Dijkstra's algorithm and a Minimum Spanning Tree (MST)?
> **Model Answer:**
> "They solve fundamentally different optimization problems:
> 1. **Dijkstra's Algorithm (Point-to-Point Local Latency):** Minimizes the total transit time along a single path between a *specific* source vertex and destination vertex (e.g., finding the fastest 270-second emergency gurney route from Ambulance Bay to Surgical OT).
> 2. **Minimum Spanning Tree (Global Network Connectivity):** Connects *all* $V$ vertices together using exactly $V - 1$ edges such that the *sum of all edge weights is minimized* (e.g., establishing a 1280-second hospital-wide emergency telemetry or communication backbone).
> **Critical Viva Insight:** An edge belonging to an MST is NOT guaranteed to be on the shortest path between two specific vertices, and the shortest path between two vertices may not be part of the MST."

---

### Q4: Why is Hash Table lookup described as 'expected O(1)' rather than guaranteed O(1)?
> **Model Answer:**
> "Hash functions map an infinite or large key space to a finite table of $M$ buckets. When multiple distinct keys produce the same bucket index (hash collision), they must be resolved via chaining or open addressing.
> - Under the assumption of simple uniform hashing, keys distribute evenly across buckets, giving an average chain length equal to the load factor $\alpha = n / M \le 0.75$, which is $O(1)$ average time.
> - However, in the worst-case pathological scenario (e.g., an adversarial input where all keys hash to the exact same bucket), the table degrades into an $O(n)$ linear linked list. Therefore, lookup is formally **expected $O(1)$**."

---

### Q5: What is a 'tombstone' in open addressing, and why is it necessary?
> **Model Answer:**
> "In open addressing with linear probing, when two keys collide, the second key probes forward to the next available empty slot.
> If we delete the first key by simply marking its slot as `EMPTY`, any subsequent search for the second key would hash to the first slot, find it empty, and prematurely terminate, reporting a false negative.
> A **tombstone (`DELETED` sentinel)** solves this:
> 1. `search()` continues probing past tombstones to find downstream keys.
> 2. `insert()` can reuse a tombstone slot to store a new key, keeping memory utilization optimal.
> 3. During `rehash()`, tombstones are purged, restoring clean contiguous clusters."

---

### Q6: Why is Merge Sort stable, while Quicksort and Heapsort are unstable?
> **Model Answer:**
> "- **Merge Sort is Stable:** During the merge phase, when two elements have equal keys (`left[i] == right[j]`), the algorithm explicitly selects the element from the left subarray first (`<=` comparison). Equal elements never overtake each other, strictly preserving the original chronological FIFO order.
> - **Quicksort is Unstable:** Long-range partitioning swaps swap elements across large array distances over the pivot, which can easily move an earlier element past an identical later element.
> - **Heapsort is Unstable:** The algorithm repeatedly swaps the root element with the tail element of the heap and sifts down. These root-to-tail swaps disrupt the relative ordering of duplicate keys."

---

### Q7: Explain the three cases of Binary Search Tree (BST) node deletion.
> **Model Answer:**
> "When deleting key $K$ from a BST:
> 1. **Case 1 (Leaf Node — 0 children):** Simply unlink the node from its parent (set parent's left/right pointer to `nullptr`) and deallocate the node.
> 2. **Case 2 (Single Child — 1 child):** Bypass the node by linking the parent directly to the node's single child, then deallocate the node.
> 3. **Case 3 (Two Children):** 
>    - Locate the node's **in-order successor** (the smallest key in its right subtree, found by going right once then left as far as possible).
>    - Copy the successor's key and value into the target node.
>    - Recursively delete the successor from the right subtree (which is guaranteed to fall into Case 1 or Case 2 because the in-order successor cannot have a left child)."

---

### Q8: How does dynamic emergency triage update preserve patient arrival sequence?
> **Model Answer:**
> "In an emergency department, a patient's condition can deteriorate (e.g., Urgency 3 $\to$ 1) or stabilize while waiting.
> In our custom `TriageMinHeap`:
> 1. Every patient is assigned a monotonic, unique `arrivalSequence` integer upon registration.
> 2. The min-heap uses a composite comparator: primary key is `urgency` (lower number = higher clinical priority), secondary key is `arrivalSequence` (lower number = earlier arrival).
> 3. When `updateUrgency()` is called, the patient's position is located in $O(1)$ via the dense `heapPosByHandle` index.
> 4. The urgency field is updated, but the `arrivalSequence` remains completely untouched.
> 5. Sift-up or sift-down is executed in $O(\log n)$ time.
> As verified in test `testEqualUrgencyFIFOAndUrgencyUpdates`, when patient P108 deteriorates to Urgency 1, they are extracted after earlier Urgency 1 patients (P104 and P107) but before later arrivals (P112), strictly honoring clinical justice."

---

### Q9: Why does Dijkstra's algorithm fail on graphs with negative edge weights?
> **Model Answer:**
> "Dijkstra's algorithm relies on a greedy choice property: once a vertex $u$ is extracted from the priority queue (its minimum distance is finalized), it assumes no shorter path to $u$ can ever be discovered.
> If negative edge weights are present, traversing a negative edge later in the graph could reduce the path cost to an already-finalized vertex, invalidating the greedy assumption.
> In our hospital campus, edge weights represent physical corridor transit durations in seconds, which are strictly non-negative ($w \ge 0$). Our `DistanceArithmetic.hpp` module actively asserts non-negative weights and guards against integer overflow."

---

### Q10: How does your implementation prevent false cycle detections in undirected graph DFS?
> **Model Answer:**
> "In an undirected graph, an edge between $u$ and $v$ appears in both directions ($u \to v$ and $v \to u$).
> In a naive DFS, visiting $v$ from $u$ and then inspecting the back-edge $v \to u$ would immediately mistake the undirected corridor for a 2-node cycle.
> Our DFS cycle detection algorithm explicitly tracks the `parent` vertex for every recursive exploration. When inspecting neighbors of $v$, if neighbor $w == \text{parent}$, the edge is recognized as the reciprocal undirected corridor and is safely ignored. A true cycle is flagged only when an edge leads to an already-visited vertex that is *not* the direct parent."

---

## 2. Quick Viva Defense Cheat-Sheet

| Topic | Key Term to Mention | Danger Pitfall to Avoid |
|---|---|---|
| **Hash Tables** | Polynomial rolling hash, load factor $\alpha \le 0.75$, prime rehash, tombstones. | Claiming lookup is *always* $O(1)$ (must say *expected* or *average*). |
| **Heaps** | Floydean bottom-up $\Theta(n)$ build, complete binary tree, array indexing (`2i+1`, `2i+2`). | Confusing binary search tree order with binary heap order. |
| **Graphs** | Greedy choice, cut property, union-by-rank, inverse Ackermann $\alpha(V)$. | Calling Dijkstra an all-pairs algorithm (it is single-source). |
| **Sorting** | Stability, Median-of-Three pivot, in-place auxiliary space, tail recursion bounding. | Claiming Quicksort is always $O(n \log n)$ (worst case is $O(n^2)$). |
| **State Machine** | Reciprocal assignment, immediate handover release, independent value snapshot. | Claiming resources are released at patient discharge (they release at handover). |
