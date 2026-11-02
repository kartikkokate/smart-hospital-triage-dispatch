# Smart Hospital Emergency Triage & Route Dispatch System
## Comprehensive Academic Project Report

**Course:** CS2305 — Data Structures-I  
**Program:** B.Tech Computer Engineering (Second Year, Semester 1, AY 2026–27)  
**Implementation Language:** C++17 (Strict ANSI, Single-Threaded, Console/Terminal)  
**Standard Library Policy:** Foundational language primitives only (`std::vector` for dynamic storage, `std::string`, `std::unique_ptr`, `std::iostream`, `std::fstream`, `std::swap`). Zero STL containers (`std::map`, `std::unordered_map`, `std::set`, `std::list`, `std::queue`, `std::stack`, `std::priority_queue`) used for domain models or operational indexes.

---

## 1. Problem Statement & Motivation

Hospital emergency departments and large multi-wing medical campuses face two critical, concurrent operational challenges:
1. **Dynamic Emergency Patient Triage & Prioritization:** Patients arrive unpredictably with varying medical acuity (Urgency Level 1: Resuscitation to Level 5: Non-Urgent). Waiting patients can dynamically deteriorate or stabilize, necessitating in-place priority updates while preserving chronological arrival sequences (FIFO tie-breaking within identical urgency tiers). Routine outpatient consultations run in parallel on a deterministic schedule.
2. **Campus Navigation & Rapid Route Dispatch:** Hospital campuses span specialized physical wards (Ambulance Bay, ER Triage, Resuscitation Bay, Surgery OT, ICU, Radiology/CT, Blood Bank, Pharmacy, Isolation). Inter-wing corridors experience temporary maintenance or sterilization closures. Transfer teams (gurneys) must be dispatched along minimum-time physical paths, avoiding blocked corridors. In addition, hospital engineers periodically compute minimal spanning infrastructure (MST) for inter-wing telemetry and emergency backup interconnects.

This project unifies syllabus Themes A (*Hospital Patient Management*) and B (*Campus Navigation & Rescue Planning*) into a robust, single-threaded simulation engineered entirely on custom, from-scratch data structures.

---

## 2. Project Scope & Operational Boundaries

### 2.1 Implemented Baseline Scope
- **Patient Management:** Authoritative registry with expected $O(1)$ Hash Table lookup (`HashTableChaining`) and hierarchical demographic search via Binary Search Tree (`BinarySearchTree`).
- **Emergency Triage:** Custom array-backed Binary Min-Heap (`TriageMinHeap`) with composite comparator `(urgency, arrivalSequence)`, dense handle indexing (`heapPosByHandle`), and arrival-preserving $O(\log n)$ dynamic acuity updates.
- **Outpatient Consultations:** Strict FIFO outpatient appointment booking, calling, cancellation, and emergency deterioration conversion via custom Queue (`CustomQueue`).
- **Dispatch Fleet Management:** Two-leg deterministic routing (Leg 1: resource to patient; Leg 2: patient to clinical destination) selecting candidates by shortest reachable pickup time and resolving ties by resource ID.
- **Campus Graph Routing:** Dual synchronized Adjacency List (`SinglyLinkedList`) and Adjacency Matrix (`CampusGraph`), Dijkstra shortest paths with stack-based route reconstruction (`CustomStack`), BFS reachability, DFS cycle detection, and Kruskal (with custom Disjoint Set Union) and Prim algorithms for Minimum Spanning Trees (MST) and Spanning Forests (MSF).
- **Audit Logging:** Chronological, immutable patient lifecycle event snapshots recorded in a custom Doubly Linked List (`DoublyLinkedList`).
- **Reporting & Sorting:** Independent value-copy report generation (`PatientSnapshot`) with custom recursive Merge Sort, Quicksort (Median-of-3, Lomuto), and in-place Heapsort.
- **Interactive UI & Verification:** Iterative, stream-injectable console interface (`ConsoleApp`) and deterministic self-validating noninteractive verification (`--demo`).

### 2.2 Optional / Unimplemented Syllabus Topics
To preserve architectural integrity and avoid superficial features, the following syllabus extension ideas were explicitly scoped out and left unimplemented:
- *Infix-to-Postfix Arithmetic Expression Evaluator:* A generic math expression parser is not operationally relevant to hospital patient logistics.
- *Sparse Bed Matrix with Fast Transpose:* A $3 \times 12$ matrix of campus beds is dense rather than sparse; compressing it into triplets adds needless complexity without runtime benefit.
- *Circular Linked List Staff Roster:* On-call physician shift rotations are orthogonal to real-time emergency transport and dispatch.

---

## 3. System Architecture & Component Interactions

```
                                [ ConsoleApp (UI) ]
                                         │
                                         ▼
                            [ HospitalSystem Facade ]
                                         │
     ┌───────────────────┬───────────────┴───────────────┬───────────────────┐
     ▼                   ▼                               ▼                   ▼
[HospitalRegistry]  [TriageManager]             [AppointmentManager] [DispatchManager]
  ├─ Patient Storage   └─ TriageMinHeap           └─ CustomQueue       └─ CampusMapManager
  ├─ HashTableChaining      (BinaryMinHeap)            (FIFO Appts)         └─ CampusGraph
  └─ BinarySearchTree                                                           ├─ AdjList
                                                                                ├─ AdjMatrix
                                                                                └─ Dijkstra/MST
```

All user interactions and automated test workflows communicate strictly through the `HospitalSystem` facade. The facade enforces:
- Valid lifecycle state machine transitions.
- Reciprocal assignment matching (`patient.assignedResourceId` $\leftrightarrow$ `resource.assignedPatientId`).
- Checked arithmetic on all simulation timestamps and corridor edge costs (`int64_t`).
- Immutable event generation into `DoublyLinkedList<AuditEvent>`.

---

## 4. Custom Data Structure Rationale & Invariants

| Component | Abstract Data Type | Custom Implementation Class | Design Rationale & Invariants |
|---|---|---|---|
| Primary Patient Registry | Hash Table (Separate Chaining) | `HashTableChaining<std::string, Patient>` | Expected $O(1)$ lookup by alphanumeric patient ID. Uses polynomial rolling hash $H(s) = (\sum s[i] \cdot 31^i) \pmod M$. Auto-rehashes to prime $M \ge 2M+1$ when load factor $\alpha > 0.75$. |
| Patient Range / Tree Index | Binary Search Tree | `BinarySearchTree<std::string, Patient*>` | $O(h)$ demographic and ordered queries (average $O(\log n)$, worst-case $O(n)$). Full 3-case deletion (leaf, 1-child, 2-children with in-order successor). Non-owning pointer policy prevents entity deallocation on tree removal. |
| Emergency Acuity Triage | Binary Min-Heap | `TriageMinHeap` (wrapping `BinaryMinHeap`) | $O(1)$ top-priority access. Composite comparator orders by Urgency Level (1–5) first, then monotonic `arrivalSequence` (FIFO). Direct handle tracking (`heapPosByHandle`) enables $O(\log n)$ sift-up/down in-place condition updates. |
| Outpatient Clinic Roster | FIFO Queue | `CustomQueue<std::string>` | Strict FIFO scheduling. Node-based queue with head and tail pointers. Safely resets head and tail to `nullptr` upon dequeuing the final element. Supports $O(n)$ arbitrary cancellation and zero-allocation node transfer. |
| Dispatch Route Backtracking | LIFO Stack | `CustomStack<int>` | Reconstructs shortest path from Dijkstra predecessor array in reverse order, popping nodes in chronological forward travel order. Safe underflow protection. |
| Campus Spatial Topology | Undirected Weighted Graph | `CampusGraph` | Dual synchronized representations: Adjacency List (`SinglyLinkedList<Edge>`) for $O(V+E)$ iteration and Adjacency Matrix (`int64_t[][]`) for $O(1)$ edge existence queries and $O(V^2)$ traversal demonstrations. Explicit sentinels (`NO_EDGE = -1`, `BLOCKED_EDGE = -2`). |
| Minimum Spanning Tree Helper | Disjoint Set Union (DSU) | `DisjointSetUnion` | Cycle detection for Kruskal's algorithm. Uses path compression and union by rank, achieving nearly $O(1)$ amortized time per operation ($O(\alpha(V))$). |
| Audit Trail / Event History | Doubly Linked List | `DoublyLinkedList<AuditEvent>` | Chronological audit trail. Bidirectional pointers allow forward chronological log inspection and reverse most-recent-first inspection. Preserves immutable snapshots. |
| Report Sorting Engine | Sorting Algorithms | `customMergeSort`, `customQuickSort`, `customHeapSort` | Completely isolated sorting algorithms. Stable Merge Sort ($O(n \log n)$ time, $O(n)$ space); Unstable Quicksort (Median-of-3, Lomuto partition, $O(\log n)$ recursion stack); In-Place Heapsort ($O(n \log n)$ time, $O(1)$ auxiliary space). |

---

## 5. End-to-End Workflow & Discrete Simulation

```
[Arrival / Registration] ──> [TriageMinHeap] ──> [Dispatch Selection] ──> [Resource Assignment]
                                                         │
                                                         ▼
[Discharge] <── [Clinical Treatment] <── [Handover] <── [Pickup Transit]
```

1. **Intake:** Patients are registered with validated unique ID, name, age, complaint, location, and non-negative billing amount.
2. **Prioritization:** Emergency patients enter `TriageMinHeap`; routine patients enter `CustomQueue`.
3. **Dynamic Acuity Adjustment:** Condition deterioration (e.g. Urgency 3 to 1) triggers min-heap sift-up while strictly preserving the initial arrival sequence.
4. **Dispatch Selection & Two-Leg Routing:**
   - Candidate is selected based on highest priority in triage or waiting pending dispatch.
   - Leg 1 (Pickup): Best available resource chosen by minimum transit time to patient location (tie-broken by lexicographical resource ID).
   - Leg 2 (Transport): Validated route from patient location to target treatment unit.
5. **Simulation Movement:**
   - Pickup Step: Advances simulation clock by Leg 1 transit time. Resource state changes from `EN_ROUTE_PICKUP` to `TRANSPORTING_PATIENT`; patient transitions from `DISPATCH_PENDING` to `IN_TRANSIT`.
   - Handover Step: Advances clock by Leg 2 transit time. Patient transitions to `UNDER_TREATMENT`. The resource is **immediately released** back to `AVAILABLE` at the destination wing.
6. **Discharge:** Upon procedure completion, patient transitions to terminal state `DISCHARGED`. Reused resources are completely isolated from discharge actions.

---

## 6. Verification Results Summary

The system is rigorously verified via two complementary harnesses:
1. **Automated Test Battery (`bin/TestRunner.exe`):**
   - 41 test suites encompassing 96 individual assertions and subtests.
   - Full pass rate: **41 Passed, 0 Failed (100% pass rate)**.
   - Covers: unit operations on all 11 custom data structures, exception safety rollbacks, distance arithmetic saturation, seed data cross-validation, two-leg routing and rerouting, pickup cancellation, snapshot decoupling, sorting stability, and 9 scripted console UI workflows.
2. **Automated Noninteractive Verification (`bin/SmartHospitalSystem.exe --demo`):**
   - Self-validating execution verifying all required operations, states, and graph totals (e.g., 270s route, 1280s MST weight).
   - Terminates deterministically with exit code 0.

---

## 7. Known Environment Limitations

1. **Toolchain Dependency:** The provided convenience script (`build_and_test.bat`) relies on the local MinGW compiler installed at `C:\MinGW\bin\g++.exe`. Portable compilation on other machines requires standard GCC, Clang, or MSVC supporting C++17.
2. **In-Memory Storage:** All patient, resource, and graph state resides purely in RAM. Terminating the application clears live runtime state; seed data is reloaded on next startup.
3. **Unverified Host Capabilities:** CMake/CTest, alternative compilers, and memory diagnostics (Valgrind/AddressSanitizer) remain marked unverified in this local environment because those tools are not installed in the Windows system PATH.

---

## 8. Conclusion

The Smart Hospital Emergency Triage & Route Dispatch System fulfills all academic and architectural requirements of CS2305 Data Structures-I. By substituting all standard library containers with robust, custom implementations and validating every invariant through automated testing, the project demonstrates mastery across the entire data structures curriculum (Course Outcomes CO1 through CO6).
