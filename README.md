# Smart Hospital Emergency Triage & Route Dispatch System
**Course:** CS2305 — Data Structures-I, B.Tech Computer Engineering, AY 2026–27  
**Technology Stack:** C++17, Console/Terminal (Single-Threaded), MinGW GCC 6.3  
**Status:** Milestone 8 Verified & Complete (Final Submission Ready: Self-Validating `--demo`, Corrected CO1–CO6 Mapping, Strict CLI Parser, Comprehensive Documentation Suite, and 41/41 Automated Tests Passing)

---

## 1. Project Overview
The **Smart Hospital Emergency Triage & Route Dispatch System** is a realistic, single-threaded educational simulation designed to demonstrate mastery of all core data structures and algorithms across Course Outcomes **CO1 through CO6**:
- **CO1 (Arrays, Recursion, Sorting):** Contiguous vector-backed storage, recursive divide-and-conquer Merge Sort, recursive Quicksort with median-of-3 partitioning, and sorting telemetry tracking (comparisons, moves, swaps, stability).
- **CO2 (Linked Lists, Hashing):** Custom `SinglyLinkedList` with forward iterators, `DoublyLinkedList` audit trail with bidirectional pointers, polynomial rolling hash table with separate chaining (`HashTableChaining`, expected $O(1)$ lookup), and open addressing linear probing (`HashTableProbing`).
- **CO3 (Stacks, Queues):** Linked node LIFO `CustomStack` for Dijkstra shortest path reconstruction and iterative BST traversals; linked node FIFO `CustomQueue` for outpatient appointments, consultation turns, and breadth-first search.
- **CO4 (Binary Trees, BST):** Node-based `BinarySearchTree` for secondary patient indexing ($O(h)$ search/insert/delete, where $h$ is tree height, $O(\log n)$ average, $O(n)$ worst-case), in-order/pre-order/post-order/level-order traversals, 3-case node deletion with in-order successor replacement, and independent cloned mirror tree (`MirrorBinaryTree`).
- **CO5 (Heaps, Priority Applications, Heapsort):** Generic `BinaryMinHeap` for graph algorithms, composite `TriageMinHeap` with acuity levels (1 to 5) and arrival sequence FIFO tie-breaking (`heapPosByHandle`), dynamic priority updates preserving arrival order, and strictly in-place $O(1)$ auxiliary space Heapsort.
- **CO6 (Graphs, Traversal, MST, Shortest Paths):** Dual synchronized campus graph representations (Adjacency Matrix and Adjacency List), BFS and DFS traversals, cycle detection, Dijkstra's algorithm for optimal route planning (270s seed route), dynamic corridor obstruction and rerouting, and Minimum Spanning Tree / Forest computation via Kruskal's (with custom DSU) and Prim's algorithms (1280s total weight).

---

## 2. Directory Structure & Key Files
```
SmartHospitalSystem/
├── CMakeLists.txt                  # Portable C++17 build configuration (SmartHospitalSystem_CompleteTestSuite)
├── README.md                       # Comprehensive system guide, build instructions, and verification summary
├── plan.md                         # Authoritative project milestone specifications and audit records
├── build_and_test.bat              # Local MinGW batch compilation and test execution script
├── include/
│   ├── models/
│   │   ├── Patient.hpp             # Patient model, UrgencyLevel (1-5), PatientState, dense handle tracking
│   │   ├── DispatchResource.hpp    # Gurney model, ResourceState, assignment tracking
│   │   ├── CampusLocation.hpp      # Campus node definition, CorridorEdge
│   │   ├── AuditEvent.hpp          # Immutable historical snapshot record for audit trails
│   │   └── PatientSnapshot.hpp     # Independent value copy representing a patient at an exact timestamp
│   ├── algorithms/
│   │   ├── DistanceArithmetic.hpp  # Production int64_t safe distance arithmetic & infinity saturation
│   │   └── CustomSorting.hpp       # Merge Sort (Stable), Quicksort (Median-of-3), Heapsort (In-Place O(1))
│   ├── ds/                         # Custom data structures implemented from scratch
│   │   ├── SinglyLinkedList.hpp    # Singly linked list with forward iterators and iterative cleanup
│   │   ├── DoublyLinkedList.hpp    # Doubly linked list with bidirectional pointers and integrity check
│   │   ├── CustomStack.hpp         # LIFO stack with underflow protection and iterative cleanup
│   │   ├── CustomQueue.hpp         # FIFO queue with safe final-element reset, contains, remove, forEach
│   │   ├── HashTableChaining.hpp   # Hash table using polynomial hash, separate chaining & prime rehash
│   │   ├── HashTableProbing.hpp    # Open-addressing hash table with tombstones for educational comparison
│   │   ├── BinarySearchTree.hpp    # BST with 3-case deletion, traversals, and independent mirror copy
│   │   ├── BinaryMinHeap.hpp       # Generic BinaryMinHeap and transactional TriageMinHeap
│   │   ├── DisjointSetUnion.hpp    # Disjoint Set Union (DSU) with path compression & union by rank
│   │   └── CampusGraph.hpp         # Synchronized Adjacency List & Matrix, BFS/DFS, Dijkstra, Kruskal & Prim
│   ├── services/                   # Integrated hospital, dispatch, and educational services
│   │   ├── DataLoader.hpp          # Strict CSV and map topology loader with campus cross-validation
│   │   ├── HospitalRegistry.hpp    # Authoritative patient store with hash table, BST, and dense handle indexes
│   │   ├── CampusMapManager.hpp    # Campus map topology, corridor closures, reachability & MST coordinator
│   │   ├── TriageManager.hpp       # Emergency triage priority manager with dynamic urgency updates
│   │   ├── AppointmentManager.hpp  # Outpatient appointment queue, FIFO calls, cancellation, and triage conversion
│   │   ├── DispatchManager.hpp     # Deterministic two-leg routing, resource selection, movement, and handover release
│   │   ├── HospitalSystem.hpp      # Top-level coordinating facade, state transitions, dispatch policy, audit log
│   │   ├── ReportingService.hpp    # Non-mutating independent value-copy report generation and custom sorting
│   │   └── DemonstrationService.hpp# Standalone educational demonstrations for CO1-CO6 concepts
│   └── ui/
│       └── ConsoleApp.hpp          # Stream-injectable iterative console user interface and input parser
├── src/
│   ├── main.cpp                    # Application entry point supporting interactive mode and self-validating --demo
│   ├── models/
│   │   ├── Patient.cpp             # Patient model utilities and string conversions
│   │   └── DispatchResource.cpp    # Resource model string conversions
│   └── services/
│       └── DataLoader.cpp          # Strict, transactional parser for patients, resources, and map
├── data/
│   ├── patients_seed.csv           # 12 deterministic synthetic patient records (P101 to P112)
│   ├── resources_seed.csv          # 3 deterministic transfer gurneys (GURNEY-01 to GURNEY-03)
│   └── campus_map_seed.txt         # 12-node, 15-edge campus topology with verified weights (1850s sum)
├── tests/
│   └── TestRunner.cpp              # Automated test harness with 41 deterministic verification test modules
└── docs/                           # Academic deliverables, viva preparation, and project reports
    ├── project_report.md           # Formal project report covering scope, architecture, data models, and results
    ├── co_mapping.md               # Detailed mapping of CO1–CO6 to source code, features, and test evidence
    ├── complexity_reference.md     # Mathematical time/space complexity bounds and implementation trade-offs
    ├── viva_prep.md                # Second-year student language viva defense guide and faculty Q&A
    ├── demo_script.md              # Reproducible 5–8 minute live demonstration walkthrough script
    └── verification_report.md      # Executed tests, actual toolchain results, and unverified capabilities
```

---

## 3. Custom Data Structures & Operations Summary

| Data Structure | Primary Implementation & Invariants | Time Complexity (Best / Avg / Worst) | Auxiliary Space |
| :--- | :--- | :--- | :--- |
| `SinglyLinkedList<T>` | Head/tail node pointers; forward iterators; deep-copy and move semantics; safe iterative cleanup. | Push Front/Back: $O(1)$<br>Pop Front: $O(1)$<br>Search / Remove: $O(n)$ | $O(n)$ total<br>$O(1)$ auxiliary |
| `DoublyLinkedList<T>` | Prev/Next node pointers; integrity checker; head/tail reset to `nullptr` on final removal. | Push/Pop Front/Back: $O(1)$<br>Search / Mid-Remove: $O(n)$ | $O(n)$ total<br>$O(1)$ auxiliary |
| `CustomStack<T>` | Linked node LIFO container; iterative `clear()`; underflow protection. | Push / Pop / Top: $O(1)$<br>Clear: $O(n)$ | $O(n)$ total<br>$O(1)$ auxiliary |
| `CustomQueue<T>` | Linked node FIFO container; head/tail pointers; encapsulated `contains`, `remove`, `detachNode`. | Enqueue / Dequeue / Front: $O(1)$<br>Arbitrary Removal: $O(n)$ | $O(n)$ total<br>$O(1)$ auxiliary |
| `HashTableChaining<K, V>` | Array of bucket linked lists; polynomial rolling hash $H(s)$; dynamic rehashing at load factor $\alpha > 0.75$. | **Expected Lookup:** $O(1)$<br>Best: $\Omega(1)$<br>Worst-Case: $O(n)$ (all keys collide into single bucket)<br>Rehash: $O(n + M)$ | $O(n + M)$ total<br>$O(1)$ auxiliary |
| `HashTableProbing<K, V>` | Open addressing with linear probing; tri-state slots (`EMPTY`, `OCCUPIED`, `DELETED`); tombstone reuse. | **Expected Lookup:** $O(1)$<br>Worst-Case: $O(M)$ (full cluster traversal)<br>Rehash: $O(n + M)$ avg, $O(n^2)$ worst | $O(M)$ table slots<br>$O(1)$ auxiliary |
| `BinarySearchTree<K, V>` | Node-based BST; duplicate in-place value update; 3-case deletion with in-order successor; non-owning pointer policy. | **Ordinary Search / Insert / Delete:** $O(h)$<br>Average: $O(\log n)$ (random balanced inputs)<br>Worst-Case: $O(n)$ (degenerate sorted inputs) | $O(n)$ total<br>$O(h)$ traversal stack |
| `MirrorBinaryTree<K, V>` | Cloned inverted binary tree (`mirror->left = clone(right)`); intentionally omits BST search methods. | Traversal / Mirror Creation: $O(n)$ | $O(n)$ total<br>$O(h)$ recursion |
| `BinaryMinHeap<T, Comp>` | Contiguous vector-backed min-heap; bottom-up build; const pointer return prevents external corruption. | Peek: $O(1)$<br>Insert / ExtractMin: $O(\log n)$<br>Bottom-Up Build: $O(n)$ | $O(n)$ vector<br>$O(1)$ auxiliary |
| `TriageMinHeap` | Acuity levels (1–5) primary + arrival sequence FIFO tie-breaker; dynamic `changeUrgency` in $O(\log n)$ via `heapPosByHandle`. | Peek: $O(1)$<br>Insert / ExtractMin: $O(\log n)$<br>Dynamic Acuity Sift: $O(\log n)$ | $O(n)$ heap + handle pos<br>$O(1)$ auxiliary |
| `DisjointSetUnion` | Disjoint set forest with path compression and union by rank; near-linear performance. | Find / Unite: $O(\alpha(V))$ amortized (essentially $O(1)$) | $O(V)$ parent/rank arrays |
| `CampusGraph` | Dual synchronized representations: weighted Adjacency List (`SinglyLinkedList`) and Adjacency Matrix ($V \times V$). | Central mutation ensures agreement.<br>Corridor weight: $w \ge 0$, sentinels `-1` (no edge), `-2` (blocked). | $O(V^2 + E)$ total |
| `Dijkstra's Algorithm` | Shortest path finder using `BinaryMinHeap` and `CustomStack` path reversal. | Time: $O((V + E) \log V)$ | $O(V)$ distances & heap |
| `Kruskal's Algorithm` | MST finder using custom Merge Sort on edges and Disjoint Set Union. | Time: $O(E \log E) + O(V^2)$ matrix scan | $O(E + V)$ auxiliary |
| `Prim's Algorithm` | Vertex-centric MST finder using `BinaryMinHeap` and `inMST` tracking. | Time: $O((V + E) \log V)$ | $O(V)$ keys & heap |
| `Custom Merge Sort` | Stable divide-and-conquer sorting using temporary auxiliary buffer. | Time: $\Theta(n \log n)$ best, average, and worst-case | $O(n)$ auxiliary buffer |
| `Custom Quicksort` | Unstable in-place sorting using median-of-3 pivot selection and Lomuto partition. | Time: $O(n \log n)$ average, $O(n^2)$ worst-case | $O(\log n)$ recursion stack |
| `Custom Heapsort` | Unstable strictly in-place sorting using bottom-up heapification. | Time: $O(n \log n)$ best, average, and worst-case | **Strictly $O(1)$ auxiliary space** |

---

## 4. Building, Running and CLI Execution Modes

### Option A: Local MinGW Batch Compilation (`build_and_test.bat`)
Run the clean, self-contained batch build script:
```cmd
build_and_test.bat
```
- **Prerequisite:** MinGW GCC 6.3+ installed at `C:\MinGW\bin\g++.exe` (or accessible in PATH).
- **Functionality:**
  1. Anchors working directory to script location (`%~dp0`).
  2. Cleans prior binaries in `bin/` so stale builds never execute.
  3. Recompiles both targets from source:
     * `bin\SmartHospitalSystem.exe`
     * `bin\TestRunner.exe`
     with compiler flags: `-std=c++1z -Wall -Wextra -pedantic`.
  4. Runs all 41 automated tests via `bin\TestRunner.exe data/`.
  5. Executes self-validating demonstration via `bin\SmartHospitalSystem.exe --demo data/`.

> [!WARNING]
> **Machine-Specific Path Limitation:** `build_and_test.bat` explicitly references `C:\MinGW\bin\g++.exe`. If MinGW is installed in another directory on your workstation, either update the path in line 16 of `build_and_test.bat` or compile directly via `g++` from terminal.

---

### Command-Line Execution Modes & Syntax

The main executable (`bin\SmartHospitalSystem.exe`) supports two operating modes and strict error checking:

#### 1. Interactive Console Mode (Default)
```powershell
# Using relative path:
.\bin\SmartHospitalSystem.exe data/

# Using explicit flag:
.\bin\SmartHospitalSystem.exe --data-dir data/

# Using quoted path containing spaces:
& ".\bin\SmartHospitalSystem.exe" "data/"
```
- Launches the complete 9-submenu interactive terminal application.
- Allows live registration, emergency triage, outpatient appointment calls, 2-leg dispatch simulation, corridor blocking/rerouting, report generation, educational demonstrations, and audit verification.
- **In-Memory Volatile Persistence:** All state changes reside exclusively in RAM. Exiting the program resets all structures back to initial seed data without altering CSV/TXT files on disk.

#### 2. Self-Validating Non-Interactive Mode (`--demo`)
```powershell
.\bin\SmartHospitalSystem.exe --demo data/

# Or with quoted path:
& ".\bin\SmartHospitalSystem.exe" --demo "data/"
```
- Automatically verifies seed counts (12 patients, 3 gurneys, 12 nodes, 15 corridors).
- Validates data structure invariants (Stack LIFO, Queue FIFO, DoublyLinkedList audit trail, HashTable lookups for `P104 Marcus Brody` and `GURNEY-02 Critical Care Mobile Unit`, BST count, and Min-Heap root).
- Verifies Dijkstra shortest path from Node 0 to Node 3 via `0 -> 1 -> 5 -> 3` (exact duration: **270s**).
- Verifies Kruskal and Prim Minimum Spanning Trees (exact weight: **1280s**, 11 edges).
- Simulates complete clinical workflow: registration (`P501 Jonathan Wick`), acuity update (`P108` from 4 to 2 preserving sequence `#8`), outpatient consultation (`P102`), dispatch candidate selection, gurney pairing (`GURNEY-01`), Leg 1 pickup, Leg 2 transport/handover, and discharge.
- Checks cross-service system integrity after every lifecycle step.
- Prints `[DEMO COMPLETE] Noninteractive demonstration completed and self-validated successfully.` and **returns exit code 0** on success.
- On any invariant or workflow failure, prints the actual diagnostic error and **returns exit code 1**.

#### 3. CLI Help & Error Handling Verification
- **Help Flag:**
  ```powershell
  .\bin\SmartHospitalSystem.exe --help
  ```
  Displays complete CLI syntax and flag descriptions, exiting with code 0.
- **Strict Error Detection:**
  * Missing argument: `.\bin\SmartHospitalSystem.exe --data-dir` $\implies$ exits with code 1.
  * Repeated flags: `.\bin\SmartHospitalSystem.exe --data-dir data/ --data-dir other/` $\implies$ exits with code 1.
  * Conflicting paths: `.\bin\SmartHospitalSystem.exe --data-dir data/ other/` $\implies$ exits with code 1.
  * Multiple positional: `.\bin\SmartHospitalSystem.exe data/ extra/` $\implies$ exits with code 1.
  * Unknown flags: `.\bin\SmartHospitalSystem.exe --unknown-flag` $\implies$ exits with code 1.

---

### Option B: CMake & CTest Build (Multi-Platform)
*(Status: CMakeLists.txt is maintained for portable multi-platform build systems; unverified on current host as CMake is not in local PATH)*
```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest --output-on-failure
```
- Registers test target: `SmartHospitalSystem_CompleteTestSuite`.

---

## 5. Course Outcome (CO) Mapping

| Course Outcome | Primary Custom Structures & Implementations | Callable Features & UI Locations | Verification & Test Evidence |
| :--- | :--- | :--- | :--- |
| **CO1: Arrays, Recursion, Sorting** | Contiguous vector buffers; recursive Merge Sort; recursive Quicksort (median-of-3); `SortMetrics` telemetry. | Menu 6 (Patient Reports: Sort by Age, Arrival, Urgency, Billing, Waiting); Menu 7 Demo 1. | `testCustomSortingAlgorithms`<br>`testSortingStability`<br>`testPatientSnapshotAndReportingService`<br>Console App Subtest 8 |
| **CO2: Linked Lists, Hashing** | `SinglyLinkedList<T>`; `DoublyLinkedList<T>`; `HashTableChaining<K, V>` (polynomial hash, $\alpha \le 0.75$); `HashTableProbing<K, V>` (linear probing, tombstones). | Menu 1 (Registration, Lookup); Menu 8 (Audit Trail Log); Menu 7 Demo 2 (Collision Benchmark). | `testSinglyLinkedList`<br>`testDoublyLinkedList`<br>`testHashTableChaining`<br>`testHashTableProbing`<br>`testExceptionSafeCopySemantics`<br>`testHospitalRegistryAndRegistration` |
| **CO3: Stacks, Queues** | Linked node `CustomStack<T>`; linked node `CustomQueue<T>`; encapsulated deletion & detach/reinsert. | Menu 3 (Outpatient Clinic Appointments: View, FIFO Call, Cancel, Convert); Menu 5 (Dijkstra Path Reconstruction). | `testCustomStack`<br>`testCustomQueue`<br>`testAppointmentFIFOAndCancellationAndConversion`<br>`testPreserveAppointmentOrderOnFailedConversion`<br>Console App Subtest 4 |
| **CO4: Binary Trees, BST** | Node-based `BinarySearchTree<K, V>`; 3-case node deletion (leaf, 1-child, 2-children with successor); `MirrorBinaryTree<K, V>`. | Menu 1 Option 2 (Patient Dossier Lookup via BST); Menu 7 Demo 4 (Traversals, Mirroring, Deletion). | `testBinarySearchTree`<br>`testHospitalRegistryAndRegistration`<br>`testEducationalDemonstrations` |
| **CO5: Heaps, Priority Applications, Heapsort** | Generic `BinaryMinHeap<T>`; composite `TriageMinHeap` (urgency 1-5, FIFO arrival tie-breaker, `heapPosByHandle`); strictly in-place `customHeapSort`. | Menu 2 (Triage Status, Intake, Urgency Acuity Update); Menu 4 (Dispatch Candidate Selection); Menu 6 Sort 3 (Heapsort). | `testBinaryMinHeapAndTriage`<br>`testEqualUrgencyFIFOAndUrgencyUpdates`<br>`testPendingDispatchOrderingPolicy`<br>`testCustomSortingAlgorithms`<br>Console App Subtest 5 |
| **CO6: Graphs, Traversal, MST, Shortest Paths** | Synchronized Adjacency List & Matrix; BFS; DFS; Dijkstra's algorithm; Kruskal's MST (with DSU); Prim's MST; dynamic corridor blocking. | Menu 5 (Campus Map: View Locations/Corridors, Block/Reopen, Dijkstra Route, BFS/DFS, MST/MSF); Menu 4 (2-Leg Dispatch Routing); Menu 7 Demos 3 & 5. | `testCampusGraphAndRouting`<br>`testSimulationLegsCorridorClosureAndRerouting`<br>`testDeterministicResourceSelectionAndTieBreaking`<br>`testEducationalDemonstrations`<br>Console App Subtest 6 & 7 |

*(For full details and syllabus conflict notes, see [docs/co_mapping.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/co_mapping.md)).*

---

## 6. Academic Deliverables Index

All documentation deliverables are located in the `docs/` folder:

1. [docs/project_report.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/project_report.md): Formal project report covering problem background, architecture, workflows, data structure selection rationale, algorithmic results, limitations, and separation from optional syllabus topics.
2. [docs/co_mapping.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/co_mapping.md): Mapping of Course Outcomes CO1–CO6 to source code, callable features, and automated test modules, including syllabus discrepancy analysis.
3. [docs/complexity_reference.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/complexity_reference.md): Mathematical time/space complexity reference table with best, average, and worst-case bounds, auxiliary space, and implementation overheads.
4. [docs/viva_prep.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/viva_prep.md): Second-year student language viva defense manual with justifications, invariants, edge cases, and answers to top 10 faculty questions.
5. [docs/demo_script.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/demo_script.md): Reproducible 5 to 8 minute viva demonstration walkthrough script with exact keystrokes, inputs, and expected console observations.
6. [docs/demo_transcript.txt](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/demo_transcript.txt): Exact recorded console execution transcript from a fresh launch validating all demo steps.
7. [docs/verification_report.md](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/verification_report.md): Verification report documenting executed tests (41/41 passing), self-validating `--demo` results, CLI test matrix, external path execution, and unverified tools.


---

## 7. Known Limitations & Unverified Tools

- **Single-Threaded Execution:** The system is purposefully single-threaded to model deterministic academic simulations without race conditions or threading overhead.
- **In-Memory Persistence:** Modifying patient acuity, booking appointments, or moving gurneys does not alter the seed CSV or TXT files. State is volatile and resets upon application termination.
- **CMake & CTest:** `CMakeLists.txt` is maintained and syntactically correct, but CMake was not invoked in this local environment; marked **UNVERIFIED**.
- **Alternative Compilers (Clang, MSVC, GCC 11+):** Only MinGW GCC 6.3.0 is installed on this host; marked **UNVERIFIED**.
- **Automated Memory Diagnostics (Valgrind, ASan):** Valgrind is unavailable on native Windows, and ASan is unsupported in GCC 6.3.0; marked **UNVERIFIED**.
