# Smart Hospital Emergency Triage & Route Dispatch System
## Comprehensive Project Specification & Implementation Plan (Final Revised)
**Course:** CS2305 — Data Structures-I, B.Tech Computer Engineering, AY 2026–27  
**Technology Stack:** C++17, CMake, Console/Terminal (Single-Threaded)  
**Document Status:** Final Architecture & Verification Plan  

---

## 1. Problem Statement and Objectives

### 1.1 Real-World Motivation
In hospital emergency departments (ED) and high-acuity medical campuses, operational efficiency directly affects patient outcomes across two critical areas:
1. **Dynamic Emergency Triage & Treatment Sequencing:** Patients arrive unpredictably with varying degrees of medical urgency. Clinical conditions can change dynamically while waiting (condition deterioration or stabilization). Patients must be prioritized strictly by urgency first, with arrival sequence breaking ties (FIFO within the same urgency tier). Routine outpatient consultations run in parallel on a predictable, non-emergency schedule.
2. **Campus Navigation & Rapid Route Dispatch:** Modern hospital campuses span multiple wings, diagnostic suites (Radiology/CT), intensive care units (ICU), operating theatres (OT), and ambulance reception bays. Corridors and transfer paths undergo scheduled maintenance, sterilization, or emergency closures. Transfer gurneys and critical transport teams must be routed along minimum-time paths while avoiding blocked corridors. In addition, campus utility planners periodically solve network connectivity problems (e.g., establishing a baseline interconnect or emergency communication backbone) to connect all wings at minimum total transit-time weight.

### 1.2 Academic Objectives
This project synthesizes two standard syllabus project themes:
- *Theme A: Hospital Emergency Patient Management* (Registration, urgency triage, appointment scheduling, treatment lifecycle, discharge).
- *Theme B: Maze Exploration & Rescue Planning* (Campus map representation, accessible exploration, blocked path handling, point-to-point shortest routing, and network spanning).

The objective is to design and implement an end-to-end, single-threaded educational simulation in C++17 where:
- Every central data structure (Hash Tables, Binary Search Tree, Priority Min-Heap, Linked Lists, Stack, Queue, Graph Adjacency List & Matrix, Disjoint Set Union) and algorithm (Dijkstra, BFS, DFS, Prim, Kruskal, Merge Sort, Quick Sort, Heapsort) is **implemented from scratch** rather than wrapped around STL container equivalents.
- Standard C++ STL utilities are confined strictly to foundational language primitives (`std::string`, `std::vector` for dynamic array storage, `std::unique_ptr` for memory ownership, `std::iostream` / `std::fstream` for I/O, `std::numeric_limits`, and `std::swap`).
- The system demonstrates concrete, verifiable alignment with every Course Outcome (CO1 through CO6) through reproducible interactive demonstrations and deterministic automated tests suitable for a rigorous second-year viva examination.

---

## 2. Scope, Boundaries, and Assumptions

### 2.1 In-Scope Capabilities
- **Patient Management & Lookup:** Unique alphanumeric patient IDs (`P101`, `P102`, ...); expected constant-time lookup by ID via a custom hash table with separate chaining; search and range queries via a custom Binary Search Tree (BST) indexed by ID.
- **Acuity Triage:** Synthetic urgency levels from 1 (Highest Priority / Immediate) to 5 (Lowest Priority / Routine). Managed via an array-based binary min-heap where Urgency Level is the primary key and monotonic arrival sequence is the secondary tie-breaker.
- **Dynamic Urgency Update:** Condition updates while waiting preserve the original arrival sequence and adjust position via heap sift-up or sift-down without stale or duplicated entries.
- **Outpatient Appointments:** Non-emergency clinic visits enqueued, dequeued, and cancelled through an explicit custom FIFO queue.
- **Complete Patient Lifecycle & Audit Trail:** Chronological patient events (arrival, triage, appointment, dispatch assignment, pickup, transport, handover, treatment, discharge) tracked in a custom doubly linked list with immutable event snapshot records.
- **Campus Route Planning & Exploration:** Campus represented as a weighted undirected graph with synchronized Adjacency List and Adjacency Matrix models. Support for corridor blocking/reopening, BFS reachable wing exploration, DFS cycle detection, and Dijkstra’s shortest path calculation with custom stack-based route reconstruction.
- **Safe Distance Arithmetic:** All route distances and totals use `int64_t` with guard values based on `std::numeric_limits<int64_t>::max() / 4` (`kInfinity`), validated non-negative edge weights ($w \ge 0$), and guarded addition.
- **Campus Network Infrastructure (MST):** Both Prim's and Kruskal's (with custom Disjoint Set Union) algorithms implemented to demonstrate minimal transit-time spanning networks on the campus graph.
- **Snapshots & Sorting Comparison:** Generation of patient reports sorted by Age, Wait Time, or Bill Amount using custom recursive Merge Sort, recursive Quick Sort, and Heapsort with step-by-step visual trace options.
- **Educational Demonstrations:** Explicit comparison modules (Hash Table Chaining vs. Linear Probing with tombstones, BST Mirror Creation vs. In-Place mutation, Dijkstra vs. MST, Recursive vs. Iterative BST traversals, Adjacency Matrix vs. List DFS).

### 2.2 Out-of-Scope (Explicit Non-Goals)
- Graphical User Interfaces (GUI), web frontends, or mobile apps (strictly ANSI console/terminal interface).
- External databases (SQLite, MySQL) or cloud backends (all state resides in-memory, with seed loading from structured CSV/text files).
- Real-time multi-threaded movement or asynchronous background timers (discrete simulation steps are used for resource transit).
- Live networking, HTTP sockets, or inter-process communication.
- Validated clinical triage diagnostic rules or clinical wait-time regulations.

### 2.3 Key Operational Assumptions
1. **Synthetic Urgency Scale Definition:** Lower numeric values denote higher clinical priority:
   - `1 - Resuscitation` (Immediate life threat — top triage priority).
   - `2 - Emergent` (High acuity — second priority).
   - `3 - Urgent` (Moderate acuity).
   - `4 - Less Urgent` (Low acuity).
   - `5 - Non-Urgent` (Routine outpatient consultation).
2. **Single Primary Entity Registry:** The `HospitalRegistry` owns the authoritative `Patient` records via `std::vector<std::unique_ptr<Patient>>`. All indexes and queues store stable non-owning pointers (`Patient*`), integer handles, or immutable `patientId` strings.
3. **Graph Non-Negativity:** All edge weights in the campus map represent non-negative physical corridor transit times in seconds ($w(u, v) \ge 0$), satisfying Dijkstra's algorithm prerequisites.
4. **Course Outcome Alignment Note:** The supplied course syllabus contains two differing course outcome (CO) listings. This plan adopts the standard six-unit curriculum structure corresponding to Units I through VI (CO1: Arrays/Recursion/Sorting, CO2: Lists/Hashing, CO3: Stacks/Queues, CO4: Trees, CO5: Heaps/Priority, CO6: Graphs/MST/Shortest Paths). This mapping is flagged for explicit confirmation with course faculty.

---

## 3. User Workflow, State Transitions, and Lifecycle Invariants

### 3.1 Patient Lifecycle State Machine
A patient moves through unambiguous, strictly enforced states. Invalid actions (e.g., duplicate resource assignment, treating a discharged patient, dispatching to an unreachable destination) are rejected with clear diagnostic messages.

```
                  [ Patient Arrival ]
                           │
                           ▼
                    ( REGISTERED )
                      │         │
               Emergency       Outpatient
               Triage          Clinic
                      │         │
                      ▼         ▼
             ( TRIAGED_WAITING )   ( APPOINTMENT_SCHEDULED )
               │         ▲                  │
        Select │ Dynamic │                  │ Called
           for │ Urgency └───┐              │ in FIFO Order
      Dispatch │ Update      │              │
               ▼             │              │
    ( DISPATCH_PENDING ) [Unassigned]       │
               │                            │
      Resource │ Candidate Selected         │
      Assigned │ & Leg 1 Validated          │
               ▼                            │
    ( DISPATCH_PENDING ) [Assigned]         │
               │                            │
        Pickup │ Resource Arrives           │
       Arrival │ at Patient Node            │
               ▼                            │
             ( IN_TRANSIT )                 │
               │                            │
      Arrival  │ Handover to Clinical Unit  │
            &  │ Resource Released to Idle  │
      Handover ▼                            ▼
             ( UNDER_TREATMENT ) ◄──────────┘
                     │
             Procedure Complete
                     │
                     ▼
               ( DISCHARGED )
                     │
            [ Snapshot Logged to ]
            [ Audit Trail (DLL)  ]
```

#### Detailed Patient State & Assignment Definitions:
1. `REGISTERED`: Master record created in registry; demographics, complaint, monotonic `arrivalSequence`, and simulated arrival time recorded.
2. `TRIAGED_WAITING`: Urgency level assigned (1–5); patient entry placed in the `TriageMinHeap`.
3. `APPOINTMENT_SCHEDULED`: Placed in the `AppointmentQueue` (FIFO) for non-emergency consultation.
4. `DISPATCH_PENDING (Unassigned)`: Patient extracted from triage heap as the next priority case; `assignedResourceId = ""`. Awaiting resource assignment and route validation.
5. `DISPATCH_PENDING (Assigned)`: Resource assigned (`assignedResourceId = resourceId`); resource is traveling to pickup location (`EN_ROUTE_PICKUP`). Patient is locked against duplicate assignment.
6. `IN_TRANSIT`: Assigned resource has arrived at pickup location, patient is loaded, and transit toward destination begins.
7. `UNDER_TREATMENT`: Patient has arrived at destination clinical unit, handover is completed, the transfer resource is released back to `AVAILABLE`, and medical treatment is actively underway.
8. `DISCHARGED`: Medical treatment finished; discharge summary recorded; patient marked discharged and archived in history.
9. `CANCELLED`: Appointment cancelled or patient withdrew prior to treatment; safely purged from active queues and any assigned resource is immediately released.

### 3.2 Dispatch Resource State Machine
Mobile transfer units (e.g., `GURNEY-01`, `GURNEY-02`) follow an independent lifecycle:

```
        ( AVAILABLE ) ◄───────────────────────────┐
              │                                   │
       Assign to Patient                          │ Handover Complete at
       & Begin Pickup Leg                         │ Destination Unit;
              │                                   │ Resource Released
              ▼                                   │
      ( EN_ROUTE_PICKUP )                         │
              │                                   │
       Arrive at Patient Pickup Point             │
       & Begin Patient Transport                  │
              │                                   │
              ▼                                   │
      ( TRANSPORTING_PATIENT ) ───────────────────┘
```

#### Detailed Resource State Definitions:
1. `AVAILABLE`: Stationed at a campus node (`currentNodeId`), idle and eligible for assignment (`assignedPatientId = ""`).
2. `EN_ROUTE_PICKUP`: Traveling from current station to patient's pickup location (`assignedPatientId = patientId`).
3. `TRANSPORTING_PATIENT`: Patient aboard; traveling from pickup location to target clinical unit (`assignedPatientId = patientId`).

### 3.3 State Transition Matrix

| Current State | Event / Action | Next State | Resource Action | Invariant / Validation Rule |
|---|---|---|---|---|
| `None` | Patient Register | `REGISTERED` | None | Monotonic `arrivalSequence` assigned; record added to master store. |
| `REGISTERED` | Assign Urgency | `TRIAGED_WAITING` | None | Inserted into `TriageMinHeap` with key `(urgency, arrivalSequence)`. |
| `REGISTERED` | Book Appointment| `APPOINTMENT_SCHEDULED`| None | Enqueued into custom FIFO `AppointmentQueue`. |
| `TRIAGED_WAITING` | Update Urgency | `TRIAGED_WAITING` | None | `arrivalSequence` **remains unchanged**; position updated via sift-up/down. |
| `TRIAGED_WAITING` | Extract Next | `DISPATCH_PENDING` (Unassigned) | None | Extracted from min-heap root; `assignedResourceId = ""` (unassigned). |
| `TRIAGED_WAITING` | Cancel | `CANCELLED` | None | Arbitrary removal from heap; position handle cleared. |
| `APPOINTMENT_SCHEDULED` | Cancel Appt | `CANCELLED` | None | Node unlinked from `AppointmentQueue`. |
| `APPOINTMENT_SCHEDULED` | Call Appt Turn | `UNDER_TREATMENT` | None | Dequeued from `AppointmentQueue`; consultation starts at clinic. |
| `DISPATCH_PENDING` (Unassigned) | Assign Resource Success | `DISPATCH_PENDING` (Assigned) | Resource $\to$ `EN_ROUTE_PICKUP` | Validates Leg 1 (pickup) and Leg 2 (transport). Selected by min pickup time. Patient locks `assignedResourceId`. |
| `DISPATCH_PENDING` (Unassigned) | Assign Resource Failed | `DISPATCH_PENDING` (Unassigned) | None | No resource available or leg unreachable; state preserved without changes. |
| `DISPATCH_PENDING` (Assigned) | Re-assign Attempt | `DISPATCH_PENDING` (Assigned) | None | **Rejected:** Duplicate assignment prevented. |
| `DISPATCH_PENDING` (Assigned) | Cancel Patient | `CANCELLED` | Resource $\to$ `AVAILABLE` | Resource released immediately at its current node; `assignedPatientId` cleared. |
| `DISPATCH_PENDING` (Assigned) | Pickup Arrival | `IN_TRANSIT` | Resource $\to$ `TRANSPORTING_PATIENT` | Resource reaches patient node; patient loaded aboard. |
| `IN_TRANSIT` | Handover at Dest | `UNDER_TREATMENT` | Resource $\to$ `AVAILABLE` | Patient reaches destination. **Resource released immediately**; `currentNodeId` set to destination. |
| `UNDER_TREATMENT` | Complete & Exit | `DISCHARGED` | **None** | Patient discharged. **Does not touch any resource** (freed at handover). |
| `DISCHARGED` | Any action | `DISCHARGED` | None | Terminal state. Repeated discharge or re-triage rejected with descriptive error. |
| `CANCELLED` | Any action | `CANCELLED` | None | Terminal state. Record archived. |

---

## 4. Complete Dispatch Behavior Specification

A hospital transfer dispatch is a two-leg operation simulated in discrete operational steps:

```
[ Resource Station ] ─── Leg 1: Pickup Transit ───► [ Patient Location ] ─── Leg 2: Transport Transit ───► [ Destination Unit ]
```

### 4.1 Dispatch Pre-Condition Validation
Before assigning any resource or altering patient state:
1. **Target Route Feasibility (Leg 2):** Run Dijkstra from `patient.currentLocationNode` to `patient.targetLocationNode`. If cost is `kInfinity` (unreachable due to corridor blocks), abort dispatch. Report: `"Destination unreachable due to corridor closure."`
2. **Resource Availability & Pickup Feasibility (Leg 1):** Iterate through all resources where `resource.state == AVAILABLE`:
   - Compute shortest route from `resource.currentNodeId` to `patient.currentLocationNode`.
   - Discard resources with unreachable pickup paths ($cost = \text{kInfinity}$).
3. **Deterministic Resource Selection Rule:**
   - From all candidate available resources with a valid pickup path, select the resource minimizing `pickupTravelTime`.
   - **Tie-Breaker:** If two or more resources have identical pickup travel times, select the one with the lexicographically smallest `resourceId` (e.g., `GURNEY-01` before `GURNEY-02`).
4. **All-or-Nothing Commit:** If no candidate resource exists, commit nothing. Patient remains safely in `DISPATCH_PENDING` (Unassigned). No partial updates occur.

### 4.2 Corridor Closure During Active Simulation Legs
Because the simulation executes in discrete steps rather than real-time threads:
1. **Closure During Leg 1 (Pickup Leg):**
   - Resource attempts to recalculate route to patient from its current node.
   - If unreachable: dispatch is aborted. The resource is released to `AVAILABLE` at its current node (`assignedPatientId = ""`), and the patient reverts to `DISPATCH_PENDING` (Unassigned).
2. **Closure During Leg 2 (Transport Leg):**
   - Resource and patient are paused at their current intermediate node.
   - Recalculate shortest route from current intermediate node to `patient.targetLocationNode`.
   - If unreachable: transfer halts at the intermediate node in a `PAUSED / HOLD` state. The resource remains coupled with the patient (`assignedPatientId = patientId`, `assignedResourceId = resourceId`) to safeguard patient custody until the corridor is reopened or manual transfer is initiated.

### 4.3 Handover and Resource Release Guarantee
- When Leg 2 completes, the patient arrives at `patient.targetLocationNode`.
- Handover occurs:
  1. `patient.state = PatientState::UNDER_TREATMENT`
  2. `patient.currentLocationNode = patient.targetLocationNode`
  3. `patient.assignedResourceId = ""`
  4. `resource.currentNodeId = patient.targetLocationNode`
  5. `resource.assignedPatientId = ""`
  6. `resource.state = ResourceState::AVAILABLE`
- **Critical Isolation Rule:** Because the resource is released at handover, the subsequent discharge of the patient (`UNDER_TREATMENT` $\to$ `DISCHARGED`) does not interact with any resource. This prevents corrupting the state of a resource that may have been reassigned to another patient.

---

## 5. Course-Outcome (CO) Coverage Matrix

| Course Outcome | Required Syllabus Coverage | Concrete Implementation in This Project | Demonstrable Viva Feature |
|---|---|---|---|
| **CO1** | Arrays, Recursion, Sorting | Dynamic record snapshot arrays; recursive Merge Sort; recursive Quick Sort; step-by-step trace mode; in-place standalone Heapsort. | Snapshot sorting menu sorting patient records by age and bill amount; side-by-side comparison of Quick Sort worst-case recursion space ($O(n)$) vs Merge Sort consistency. |
| **CO2** | Linked Lists and Hashing | Custom Doubly Linked List for patient audit trails; Custom Hash Table with separate chaining; separate Linear Probing demonstration with tombstones. | Display bucket chain distribution; insert colliding patient IDs (`hash(id) % M` identical); delete keys and demonstrate chain relinking and tombstone handling. |
| **CO3** | Stacks and Queues | Custom Node-based Stack; Custom Node-based FIFO Queue; overflow/underflow protections. | Custom Stack used for Dijkstra shortest path reversal and iterative DFS; Custom Queue used for outpatient appointment scheduling and BFS level traversal. |
| **CO4** | Binary Trees and BSTs | Custom BST indexed by ID with insert, search, delete (0, 1, 2 children); recursive & stack-based traversals; height/leaf display; mirror tree generation. | Visual step-by-step deletion of a two-child BST node with inorder successor; display mirror tree side-by-side with original BST to show reversed traversal. |
| **CO5** | Heaps and Priority Applications | Custom array-based Binary Min-Heap; composite comparator `(urgency, arrivalSequence)`; dynamic priority update; in-place Heapsort. | Dequeue equal-urgency patients in strict FIFO sequence; update waiting patient urgency preserving arrival order; update root element; run in-place Heapsort. |
| **CO6** | Graph Traversal, MST, Shortest Paths | Synchronized Adjacency List & Matrix representations; BFS accessible exploration; DFS cycle detection; Dijkstra shortest route; Prim's & Kruskal's MST (with DSU). | Block corridor (1, 5); recalculate Dijkstra to show alternate path (cost increases from 210s to 270s); run Prim and Kruskal on campus map and confirm identical minimum weight (1280s). |

---

## 6. Complete Syllabus Topic Classification

| Syllabus Topic | Project Classification | Pedagogical Justification & Role in Project |
|---|---|---|
| **Arrays & Record Vectors** | **Core Application** | Storage buffer for patient snapshot extraction, sorting arrays, and heap backing storage. |
| **Recursion** | **Core Application** | Employed in Merge Sort, Quick Sort, BST traversals, tree height/mirroring, and graph DFS. |
| **Singly Linked Lists** | **Core Application** | Underpins the custom FIFO appointment queue and hash table collision buckets. |
| **Doubly Linked Lists** | **Core Application** | Powers the bidirectional chronological patient audit trail and event history log. |
| **Circular Linked Lists** | **Optional Extension** | Round-robin emergency physician on-call roster rotation demo. |
| **Custom Hash Table (Chaining)** | **Core Application** | Primary patient ID index providing expected $O(1)$ medical record lookups. |
| **Hash Table (Linear Probing)** | **Supporting Demonstration** | Educational benchmark module comparing open addressing with tombstones vs. chaining. |
| **Custom Stack (Linked List)** | **Core Application** | Used for Dijkstra shortest path reconstruction and iterative BST traversals. |
| **Custom Queue (Linked List)** | **Core Application** | Powers outpatient appointment scheduling and BFS campus wing exploration. |
| **Binary Search Tree (BST)** | **Core Application** | Primary ID index supporting insertion, multi-case deletion, and full traversals. |
| **BST Mirroring & Metrics** | **Supporting Demonstration** | Demonstrates tree transformations and structural inspection on an independent clone copy. |
| **Binary Min-Heap with Key Update** | **Core Application** | Core engine of emergency triage prioritizing urgency with arrival-order tie-breaking. |
| **Heapsort** | **Core Application** | Implemented as a standalone in-place sorting engine ($O(1)$ aux space). |
| **Graph Adjacency List** | **Core Application** | Scalable representation of the simulated campus map for Dijkstra, BFS, and Kruskal. |
| **Graph Adjacency Matrix** | **Supporting Demonstration** | Synchronized dual representation used for matrix DFS and cycle verification. |
| **Dijkstra’s Shortest Path** | **Core Application** | Computes fastest response routes for emergency transfer units with obstacle rerouting. |
| **Prim’s & Kruskal’s MST (with DSU)**| **Core Application** | Used for campus utility network planning (minimum transit-time spanning tree). |
| **Fast / Slow Pointer Techniques** | **Supporting Demonstration** | Cycle detection helper demonstrated on linked-list history chains. |
| **Infix to Postfix & Evaluation** | **Optional Extension** | Generic stack-based arithmetic expression parser and evaluator. |
| **Sparse Matrix & Fast Transpose** | **Optional Extension** | Representation of large hospital bed occupancy matrices with fast transpose demo. |
| **Double-Ended Priority Queue (DEPQ)** | **Theory / Viva Coverage** | Viva topic: simultaneous min/max retrieval (highest vs. lowest acuity patient monitoring). |
| **Leftist / Binomial / Fibonacci Heaps**| **Theory / Viva Coverage** | Theoretical comparison: amortized $O(1)$ decrease-key vs. standard binary heap $O(\log n)$. |
| **Preemptive vs. Nonpreemptive Scheduling**| **Theory / Viva Coverage** | Analysis of emergency triage (preemptive priority insertion) vs. outpatient clinic (nonpreemptive FIFO). |
| **Josephus Problem** | **Theory / Viva Coverage** | Theoretical application of circular lists in resource elimination algorithms. |

---

## 7. System Architecture & Modular Folder Structure

```
SmartHospitalSystem/
├── CMakeLists.txt                  # Portable CMake configuration (C++17, MSVC & GCC/Clang warnings)
├── README.md                       # Project overview, build instructions, viva guide
├── plan.md                         # This comprehensive design document
├── include/
│   ├── models/
│   │   ├── Patient.hpp             # Patient model, UrgencyLevel enum, PatientState enum
│   │   ├── DispatchResource.hpp    # Gurney model, ResourceState enum, location
│   │   ├── CampusLocation.hpp      # Campus node definition (ID, name, type)
│   │   └── AuditEvent.hpp          # Immutable snapshot record for chronological history
│   ├── ds/
│   │   ├── SinglyLinkedList.hpp    # Custom singly linked list template
│   │   ├── DoublyLinkedList.hpp    # Custom doubly linked list template
│   │   ├── HashTableChaining.hpp   # Custom hash table with separate chaining
│   │   ├── HashTableProbing.hpp    # Custom hash table with linear probing (demo)
│   │   ├── CustomStack.hpp         # Node-based LIFO stack
│   │   ├── CustomQueue.hpp         # Node-based FIFO queue
│   │   ├── BinarySearchTree.hpp    # Custom BST with 3-case deletion & traversals
│   │   ├── BinaryMinHeap.hpp       # Generic binary min-heap supporting triage & routing
│   │   ├── GraphAdjList.hpp        # Adjacency list graph with blocked edge support
│   │   ├── GraphAdjMatrix.hpp      # Synchronized adjacency matrix graph for DFS/cycle demo
│   │   └── DisjointSetUnion.hpp    # DSU with union by rank and path compression
│   ├── algorithms/
│   │   ├── SortingAlgorithms.hpp   # Merge Sort, Quick Sort, in-place Heapsort with traces
│   │   ├── DijkstraRouting.hpp     # Shortest path calculation with int64_t guarded arithmetic
│   │   ├── GraphTraversals.hpp     # BFS (reachable wings) and DFS (cycle detection)
│   │   └── MinimumSpanningTree.hpp # Prim's and Kruskal's MST algorithms
│   ├── services/
│   │   ├── HospitalRegistry.hpp    # Master storage owning Patient unique_ptrs
│   │   ├── TriageManager.hpp       # Emergency priority heap orchestration with handle mapping
│   │   ├── AppointmentManager.hpp  # Outpatient FIFO queue orchestration
│   │   ├── CampusMapManager.hpp    # Synchronized graph management, corridor blocking, MST
│   │   ├── DispatchManager.hpp     # Two-leg dispatch routing, resource allocation, handover
│   │   └── HospitalSystem.hpp      # Master facade binding all services
│   ├── ui/
│   │   ├── ConsoleUI.hpp           # Interactive ANSI menu system and screen router
│   │   └── UIHelpers.hpp           # Formatting, table rendering, input sanitization
│   └── demos/
│       ├── AlgorithmDemos.hpp      # Standalone educational demonstrations
│       └── BenchmarkRunner.hpp     # Performance and collision comparative runners
├── src/
│   ├── main.cpp                    # Application entry point
│   ├── models/
│   │   ├── Patient.cpp
│   │   ├── DispatchResource.cpp
│   │   └── CampusLocation.cpp
│   ├── services/
│   │   ├── HospitalRegistry.cpp
│   │   ├── TriageManager.cpp
│   │   ├── AppointmentManager.cpp
│   │   ├── CampusMapManager.cpp
│   │   ├── DispatchManager.cpp
│   │   └── HospitalSystem.cpp
│   ├── ui/
│   │   ├── ConsoleUI.cpp
│   │   └── UIHelpers.cpp
│   └── demos/
│       └── AlgorithmDemos.cpp
├── data/
│   ├── patients_seed.csv           # Deterministic synthetic patient dataset (12 records)
│   ├── resources_seed.csv          # Deterministic synthetic dispatch resources (3 units)
│   └── campus_map_seed.txt         # Pre-configured 12-node, 15-edge campus graph topology
└── tests/
    ├── TestRunner.cpp              # Custom test runner registered with CTest
    ├── TestLinearDS.cpp            # Tests for Lists, Stacks, Queues, Hash Tables
    ├── TestTreeAndHeap.cpp         # Tests for BST deletion, traversals, Min-Heap
    └── TestGraphAndSorting.cpp     # Tests for Dijkstra, MST, Quick/Merge/Heapsort
```

### 7.1 Portable Compiler Warnings (CMake)
```cmake
cmake_minimum_required(VERSION 3.14)
project(SmartHospitalSystem VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wconversion)
endif()
```

---

## 8. Data Models & Master Ownership Strategy

### 8.1 Authoritative Memory Ownership Pattern
```
 ┌───────────────────────────────────────────────────────────────┐
 │               HospitalRegistry (Master Storage)               │
 │  std::vector<std::unique_ptr<Patient>> patientStore;         │
 │  Stores smart pointers contiguously; Patient objects on heap  │
 └───────────────────────────────┬───────────────────────────────┘
                                 │
              Borrows stable non-owning pointer (Patient*)
              or stable key (PatientID) / handle (int)
                                 │
         ┌───────────────────────┼───────────────────────┐
         ▼                       ▼                       ▼
 ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
 │ HashTable     │       │ BinarySearch  │       │ TriageMinHeap │
 │ (ID -> Handle)│       │ Tree (ID Key) │       │ (HeapNode)    │
 └───────────────┘       └───────────────┘       └───────┬───────┘
         │                       │                       │
         ▼                       ▼                       ▼
 ┌───────────────┐       ┌───────────────┐       ┌───────────────┐
 │ Appointment   │       │ PatientAudit  │       │ Position Array│
 │ Queue (FIFO)  │       │ Trail (DLL)   │       │ (Handle->Pos) │
 └───────────────┘       └───────────────┘       └───────────────┘
```

1. **Lifetime Guarantee:** `HospitalRegistry` owns every `Patient` via `std::unique_ptr<Patient>`. Records remain allocated for the lifetime of the application so historical queries and audit trails remain permanently valid without dangling pointers.
2. **Dense Stable Handles:** Every registered patient receives a zero-indexed integer `internalHandle` ($0 \le \text{handle} < N$). The heap maintains a direct position array `heapPosByHandle[handle]` mapping handles directly to heap array indices.
3. **Audit Trail Immutability:** When state transitions occur, `AuditEvent` records copy historical values into an independent struct so modifications to current patient records do not mutate history.

### 8.2 Domain Entity Models

```cpp
enum class UrgencyLevel : int {
    RESUSCITATION = 1, // Highest urgency
    EMERGENT      = 2,
    URGENT        = 3,
    LESS_URGENT   = 4,
    NON_URGENT    = 5  // Lowest urgency
};

enum class PatientState {
    REGISTERED,
    TRIAGED_WAITING,
    APPOINTMENT_SCHEDULED,
    DISPATCH_PENDING,
    IN_TRANSIT,
    UNDER_TREATMENT,
    DISCHARGED,
    CANCELLED
};

struct Patient {
    int internalHandle;             // Stable dense integer handle (0 to N-1)
    std::string patientId;          // e.g., "P101"
    std::string fullName;           // e.g., "Sarah Connor"
    int age;                        // e.g., 29
    std::string complaint;          // e.g., "Suspected Arm Fracture"
    UrgencyLevel urgency;           // 1 to 5
    uint64_t arrivalSequence;       // Monotonic arrival order (1, 2, 3...) for FIFO tie-breaking
    int simulatedArrivalTime;       // Simulation clock ticks / minutes for wait-time reporting
    PatientState state;             // Current lifecycle state
    std::string assignedResourceId; // Non-empty if dispatch assigned; empty if unassigned
    int currentLocationNode;        // Current campus map node ID (0 to 11)
    int targetLocationNode;         // Target treatment wing node ID (0 to 11)
    double billingAmount;           // Synthetic billing metric for sorting reports
};

struct DispatchResource {
    std::string resourceId;         // e.g., "GURNEY-01"
    std::string name;               // e.g., "Trauma Transfer Gurney"
    ResourceState state;            // AVAILABLE, EN_ROUTE_PICKUP, TRANSPORTING_PATIENT
    int currentNodeId;              // Current campus map node ID
    std::string assignedPatientId;  // Empty string if idle
};

struct AuditEvent {
    std::string patientId;
    std::string action;             // e.g., "DISPATCH_STARTED", "HANDOVER_COMPLETED"
    PatientState priorState;
    PatientState newState;
    UrgencyLevel urgencyAtEvent;
    int locationAtEvent;
    int timestamp;                  // Simulation clock timestamp
};
```

---

## 9. Custom Data Structures & Operations Rationale

### 9.1 Custom Hash Table with Separate Chaining (`HashTableChaining<K, V>`)
- **Structure:** Array of bucket head pointers, where each bucket is a singly linked list of key-value nodes.
- **Hash Function:** Polynomial rolling hash for string keys:
  $$H(s) = \left( \sum_{i=0}^{L-1} s[i] \cdot p^i \right) \pmod M$$
  with prime base $p = 31$ and prime capacity $M$ (initial capacity $M = 17$). Key length $L$ is bounded ($L \le 16$, so hash computation is $O(1)$).
- **Operations:**
  - `insert(key, value)`: Inserts or updates node; triggers `rehash()` when load factor $\alpha = n / M > 0.75$.
  - `rehash()`: To guarantee load factor drops safely below the threshold, new capacity is chosen as the **next prime greater than or equal to $2M + 1$**.
  - `search(key)`: Returns pointer to value or `nullptr` in $\Theta(1)$ average time, $O(n)$ worst-case under pathological collisions.
  - `remove(key)`: Unlinks node from bucket list, frees node memory in $\Theta(1)$ average time, $O(n)$ worst-case.
- **Academic Discussion:** The documentation explicitly notes that separate chaining is subject to worst-case degradation if all keys hash to the same bucket. Resizing to the next prime $\ge 2M + 1$ restores $\alpha \le 0.75$.

### 9.2 Custom Binary Search Tree (`BinarySearchTree<K, V>`)
- **Primary Operational Key:** Indexed by unique `patientId` string for reliable, collision-free ID queries and ID range traversals. Demographic queries (e.g., age-based sorting) are performed via snapshot sorting (Merge/Quick sort) or linear scans.
- **Operations:**
  - `insert(key, value)`: Standard BST insertion ($\Theta(\log n)$ average, $O(n)$ worst case).
  - `search(key)`: Returns pointer to patient record or `nullptr`.
  - `deleteNode(key)`: Fully implements 0-child, 1-child, and 2-children deletion with in-order successor swapping. Child pointers are updated cleanly.
  - `traversalInorderRecursive()`, `traversalPreorder()`, `traversalPostorder()`.
  - `traversalInorderIterative()`: Stack-driven non-recursive traversal demonstrating explicit stack mechanics.
  - `traversalLevelOrder()`: Queue-driven breadth traversal.
  - `getHeight()`, `countLeaves()`, `getLeafNodes()`.
  - `createMirrorTree()`: Returns an **independent cloned copy** of the tree with recursively inverted child pointers, leaving the operational BST unaltered.
- **Degradation Note:** Presorted input produces an unbalanced chain with $O(n)$ search depth.

### 9.3 Custom Binary Min-Heap (`BinaryMinHeap<T, Comparator>`)
- **Structure:** Array-backed complete binary tree storing triage records `(internalHandle, patientId, urgency, arrivalSequence)`.
- **Comparator:**
  $$\text{Node } A \text{ precedes } B \iff (A.\text{urgency} < B.\text{urgency}) \lor (A.\text{urgency} == B.\text{urgency} \land A.\text{arrivalSequence} < B.\text{arrivalSequence})$$
- **Position Tracking via Direct Handle Array:**
  - `std::vector<int> heapPosByHandle`: Directly maps each patient's `internalHandle` to their current array index in the heap.
  - Position updates are executed on **every swap, insertion, extraction, and arbitrary removal**.
  - **Complexity Separation:**
    - Given a known internal handle, heap priority update is **strict $O(\log n)$ worst case**.
    - If starting from an external `patientId` string, resolving the handle via the hash table takes $\Theta(1)$ average and $O(n)$ worst case (collision bound).
- **Generic Heap Isolation:** Routing algorithms (Dijkstra, Prim) instantiate a clean `BinaryMinHeap<DijkstraNode>` without position tracking overhead.

### 9.4 Graph Representations & Dijkstra Heap Strategy
- **Synchronized Adjacency List & Matrix:**
  - `GraphAdjList`: Vector of linked lists of `GraphEdge` structures. Primary structure for routing and traversals.
  - `GraphAdjMatrix`: $12 \times 12$ matrix synchronized with the list. Absent edges are marked with sentinel `NO_EDGE = -1`. Blocked corridors are marked with `BLOCKED = -2`. Corridors are bidirectional ($u \leftrightarrow v$), so both directions are updated together.
- **Dijkstra / Prim Heap Strategy:**
  - Employs a **duplicate-entry min-heap with stale-entry checks**:
    - When relaxing edge $(u, v, w)$, if $dist[u] + w < dist[v]$, update $dist[v] = dist[u] + w$ and insert `{v, dist[v]}` into the heap.
    - When extracting `{u, d}`, if $d > dist[u]$, discard as a stale entry and continue.
  - **Safe Distance Arithmetic:** Uses `int64_t` with sentinel `kInfinity = std::numeric_limits<int64_t>::max() / 4` and guarded addition (`if (dist[u] != kInfinity && weight >= 0 && dist[u] + weight < dist[v])`).
  - Zero-cost check: Source-equals-destination queries return cost 0 and path `[source]` immediately.

---

## 10. Algorithm Pseudocode

### 10.1 Min-Heap Priority Update Preserving Arrival Order
```
Algorithm: updateUrgency(handle, newUrgency)
Input: handle (int), newUrgency (1 to 5)

1. pos = heapPosByHandle[handle]
2. if pos < 0 or pos >= heapSize then return false
3. oldUrgency = heapArray[pos].urgency
4. if newUrgency == oldUrgency then return true
5. heapArray[pos].urgency = newUrgency
   // Critical: heapArray[pos].arrivalSequence remains UNCHANGED
6. if newUrgency < oldUrgency then
7.     // Acuity increased -> sift up towards root (only if pos > 0)
8.     if pos > 0 then siftUp(pos)
9. else
10.    // Acuity decreased -> sink towards leaves
11.    siftDown(pos)
12. return true

Algorithm: siftUp(pos)
1. while pos > 0 do
2.     parentIdx = (pos - 1) / 2
3.     if precedes(heapArray[pos], heapArray[parentIdx]) then
4.         swapElements(pos, parentIdx)
5.         pos = parentIdx
6.     else
7.         break

Algorithm: swapElements(i, j)
1. swap(heapArray[i], heapArray[j])
2. heapPosByHandle[heapArray[i].handle] = i
3. heapPosByHandle[heapArray[j].handle] = j
```

### 10.2 Binary Search Tree: Three-Case Node Deletion
```
Algorithm: deleteBSTNode(root, targetId)
Input: Pointer to root node, targetId
Output: Pointer to new subtree root

1. if root == NULL then return NULL
2. if targetId < root.key then
3.     root.left = deleteBSTNode(root.left, targetId)
4. else if targetId > root.key then
5.     root.right = deleteBSTNode(root.right, targetId)
6. else
7.     // Case 1: Leaf node (0 children)
8.     if root.left == NULL and root.right == NULL then
9.         delete root
10.        return NULL
11.    // Case 2: Exactly 1 child
12.    else if root.left == NULL then
13.        temp = root.right
14.        delete root
15.        return temp
16.    else if root.right == NULL then
17.        temp = root.left
18.        delete root
19.        return temp
20.    // Case 3: Exactly 2 children
21.    else
22.        successor = findMinNode(root.right)
23.        root.key = successor.key
24.        root.patientData = successor.patientData
25.        root.right = deleteBSTNode(root.right, successor.key)
26. return root
```

### 10.3 Dijkstra’s Shortest Route with Guarded Arithmetic
```
Algorithm: computeShortestRoute(sourceNode, targetNode, graph)
Input: sourceNode, targetNode, campusGraph G = (V, E)
Output: Total transit cost and ordered list of nodes

1. if sourceNode == targetNode then return (cost = 0, path = [sourceNode])
2. for i = 0 to V - 1 do: dist[i] = kInfinity, parent[i] = -1, visited[i] = false
3. dist[sourceNode] = 0
4. minHeap.insert({sourceNode, cost = 0})
5. while not minHeap.isEmpty() do
6.     top = minHeap.extractMin()
7.     u = top.nodeId, d = top.cost
8.     if d > dist[u] then continue    // Discard stale entry
9.     visited[u] = true
10.    if u == targetNode then break  // Early exit on reaching target
11.    for each edge in graph.adjList[u] do
12.        if edge.isBlocked then continue  // Bypass closed corridors
13.        v = edge.destinationNode
14.        weight = edge.weight
15.        if weight >= 0 and dist[u] != kInfinity and dist[u] + weight < dist[v] then
16.            dist[v] = dist[u] + weight
17.            parent[v] = u
18.            minHeap.insert({v, dist[v]})
19. if dist[targetNode] >= kInfinity then
20.     return (cost = kInfinity, path = empty)  // Target unreachable
21. // Reconstruct path using custom Stack
22. CustomStack<int> pathStack
23. curr = targetNode
24. while curr != -1 do
25.     pathStack.push(curr)
26.     curr = parent[curr]
27. orderedPath = empty list
28. while not pathStack.isEmpty() do
29.     orderedPath.append(pathStack.pop())
30. return (dist[targetNode], orderedPath)
```

### 10.4 Kruskal’s Algorithm with Disjoint Set Union (MST)
```
Algorithm: kruskalMST(graph)
Input: Campus graph G = (V, E) with 15 undirected edges
Output: Total transit-time weight and list of MST edges

1. edgeList = empty list
2. for each u from 0 to V - 1 do
3.     for each edge in graph.adjList[u] do
4.         if not edge.isBlocked and u < edge.destinationNode then
5.             edgeList.append({u, edge.destinationNode, edge.weight})
6. sort edgeList ascending by weight  // using custom Merge Sort
7. dsu.initialize(V)
8. mstEdges = empty list, totalWeight = 0
9. for each edge (u, v, w) in edgeList do
10.    if dsu.find(u) != dsu.find(v) then
11.        dsu.unite(u, v)
12.        mstEdges.append(edge)
13.        totalWeight += w
14.        if length(mstEdges) == V - 1 then break
15. if length(mstEdges) < V - 1 then
16.     return (totalWeight, mstEdges, isDisconnected = true)
17. return (totalWeight, mstEdges, isDisconnected = false)
```

### 10.5 DFS with Parent Check for Undirected Cycle Detection
```
Algorithm: detectCycleDFS(u, parentNode, visited, graph)
Input: Current vertex u, parentNode, visited array, graph
Output: True if cycle exists, false otherwise

1. visited[u] = true
2. for each edge in graph.adjList[u] do
3.     if edge.isBlocked then continue
4.     v = edge.destinationNode
5.     if not visited[v] then
6.         if detectCycleDFS(v, u, visited, graph) then return true
7.     else if v != parentNode then
8.         return true  // Visited neighbor that is not DFS parent indicates a cycle
9. return false
```

---

## 11. Time & Space Complexity Master Table

| Component / Operation | Best Case | Average Case | Worst Case | Space Complexity | Practical Engineering & Viva Rationale |
|---|---|---|---|---|---|
| **Hash Table (Chaining)** — Search / Insert / Delete | $\Omega(1)$ | $\Theta(1)$ | $O(n)$ | $O(M + n)$ | Average case assumes uniform hashing and $\alpha \le 0.75$. Worst case occurs under pathological collisions where all keys map to one chain. String key hashing is $O(L)$ with $L \le 16$. |
| **Hash Table (Linear Probing)** — Search / Insert | $\Omega(1)$ | $\Theta(1)$ | $O(n)$ | $O(M)$ | Suffers from primary clustering as load factor approaches 1.0; requires tombstone sentinels on deletion. |
| **Binary Search Tree** — Search / Insert / Delete | $\Omega(1)$ | $\Theta(\log n)$ | $O(n)$ | $O(n)$ | Average case assumes randomly ordered keys. Worst case $O(n)$ occurs on sorted input where the tree degenerates into a linear chain. |
| **BST Stack Inorder Traversal** | $\Omega(n)$ | $\Theta(n)$ | $O(n)$ | $O(h)$ aux | Traversal visits all $n$ nodes. Auxiliary stack space is proportional to tree height $h$, bounded by $O(\log n)$ average and $O(n)$ worst case. |
| **Binary Min-Heap** — Insert / Extract-Min | $\Omega(1)$ / $\Omega(\log n)$ | $\Theta(\log n)$ | $O(\log n)$ | $O(n)$ | Array-backed complete binary tree guaranteed balanced; root peek is strict $\Theta(1)$. |
| **Binary Min-Heap** — Urgency Update via Handle | $\Omega(1)$ | $\Theta(\log n)$ | $O(\log n)$ | $O(N_{\max})$ aux | When given internal handle, position lookup is $O(1)$ and heap sift is strict $O(\log n)$ worst case. |
| **Binary Min-Heap** — Urgency Update via String ID | $\Omega(1)$ | $\Theta(\log n)$ | $O(n)$ | $O(n)$ | Involves string ID to handle lookup (hash table $\Theta(1)$ avg, $O(n)$ worst) followed by heap sift-up/down ($O(\log n)$). |
| **Linked List Stack / Queue** — Push / Pop / Enqueue / Dequeue | $\Omega(1)$ | $\Theta(1)$ | $O(1)$ | $O(n)$ | Dynamic node allocation eliminates fixed buffer overflow. Underflow checks protect empty states. |
| **Dijkstra Shortest Route** (Duplicate-Entry Heap) | $\Omega(V)$ | $\Theta(V + E \log V)$ | $O(V + E \log V)$ | $O(V + E)$ aux | Distance array initialization takes $\Theta(V)$. Duplicate heap entries bounded by $E$. Can exit early if target vertex is reached. |
| **BFS Reachable Wing Exploration** | $\Omega(1)$ | $\Theta(V_{\text{reach}} + E_{\text{reach}})$ | $O(V + E)$ | $O(V)$ aux | Explores only the component reachable from the start node. Full graph traversal requires $O(V + E)$ time. |
| **DFS Full Traversal** — Adjacency List | $\Omega(1)$ | $\Theta(V_{\text{reach}} + E_{\text{reach}})$ | $O(V + E)$ | $O(V)$ aux | Inspects only existing edges incident to reachable vertices. |
| **DFS Full Traversal** — Adjacency Matrix | $\Omega(V^2)$ | $\Theta(V^2)$ | $O(V^2)$ | $O(V)$ aux | Must inspect all $V \times V$ matrix entries regardless of edge density. |
| **Kruskal MST with DSU** | $\Omega(E \log E)$ | $\Theta(E \log E)$ | $O(E \log E)$ | $O(V + E)$ | Dominated by sorting non-blocked edges; DSU operations run in near-linear $O(E \cdot \alpha(V))$ time. |
| **Prim MST** (Duplicate-Entry Binary Heap) | $\Omega(V \log V)$ | $\Theta(E \log V)$ | $O(E \log V)$ | $O(V + E)$ aux | Duplicate heap entries bounded by $E$. Note: for dense graphs ($E \approx V^2$), matrix-based Prim ($O(V^2)$) is asymptotically faster than binary heap Prim ($O(V^2 \log V)$). |
| **Merge Sort** — Snapshot Report | $\Omega(n \log n)$ | $\Theta(n \log n)$ | $O(n \log n)$ | $O(n)$ aux | Stable sorting; divides array unconditionally; requires temporary merge buffer. |
| **Quick Sort** — Snapshot Report | $\Omega(n \log n)$ | $\Theta(n \log n)$ | $O(n^2)$ | $O(n)$ stack | In-place partition. Worst case $O(n^2)$ time and $O(n)$ call-stack space occurs when partition is maximally unbalanced on presorted data. |
| **Heapsort** — In-Place Standalone Sort | $\Omega(n \log n)$ | $\Theta(n \log n)$ | $O(n \log n)$ | $O(1)$ aux | In-place array sorting directly on the buffer without copying into a separate heap container. Optimal worst-case bounds. |

---

## 12. Console User Interface & Interactive Menu Design

```
================================================================================
       SMART HOSPITAL EMERGENCY TRIAGE & ROUTE DISPATCH SYSTEM (CS2305)
================================================================================
  [1] Emergency Patient Triage & Queue Management
      ├── 1.1 Register New Emergency Patient
      ├── 1.2 View Active Triage Priority Queue (Min-Heap Display)
      ├── 1.3 Call / Dispatch Next Highest-Acuity Patient
      ├── 1.4 Update Patient Urgency Level (Preserving Arrival Order)
      ├── 1.5 Update Patient Treatment Status (Handover / Discharge)
      └── 1.6 Search Patient by ID (Expected O(1) Hash Table Lookup)

  [2] Outpatient Appointment Scheduling (FIFO Clinic)
      ├── 2.1 Book Non-Emergency Consultation
      ├── 2.2 View Scheduled Appointment Queue (FIFO Queue Display)
      ├── 2.3 Process Next Appointment in Turn
      └── 2.4 Cancel Scheduled Appointment

  [3] Campus Map, Corridor Obstacles & Vehicle Route Dispatch
      ├── 3.1 Display Campus Topology & Active Edge Status
      ├── 3.2 Block Campus Corridor (Simulate Maintenance/Leak)
      ├── 3.3 Reopen Campus Corridor
      ├── 3.4 Explore Reachable Wings from Location (BFS Exploration)
      ├── 3.5 Calculate Shortest Emergency Route (Dijkstra + Stack Reconstruct)
      ├── 3.6 Dispatch Available Transfer Gurney (Deterministic Two-Leg Routing)
      └── 3.7 Plan Campus Utility Network (Kruskal vs Prim MST Comparison)

  [4] Patient Medical Records Index & BST Explorer
      ├── 4.1 Display Inorder Traversal (Sorted Patient ID List)
      ├── 4.2 Display Preorder & Postorder Traversals
      ├── 4.3 Iterative Stack-Based Inorder Traversal (Educational Demo)
      ├── 4.4 Level-Order Traversal (Breadth First)
      ├── 4.5 Inspect Tree Metrics (Height, Total Count, Leaf Nodes)
      ├── 4.6 Delete Patient Record (Demonstrating 0, 1, and 2-Child Cases)
      └── 4.7 Generate & Display BST Mirror Tree (Independent Cloned Copy)

  [5] Sorting & Analytical Reporting Suite
      ├── 5.1 Sort Patient Snapshot by Age (Merge Sort with Trace)
      ├── 5.2 Sort Patient Snapshot by Billing Amount (Quick Sort with Trace)
      ├── 5.3 Sort Patient Snapshot via In-Place Standalone Heapsort
      └── 5.4 Sorting Algorithm Performance Benchmark & Complexity Comparison

  [6] Educational Laboratory Demonstrations
      ├── 6.1 Hash Table Collision: Separate Chaining vs Linear Probing (Tombstones)
      ├── 6.2 Adjacency Matrix vs Adjacency List DFS & Cycle Detection
      └── 6.3 MST vs Shortest Path: Conceptual Contrast Demonstration

  [7] System Diagnostics & Automated Self-Verification
      └── Run Full Battery of Deterministic Unit & Integration Tests

  [0] Exit Application
================================================================================
Select Option [0-7]: _
```

---

## 13. Authoritative Sample Dataset & Campus Map Topology

### 13.1 Campus Location Directory (12 Nodes)
The campus topology consists of exactly 12 named physical locations:
- **Node 0:** Ambulance Reception Bay
- **Node 1:** ER Triage & Main Waiting Hall
- **Node 2:** Emergency Resuscitation Bay
- **Node 3:** Surgical Operating Theatres (OT)
- **Node 4:** Intensive Care Unit (ICU)
- **Node 5:** Central Diagnostic Radiology & CT
- **Node 6:** Blood Bank & Pathology Lab
- **Node 7:** Central Inpatient Pharmacy
- **Node 8:** General Inpatient Wards
- **Node 9:** Rooftop Air Ambulance Heliport
- **Node 10:** Infectious Disease Isolation Wing
- **Node 11:** Campus Engineering & Utility Substation

### 13.2 Authoritative Corridor Edge List (15 Undirected Edges)
All corridors are undirected. Weights represent physical transit times in seconds:

| Edge Index | Node U | Node V | Description | Weight (Seconds) |
|---|---|---|---|---|
| E1 | 0 | 1 | Ambulance Bay $\leftrightarrow$ ER Triage | 60s |
| E2 | 0 | 2 | Ambulance Bay $\leftrightarrow$ Resuscitation Bay | 80s |
| E3 | 0 | 6 | Ambulance Bay $\leftrightarrow$ Blood Bank | 180s |
| E4 | 1 | 2 | ER Triage $\leftrightarrow$ Resuscitation Bay | 100s |
| E5 | 1 | 5 | ER Triage $\leftrightarrow$ Radiology / CT | 120s |
| E6 | 1 | 7 | ER Triage $\leftrightarrow$ Pharmacy | 140s |
| E7 | 2 | 4 | Resuscitation Bay $\leftrightarrow$ ICU | 90s |
| E8 | 2 | 9 | Resuscitation Bay $\leftrightarrow$ Heliport | 60s |
| E9 | 3 | 5 | Surgical OT $\leftrightarrow$ Radiology / CT | 90s |
| E10 | 3 | 7 | Surgical OT $\leftrightarrow$ Pharmacy | 130s |
| E11 | 3 | 8 | Surgical OT $\leftrightarrow$ General Wards | 160s |
| E12 | 4 | 5 | ICU $\leftrightarrow$ Radiology / CT | 150s |
| E13 | 6 | 7 | Blood Bank $\leftrightarrow$ Pharmacy | 110s |
| E14 | 8 | 10 | General Wards $\leftrightarrow$ Isolation Wing | 200s |
| E15 | 10 | 11 | Isolation Wing $\leftrightarrow$ Utility Substation | 180s |

```
                       [9] Heliport
                           │ (60s)
                           ▼
                 [2] Resuscitation Bay ──(90s)── [4] Intensive Care (ICU)
                     │          ▲                     │
               (80s) │          │ (100s)              │ (150s)
                     ▼          │                     ▼
 [0] Ambulance Bay ──(60s)──► [1] ER Triage ──(120s)──► [5] Central Radiology / CT
                     │          │                     │
              (180s) │          │ (140s)              │ (90s)
                     ▼          ▼                     ▼
              [6] Blood Bank ──(110s)──► [7] Pharmacy ──(130s)──► [3] Surgical OT
                                                           │
                                                           │ (160s)
                                                           ▼
                                                   [8] General Wards
                                                           │
                                                           │ (200s)
                                                           ▼
                                                [10] Isolation Wing
                                                           │ (180s)
                                                           ▼
                                                [11] Utility Substation
```

### 13.3 Independently Verified Graph Metrics

#### Shortest Path Routes (Dijkstra):
- **Node 0 $\to$ Node 3:** $0 \to 1 \to 5 \to 3$ with cost $60 + 120 + 90 = \mathbf{270s}$.
- **Node 0 $\to$ Node 3 with Edge (1, 5) Blocked:** $0 \to 1 \to 7 \to 3$ with cost $60 + 140 + 130 = \mathbf{330s}$.
- **Node 1 $\to$ Node 3:** $1 \to 5 \to 3$ with cost $120 + 90 = \mathbf{210s}$.
- **Node 1 $\to$ Node 3 with Edge (1, 5) Blocked:** $1 \to 7 \to 3$ with cost $140 + 130 = \mathbf{270s}$.
- **Node 1 $\to$ Node 4 (ICU):** $1 \to 2 \to 4$ with cost $100 + 90 = \mathbf{190s}$.
- **Node 0 $\to$ Node 4 (ICU):** $0 \to 2 \to 4$ with cost $80 + 90 = \mathbf{170s}$.
- **Node 2 $\to$ Node 9 (Heliport):** $2 \to 9$ with cost $\mathbf{60s}$.
- **Source Equals Destination (e.g., Node 1 $\to$ 1):** Path `[1]`, cost $\mathbf{0s}$.

#### Minimum Spanning Tree (MST) Exact Calculation:
For $V = 12$ vertices, an MST requires exactly $V - 1 = 11$ edges. Sorting all 15 edges and applying Kruskal's / Prim's algorithm yields:
1. $(0, 1): 60\text{s}$
2. $(2, 9): 60\text{s}$
3. $(0, 2): 80\text{s}$
4. $(2, 4): 90\text{s}$
5. $(3, 5): 90\text{s}$
6. $(6, 7): 110\text{s}$
7. $(1, 5): 120\text{s}$
8. $(3, 7): 130\text{s}$
9. $(3, 8): 160\text{s}$
10. $(10, 11): 180\text{s}$
11. $(8, 10): 200\text{s}$

Skipped cycle-forming edges: $(1, 2) [100\text{s}]$, $(1, 7) [140\text{s}]$, $(4, 5) [150\text{s}]$, $(0, 6) [180\text{s}]$.  
**Total Verified MST Weight:**
$$60 + 60 + 80 + 90 + 90 + 110 + 120 + 130 + 160 + 180 + 200 = \mathbf{1280\text{ seconds}}.$$
Both Kruskal's algorithm and Prim's algorithm deterministically produce total weight **1280 seconds**.

### 13.4 Authoritative Synthetic Patient Dataset (12 Patients)

| Patient ID | Full Name | Age | Complaint | Initial Urgency | Arrival Sequence | Sim Arrival Time | Current Node | Target Node | Bill Amount | Initial State |
|---|---|---|---|---|---|---|---|---|---|---|
| `P101` | Sarah Connor | 29 | Arm Fracture | 4 (Less Urgent) | 1 | 10 | 1 | 8 (Wards) | $450.00 | `TRIAGED_WAITING` |
| `P102` | Arthur Dent | 42 | Minor Laceration | 5 (Non-Urgent) | 2 | 15 | 1 | 1 (Clinic) | $120.00 | `APPOINTMENT_SCHEDULED` |
| `P103` | James Howlett | 55 | Appendicitis | 2 (Emergent) | 3 | 20 | 1 | 3 (Surgical OT) | $2100.00 | `TRIAGED_WAITING` |
| `P104` | Marcus Brody | 58 | STEMI Heart Attack | 1 (Resuscitation) | 4 | 22 | 1 | 2 (Resus Bay) | $3500.00 | `TRIAGED_WAITING` |
| `P105` | Bruce Wayne | 38 | Smoke Inhalation | 2 (Emergent) | 5 | 28 | 1 | 4 (ICU) | $1850.00 | `TRIAGED_WAITING` |
| `P106` | Diana Prince | 31 | Diabetic Review | 5 (Non-Urgent) | 6 | 35 | 1 | 1 (Clinic) | $95.00 | `APPOINTMENT_SCHEDULED` |
| `P107` | Peter Parker | 21 | Anaphylaxis | 1 (Resuscitation) | 7 | 40 | 1 | 2 (Resus Bay) | $800.00 | `TRIAGED_WAITING` |
| `P108` | Natasha Romanoff| 34| Puncture Wound | 3 (Urgent) | 8 | 44 | 1 | 3 (Surgical OT) | $1250.00 | `TRIAGED_WAITING` |
| `P109` | Clark Kent | 36 | Abdominal Trauma | 2 (Emergent) | 9 | 50 | 1 | 5 (Radiology/CT) | $1600.00 | `TRIAGED_WAITING` |
| `P110` | Ellen Ripley | 49 | Chemical Eye Splash | 3 (Urgent) | 10 | 55 | 1 | 1 (ER Triage) | $620.00 | `TRIAGED_WAITING` |
| `P111` | Walter White | 52 | Chronic Cough | 5 (Non-Urgent) | 11 | 60 | 1 | 1 (Clinic) | $110.00 | `APPOINTMENT_SCHEDULED` |
| `P112` | Barry Allen | 28 | Collision Trauma | 1 (Resuscitation) | 12 | 65 | 1 | 2 (Resus Bay) | $2900.00 | `TRIAGED_WAITING` |

#### Exact Urgency-1 Baseline Extraction Order:
Three patients arrive with Urgency 1:
- `P104` (Arrival Sequence 4)
- `P107` (Arrival Sequence 7)
- `P112` (Arrival Sequence 12)
Because arrival sequence breaks ties, extraction order from the triage heap is strictly: **`P104` $\to$ `P107` $\to$ `P112`**.

#### Verified Urgency Update Demonstration Scenario (P108):
- Patient `P108` arrives with **Urgency 3** and **Arrival Sequence 8**.
- Patient `P108` deteriorates and urgency is updated from 3 to **1**.
- Key becomes `(Urgency 1, Arrival Sequence 8)`.
- Precedence comparison among all Urgency 1 patients:
  - `P104`: $(1, 4)$ $\to$ $4 < 8$, so `P104` precedes `P108`.
  - `P107`: $(1, 7)$ $\to$ $7 < 8$, so `P107` precedes `P108`.
  - `P108`: $(1, 8)$ $\to$ $8 < 12$, so `P108` precedes `P112`.
  - `P112`: $(1, 12)$.
- **Exact Expected Triage Extraction Order:**
  **`P104` $\to$ `P107` $\to$ `P108` $\to$ `P112`**.
- Notice: `P108` sifts up past `P112` (since $8 < 12$), but does **not** jump ahead of earlier Urgency-1 arrivals `P104` and `P107`.

---

## 14. Verification & Automated Test Plan

The system includes a built-in automated test suite executable via menu option 7 or command-line flag `--test`. Runtime memory verification is enabled via AddressSanitizer (`-fsanitize=address,undefined` on GCC/Clang) and MSVC debug heap verification (`_CrtDumpMemoryLeaks()` on Windows).

### 14.1 Comprehensive Deterministic Test Battery (24 Test Cases)

| Test ID | Targeted Component & Scenario | Verification Procedure | Expected Outcome |
|---|---|---|---|
| **TC-01** | **Linear DS:** Empty / Single Element Underflow | Instantiate empty Stack, Queue, List; invoke `pop()`, `dequeue()`, `peek()`. | Safe underflow handling without crashes; returns `false` or `std::nullopt`. |
| **TC-02** | **Hash Table:** Collisions & Resizing | Insert 8 keys constructed with identical hash bucket index for initial capacity $M = 17$. | All keys preserved in bucket linked list; resizing to next prime $\ge 2M + 1$ restores $\alpha \le 0.75$; all keys retrievable. |
| **TC-03** | **Hash Table:** Key Deletion & Relinking | Insert keys `P101`, `P102`, `P103` into same bucket; delete middle key `P102`. | Search for `P102` returns `nullptr`; search for `P101` and `P103` succeeds; bucket list pointers intact. |
| **TC-04** | **Hash Table Probing:** Tombstones | In `HashTableProbing`, insert keys, delete an element, and search for subsequent keys in probe sequence. | Tombstone sentinel correctly preserves probe sequence; lookup succeeds. |
| **TC-05** | **Min-Heap:** Equal Urgency FIFO Order | Insert three patients: `(Urgency 1, Seq 4)`, `(Urgency 1, Seq 7)`, `(Urgency 1, Seq 12)`. | Consecutive `extractMin()` calls return sequence 4, then 7, then 12. |
| **TC-06** | **Min-Heap:** Dynamic Acuity Update & Precedence | Complete fixture with `P104 (1, 4)`, `P107 (1, 7)`, `P112 (1, 12)`, and `P108 (3, 8)`. Update `P108` to Urgency 1. | Successive `extractMin()` calls return `P104`, then `P107`, then `P108`, then `P112`. |
| **TC-07** | **Min-Heap:** Urgency Update at Root | Update priority of the current root element (e.g., condition stabilizes from 1 to 4). | Root element sifts down; new root satisfies heap invariants. |
| **TC-08** | **Min-Heap:** Arbitrary Key Removal | Insert patients into heap; cancel a waiting patient in the middle; call `removePatient()`. | Target patient removed; heap reheapifies; direct handle map reflects accurate positions. |
| **TC-09** | **Appointment Queue:** FIFO & Cancellation | Enqueue `P102`, `P106`, `P111`. Cancel `P106`. Dequeue remaining patients. | Dequeue returns `P102`, then `P111`. Cancelled `P106` is bypassed. |
| **TC-10** | **BST:** Three-Case Node Deletion | Insert keys [50, 30, 70, 20, 40, 60, 80]. Delete leaf (20), 1-child (30), and 2-child (50). | Inorder traversal after each deletion maintains strict ascending order; child pointers intact. |
| **TC-11** | **BST:** Mirror Tree Independence | Build BST; call `createMirrorCopy()`. | Mirror tree inorder traversal yields descending order; original operational BST remains unchanged. |
| **TC-12** | **Sorting:** Presorted & Reverse Inputs | Run Quick Sort and Merge Sort on pre-sorted array [10..90] and reverse array [90..10]. | Both algorithms produce identical correct sorted arrays; Quick Sort executes safely without stack overflow. |
| **TC-13** | **Heapsort:** Standalone In-Place Sort | Run Heapsort on an unsorted array of 12 patient billing amounts directly in-place. | Array sorted ascending in $O(n \log n)$ time and $O(1)$ auxiliary space; matches Merge Sort output. |
| **TC-14** | **Graph:** Representation Synchronization | Add, block, and unblock corridor (1, 5). Inspect both `GraphAdjList` and `GraphAdjMatrix`. | Both representations update bidirectionally; absent edges (`-1`) distinguish from blocked edges (`-2`). |
| **TC-15** | **Graph:** Dijkstra Shortest Route & Zero Cost | Compute route from Node 0 to Node 3; compute route from Node 1 to Node 1. | 0 $\to$ 3 returns path `0 -> 1 -> 5 -> 3` (cost 270s); 1 $\to$ 1 returns path `[1]` (cost 0s). |
| **TC-16** | **Graph:** Corridor Block & Alternate Route | Block edge (1, 5); re-calculate route from Node 0 to Node 3. | Returns alternative path `0 -> 1 -> 7 -> 3` with exact verified transit cost **330s**. |
| **TC-17** | **Graph:** Unreachable Destination Handling | Disconnect Node 10 and 11 by blocking edge (8, 10); request path from Node 1 to 11. | Returns cost `kInfinity`, empty path stack, and reports destination unreachable without crashing. |
| **TC-18** | **Graph:** Kruskal vs Prim MST Equivalence | Execute both Kruskal (with DSU) and Prim on the connected 12-node campus topology. | Both algorithms return identical minimum spanning tree weight **1280s**; Kruskal detects no cycles. |
| **TC-19** | **Graph:** MST on Disconnected Graph | Block edge (8, 10); run Kruskal and Prim. | Kruskal reports disconnected graph and spanning forest; Prim reports partial component MST. |
| **TC-20** | **Graph:** BFS/DFS on Disconnected Graphs | Run BFS and DFS from disconnected components. | Correctly visits only reachable component; cycle detection correctly handles parent edges. |
| **TC-21** | **Dispatch:** Failed Dispatch Consistency | Request dispatch when all resources are busy or target leg is unreachable. | Dispatch fails; patient remains safely in `DISPATCH_PENDING` (Unassigned); no partial state modified. |
| **TC-22** | **Dispatch:** Duplicate Assignment Prevention | Attempt to assign a second resource to a patient already in `DISPATCH_PENDING (Assigned)`. | Second assignment is rejected; initial resource assignment remains intact. |
| **TC-23** | **Dispatch & Lifecycle:** Two-Legs, Handover, Reuse | Assign `GURNEY-01` to `P104`; complete transit; handover at Node 2. Assign `GURNEY-01` to `P105` before `P104` is discharged. | `GURNEY-01` is successfully reused by `P105`. Subsequent discharge of `P104` does not alter `GURNEY-01`. |
| **TC-24** | **Memory Hygiene & Leak Check** | Run 100 iterations of full patient lifecycle (registration, triage, routing, discharge, deletion). | Zero memory leaks detected by AddressSanitizer or MSVC debug heap. |

---

## 15. Implementation Milestones & Dependency Order

The project is structured into eight progressive milestones, integrating test coverage and console menu functionality at every stage:

```
[Milestone 1: Foundations, Domain Models, Seed Data & CMake]
       │
       ▼
[Milestone 2: Linear Data Structures, Hash Tables & Unit Tests]
       │
       ▼
[Milestone 3: Trees, Priority Min-Heap & Dynamic Urgency Updates]
       │
       ▼
[Milestone 4: Campus Graph System, Dijkstra Routing & MST Algorithms]
       │
       ▼
[Milestone 5: Hospital Workflow Services, Dispatch Engine & State Invariants]
       │
       ▼
[Milestone 6: Sorting Engines, Report Snapshots & Educational Visualizers]
       │
       ▼
[Milestone 7: Console User Interface & Interactive Workflows]
       │
       ▼
[Milestone 8: End-to-End Verification, Memory Audit & Viva Defense Pack]
```

### Milestone Breakdown:
1. **Milestone 1 (Foundations & Environment):**
   - Configure portable `CMakeLists.txt` with C++17 and compiler warning flags (`/W4` on MSVC, `-Wall -Wextra -pedantic` on GCC/Clang).
   - Implement domain models (`Patient.hpp`, `DispatchResource.hpp`, `CampusLocation.hpp`, `AuditEvent.hpp`).
   - Create deterministic synthetic data files (`data/patients_seed.csv`, `data/resources_seed.csv`, `data/campus_map_seed.txt`).
   - Set up test harness `TestRunner.cpp` registered with CTest.
2. **Milestone 2 (Linear Data Structures & Hashing):**
   - Implement `SinglyLinkedList<T>` and `DoublyLinkedList<T>`.
   - Implement `CustomStack<T>` and `CustomQueue<T>` with underflow protection.
   - Implement `HashTableChaining<K, V>` with polynomial hash and resizing to prime $\ge 2M + 1$.
   - Implement `HashTableProbing<K, V>` with tombstone sentinels.
   - Verify with TC-01, TC-02, TC-03, TC-04, TC-09.
3. **Milestone 3 (Hierarchical Structures & Priority Heaps):**
   - Implement `BinarySearchTree<K, V>` with 3-case deletion, traversals, and mirror cloning.
   - Implement `BinaryMinHeap<T, Comparator>` with composite comparator `(urgency, arrivalSequence)`.
   - Implement dynamic acuity update (`changeUrgency`) via `heapPosByHandle` preserving arrival order, updating root, and arbitrary removal (`removePatient`).
   - Verify with TC-05, TC-06, TC-07, TC-08, TC-10, TC-11.
4. **Milestone 4 (Graph Network & Routing Engine):**
   - Implement `GraphAdjList` and synchronized `GraphAdjMatrix` with bidirectional corridor blocking.
   - Implement `DisjointSetUnion` with union by rank and path compression.
   - Implement Dijkstra's algorithm with duplicate-entry heap, guarded arithmetic, and stack-based path reconstruction.
   - Implement BFS reachable exploration and DFS cycle detection with parent checking.
   - Implement Kruskal's and Prim's MST algorithms.
   - Verify with TC-14, TC-15, TC-16, TC-17, TC-18, TC-19, TC-20.
5. **Milestone 5 (Domain Orchestration Services):**
   - Implement `HospitalRegistry` managing `std::unique_ptr<Patient>` master storage and integer handle generation.
   - Implement `TriageManager` wrapping the emergency min-heap.
   - Implement `AppointmentManager` managing the FIFO outpatient queue.
   - Implement `DispatchManager` executing the deterministic two-leg dispatch algorithm, duplicate prevention, and handover resource release.
   - Implement `HospitalSystem` facade enforcing state transitions and audit logging.
   - Verify with TC-21, TC-22, TC-23.
6. **Milestone 6 (Report Snapshots, Sorting Engines & Educational Demonstrations) - [COMPLETED]:**
   - Implemented value-based `PatientSnapshot` records decoupled from mutable registry pointers.
   - Defined precise waiting duration calculation for active waiting patients ($t_{report} - t_{arrival}$); marked unavailable for patients in treatment/terminal states.
   - Implemented `ReportingService` with non-mutating reporting guarantee and deterministic tie-breaking for sorting by Age, Arrival Time, Urgency, Billing Amount, and Waiting Duration.
   - Implemented `CustomSorting.hpp` suite:
     * Custom Merge Sort: Stable, $O(n \log n)$ guarantee, $O(n)$ space.
     * Custom Quicksort: Unstable, Median-of-Three pivot, Lomuto partition, tail-recursion bounded stack $O(\log n)$, $O(n \log n)$ avg, $O(n^2)$ worst-case.
     * Custom Heapsort: Unstable, strictly in-place $O(1)$ auxiliary space without heap copies.
     * Standardized telemetry convention: comparisons ($1$ per `comp`), swaps ($1$ per swap), moves ($1$ per assignment in merge, $3$ per swap in quick/heap).
   - Implemented `DemonstrationService` covering:
     * Sorting comparison on identical snapshot inputs.
     * Separate Chaining vs Linear Probing on verified collision fixtures (`'A'` and `'R'`) with tombstone demonstration.
     * Adjacency List vs Adjacency Matrix traversal and complexity ($O(V+E)$ vs $O(V^2)$).
     * Binary Search Tree traversals, type-safe mirror tree, and 3-case node deletion (leaf, single-child, two-children).
     * Dijkstra shortest path (local path latency) vs Minimum Spanning Tree (global network connectivity).
   - Resolved all 5 focused consistency fixes (Fix 1-5).
   - Verified with 40/40 passing automated tests.
7. **Milestone 7 (Complete Interactive Console Interface & Reporting Refinements) - [COMPLETED]:**
   - **Reporting Refinements:**
     * Traced execution time decoupled from pure algorithmic duration: formatted as `"N/A (Traced)"` to prevent trace string formatting overhead from conflating algorithm benchmarking.
     * Report table columns widened with strict field allocation (ID: 18w, Name: 22w, Age: 5w, Urgency: 18w, State: 24w, ArrSeq: 8w, ArrTime: 14w, Wait Time: 28w, Billing: 14w); prevents overlapping on maximum supported IDs and large timestamps. Added `formatPatientDetail` displaying unabbreviated names (up to 64 chars) and complaints (up to 128 chars).
     * Corrected sorting and reporting CO references to Course Outcome 1 (CO1: Arrays, Recursion, and Sorting), with Heapsort also mapped to CO5 (Priority Queues & Heaps).
     * Documented `PatientSnapshot` as an independent value copy rather than a language-enforced immutable object.
     * Captured snapshot timestamp directly from `data.front().reportTimestamp` rather than caller-supplied clock, preventing mislabeling of older snapshots.
   - **Iterative Console Application Architecture (`ConsoleApp.hpp`):**
     * Stream-injectable architecture (`std::istream& in`, `std::ostream& out`) enabling lightweight deterministic unit testing.
     * Strict line-based token parsing (`readInt`, `readDouble`, `readString`, `readMenuChoice`); safely accepts spaces, hyphens, and apostrophes in names and complaints (`O'Connor`); rejects non-finite numbers and malformed inputs; clean EOF detection without infinite loops.
     * Non-recursive iterative loop architecture; all screens provide consistent return/back actions (`0: Back / Return to Main Menu`).
     * State mutations routed strictly through `HospitalSystem` facades (zero direct UI mutations of patient or resource state).
   - **Menu-Driven Subsystems Implemented:**
     * Patient Registration, Lookup (by ID and BST hierarchy), and Full Dossier View.
     * Emergency Triage and Dynamic Urgency Updates (preserving original arrival sequence).
     * Outpatient Appointment Booking, Viewing, FIFO Calling, Cancellation, and Emergency Conversion.
     * Mobile Resource Dispatch: candidate inspection, two-leg validation, assignment, pickup transit, transport & clinical handover, pickup cancellation, and final discharge.
     * Campus Map & Corridor Management: campus location listing, corridor blocking & reopening, point-to-point Dijkstra routing with path and cost breakdown, BFS and DFS explorations, connectivity checks, and Kruskal/Prim MST/MSF calculation.
     * Hospital Reports & Custom Sorting: field selection (Age, Arrival Time, Urgency, Billing, Waiting Duration) and algorithm selection (Merge Sort, Quicksort, Heapsort) with non-mutating guarantee.
     * Educational Demonstrations: Sorting Telemetry (CO1, CO5), Hashing Collision Resolution (CO2, CO5), Graph Traversals (CO3, CO5), BST Operations & Mirror (CO3, CO5), and Dijkstra vs MST (CO3, CO5).
     * Cross-Service System Audit History & Invariant Integrity Checker (`validateSystemIntegrity`).
   - **Execution Modes & CLI Interface:**
     * Interactive console mode (`SmartHospitalSystem.exe [data-dir]` or `--data-dir <path>`).
     * Deterministic automated verification mode (`SmartHospitalSystem.exe --demo [data-dir]`) running all educational demonstrations and terminating automatically with exit code 0.
     * Help display (`--help` / `-h`) exiting with code 0.
     * Strict rejection of unknown/unsupported CLI flags with exit code 1.
     * Updated `build_and_test.bat` to run `TestRunner.exe data/` and `SmartHospitalSystem.exe --demo data/` without hanging.
   - **Verification:**
     * Added `testConsoleAppScriptedWorkflows` with 9 comprehensive subtests covering immediate exit/EOF, invalid inputs, registration, appointments, triage update, dispatch lifecycle, blocked corridor retry, report non-mutation, and integrity verification.
     * Verified with 41/41 passing automated tests.
8. **Milestone 8 (Final Submission Audit, Academic Documentation & Packaging):**
   - Execute the complete test suite (TC-01 through TC-24).
   - Run memory leak audit with AddressSanitizer and debug heap tools.
   - Complete viva preparation documentation (`docs/viva_prep.md`).

---

## 16. Minimum Complete Submission Scope vs. Optional Extensions

### 16.1 Guaranteed Minimum Submission Scope (Baseline)
1. Complete C++17 compilation under portable warning flags (`/W4` on MSVC, `-Wall -Wextra -pedantic` on GCC/Clang).
2. Custom implementations of:
   - Singly Linked List, Doubly Linked List, Stack, Queue.
   - Hash Table (Separate Chaining).
   - Binary Search Tree (with 3-case deletion and mirror copy).
   - Binary Min-Heap (with composite comparator, handle-based position tracking, and arrival-order preserving updates).
   - Graph (Synchronized Adjacency List & Adjacency Matrix).
   - Disjoint Set Union (with path compression and union by rank).
3. Custom implementations of:
   - Dijkstra's shortest path with obstacle rerouting and guarded arithmetic.
   - Prim's and Kruskal's MST algorithms (verified sum 1280s).
   - BFS reachable exploration and DFS cycle detection.
   - Recursive Merge Sort and Quick Sort with step traces.
   - Standalone in-place Heapsort.
4. Working hospital workflow: Registration $\to$ Triage $\to$ Urgency Update $\to$ Two-Leg Dispatch $\to$ Handover $\to$ Treatment $\to$ Discharge.
5. Outpatient appointment handling (book, view, process FIFO, cancel).
6. Complete execution of all 24 verification test cases.

### 16.2 Optional Academic Extensions
1. **Generic Arithmetic Expression Evaluator:** Custom Stack-based Infix-to-Postfix conversion and postfix evaluation algorithm.
2. **Sparse Bed Occupancy Matrix:** Triplet representation `(row, col, value)` with Fast Transpose algorithm ($O(n + \text{terms})$).
3. **On-Call Staff Circular Roster:** Circular linked list simulating round-robin physician on-call rotations.

---

## 17. Risks, Limitations, and Faculty Defense Q&A

### 17.1 Potential Technical Pitfalls & Mitigations
1. **BST Degradation on Presorted IDs:** Monotonically increasing IDs (`P101`, `P102`...) cause an unbalanced BST to degrade to $O(n)$.  
   *Mitigation:* Primary $O(1)$ lookups use the hash table; the BST is used for range/ordered demographic queries and viva demonstrations. The limitation of unbalance is explicitly explained, motivating balanced trees (AVL/Red-Black).
2. **Corrupted Resource Ownership:** A resource reassigned after patient handover being released again on patient discharge.  
   *Mitigation:* Resources are released **exclusively at handover**. The subsequent discharge action does not touch any dispatch resource.
3. **Infinite Loops on Disconnected Graphs:** Dijkstra querying an unreachable destination.  
   *Mitigation:* Distances initialize to `kInfinity = std::numeric_limits<int64_t>::max() / 4`. Reachability is verified before path reconstruction.

### 17.2 Top Faculty Viva Questions & Answers
- **Q1: Why did you implement a custom Binary Min-Heap instead of using `std::priority_queue`?**  
  *Answer:* `std::priority_queue` does not support updating priority in place. In an emergency room, a patient's condition can deteriorate or stabilize while waiting. Our custom Min-Heap supports updating urgency while strictly preserving the patient's original arrival sequence, executing sift-up or sift-down in $O(\log n)$ heap time via direct handle position indexing.
- **Q2: Why maintain both an Adjacency List and an Adjacency Matrix?**  
  *Answer:* The campus graph is sparse ($V = 12, E = 15$). An adjacency list requires $O(V + E)$ space and allows Dijkstra and Kruskal to iterate only over existing corridors. We maintain a synchronized adjacency matrix to demonstrate matrix DFS and cycle detection, highlighting the $O(V^2)$ traversal space/time cost in contrast to the list.
- **Q3: What is the fundamental difference between Dijkstra’s algorithm and an MST?**  
  *Answer:* Dijkstra computes the minimum total weight path from a single origin to a specific target node (e.g., fastest route for an emergency gurney from ER to OT). An MST computes a spanning subgraph connecting all vertices together with the minimum total edge weight (e.g., minimum transit network or communications backbone). An MST edge does not guarantee the shortest point-to-point path between arbitrary vertices.
- **Q4: Why does naive Quick Sort degrade on presorted arrays, while Merge Sort does not?**  
  *Answer:* Merge Sort divides arrays unconditionally into two equal halves, guaranteeing $\Theta(n \log n)$ time in all cases. Naive Quick Sort partitioning around the first or last element of an already sorted array produces partitions of size $0$ and $n-1$, resulting in $n$ recursive levels, $O(n^2)$ time, and $O(n)$ recursion stack space.

---

---

## 18. Documentation & Viva Preparation Checklist (Milestone 8 Completed)

- [x] **Architecture Diagram & State Flowcharts:** Documented in `docs/project_report.md` with patient and resource state machines.
- [x] **Data Structure Inventory & CO Mapping:** Documented in `docs/project_report.md` and `docs/co_mapping.md` covering CO1 through CO6.
- [x] **Complexity Summary Sheet:** Documented in `docs/complexity_reference.md` with expected $O(1)$ hash lookup, $O(h)$ BST ($O(\log n)$ avg, $O(n)$ worst-case), and $O(1)$ auxiliary space Heapsort.
- [x] **Deterministic Demonstration Scripts:** Step-by-step walkthrough in `docs/demo_script.md` and self-validating non-interactive mode (`--demo`) returning exit code 0.
- [x] **Verification & Audit Report:** Documented in `docs/verification_report.md` recording 41/41 automated tests passing, strict CLI argument validation, external directory execution, and explicitly labeling unverified tools.
- [x] **Viva Examination Guide:** Documented in `docs/viva_prep.md` explaining data structures, design decisions, edge cases, and oral defense answers in second-year student language.

---

## 19. Definition of Done (DoD) — Milestone 8 Final Verification Status

1. **Compilation Cleanliness:** Rebuilt both targets with MinGW GCC 6.3.0 (`-std=c++1z -Wall -Wextra -pedantic`) with **0 warnings and 0 errors**.
2. **Data Structure Independence:** All 11 custom data structures and 8 core algorithms implemented from scratch without STL container equivalents.
3. **Automated Test Battery:** All **41 automated test cases** in `TestRunner.exe` pass deterministically (100% pass rate).
4. **Self-Validating Demo:** `SmartHospitalSystem.exe --demo data/` executes all workflows, verifies internal invariants after each step, and returns exit code 0.
5. **Interactive Console Application:** Runs stably across all 9 submenus, supports quoted paths, space/apostrophe handling, input error recovery, and clean EOF termination.
6. **Seed Dataset Verification:** Loaded and verified 12 synthetic patients, 3 gurneys, 12 locations, and 15 corridors (total weight sum = 1850s, Dijkstra seed route 0->3 = 270s, Kruskal/Prim MST = 1280s).
7. **Clinical Lifecycle & Integrity:** Full lifecycle from registration through discharge verified with cross-service consistency check after every step.
8. **Academic Documentation & Packaging:** All 6 Markdown reports created in `docs/`, clean source archive prepared, and unverified tools honestly reported.

---

## 20. Milestone 8 Audit & Final Packaging Summary

- **Course Outcome (CO) Formal Alignment:**
  * **CO1:** Arrays, recursion, custom Merge Sort (Stable) and Quicksort (Median-of-3).
  * **CO2:** Singly linked lists, Doubly linked list audit trail, HashTableChaining (expected $O(1)$ lookup), HashTableProbing (linear probing).
  * **CO3:** CustomStack (LIFO for Dijkstra reconstruction), CustomQueue (FIFO for outpatient queue and BFS).
  * **CO4:** BinarySearchTree (secondary patient index, 3-case deletion, traversals, $O(h)$ search), MirrorBinaryTree.
  * **CO5:** Generic BinaryMinHeap, TriageMinHeap (acuity 1–5, arrival sequence FIFO tie-breaking, $O(\log n)$ acuity update), Custom Heapsort ($O(1)$ auxiliary memory).
  * **CO6:** CampusGraph (synchronized matrix & list), BFS/DFS, Dijkstra shortest path (270s), Kruskal & Prim MST (1280s), dynamic corridor closures.
- **Source Package Structure:**
  The complete clean submission archive contains: `CMakeLists.txt`, `build_and_test.bat`, `README.md`, `plan.md`, `include/`, `src/`, `data/`, `tests/`, and `docs/`. Stale binaries, temporary object files, and logs are cleanly excluded.

