# Interactive Demonstration Walkthrough Script
## Smart Hospital Emergency Triage & Route Dispatch System (CS2305)

This script provides an exact, reproducible **5 to 8 minute walkthrough** for viva examinations, faculty evaluation, and live grading. It matches the actual application source code, seed files, menu numbering, prompt sequences, and console outputs from a fresh launch.

An exact recorded execution transcript is saved as evidence in [docs/demo_transcript.txt](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/demo_transcript.txt).

---

### Prerequisites & Launch

1. Open PowerShell or Command Prompt.
2. Navigate to the project root directory:
   ```powershell
   cd "c:\Users\kpvp0\Downloads\Kashish College\2ND Year\Sem1\DS\CP"
   ```
3. Launch the interactive application with the seeded dataset:
   ```powershell
   .\bin\SmartHospitalSystem.exe data/
   ```
   *(Or using explicit quoted paths: `& ".\bin\SmartHospitalSystem.exe" "data/"`)*
4. Verify the startup banner displays:
   - Clock: `65s` *(Derived from the latest simulated arrival time, Barry Allen at 65s, in `data/patients_seed.csv`)*
   - Patients: `12` *(12 seeded patients: P101 through P112)*
   - Fleet: `3` *(GURNEY-01 at Node 0, GURNEY-02 at Node 6, GURNEY-03 at Node 1)*
   - Operational mode: `[Memory-Only Mode]`

---

### Timeline Overview

| Stage | Estimated Time | Feature / Data Structure | Menu Path | Course Outcome |
| :--- | :--- | :--- | :--- | :--- |
| **Stage 1** | 0:00 – 1:30 | Emergency Patient Registration & Priority Triage | Menu `1` & Menu `2` | CO2 (Hash/List), CO5 (Min-Heap) |
| **Stage 2** | 1:30 – 3:00 | Outpatient Queue, Consult & Deterioration | Menu `3` | CO3 (Queue, Stack) |
| **Stage 3** | 3:00 – 4:30 | 2-Leg Ambulance Dispatch & Transit | Menu `4` | CO5 (Priority), CO6 (Shortest Path) |
| **Stage 4** | 4:30 – 6:00 | Corridor Blockage, Dijkstra Rerouting & MST | Menu `5` | CO6 (Dijkstra, Kruskal, Prim) |
| **Stage 5** | 6:00 – 7:15 | Analytics Reports & Algorithm Benchmarks | Menu `6` & Menu `7` | CO1, CO5 (Sorting Suite, BST) |
| **Stage 6** | 7:15 – 8:00 | Cross-Service Audit & System Invariants | Menu `8` & Exit | System Integrity & Lifecycle |

---

### Step-by-Step Walkthrough Guide

#### STAGE 1: Emergency Patient Registration & Acuity Triage (0:00 – 1:30)
**Goal:** Show $O(1)$ hash table insertion, BST secondary indexing, and min-heap priority triage.

1. At the Main Menu, type `1` and press `Enter` to enter **Patient Registration & Registry Management**.
2. Type `1` to **Register New Inpatient / Emergency Patient**.
3. Enter the following test inputs when prompted:
   - Patient ID: `P501`
   - Full Legal Name: `Jonathan Wick`
   - Patient Age: `45`
   - Chief Medical Complaint: `Severe chest trauma with hypotension`
   - Acuity Urgency: `1` *(1 = Resuscitation)*
   - Current Campus Location Node ID: `0` *(Ambulance Reception Bay)*
   - Target Destination Node ID: `3` *(Surgical Operating Theatres)*
   - Initial Billing Balance: `1500.00`
4. **Expected Observation:**
   - Console outputs:
     ```
     [SUCCESS] Patient P501 registered successfully!
     Full Name:      Jonathan Wick
     Initial State:  REGISTERED
     Urgency Acuity: 1 (1 - Resuscitation)
     Current Node:   0 (Ambulance Reception Bay)
     Target Node:    3 (Surgical Operating Theatres)
     ```
   - Total patient count advances to 13. System clock remains `65s`.
5. Type `2` to test **Search Patient Dossier by ID**:
   - Patient ID: `P501`
   - Observe dossier showing Arrival Sequence `#13`, state `REGISTERED`, location Node 0.
6. Type `0` to return to the Main Menu.
7. Type `2` and press `Enter` to open **Emergency Clinical Triage & Acuity Management**.
8. Type `1` to **View Triage Priority Queue Status & Highest Acuity Candidate**:
   - Observe:
     ```
     Active Patients Waiting in Triage Heap: 9
     Highest Priority Triage Candidate:
       Patient ID:       P104 (Marcus Brody)
       Clinical Urgency: 1 - 1 - Resuscitation
       Arrival Sequence: #4 (FIFO Tie-Breaker)
       Current Location: Node 1 (ER Triage & Main Waiting Hall)
       Patient State:    TRIAGED_WAITING
     ```
9. Type `2` to **Intake Registered Patient into Emergency Triage Priority Queue**:
   - Patient ID: `P501`
   - Assign Clinical Urgency Level: `1`
   - Observe: `[SUCCESS] Patient P501 prioritized into triage min-heap with urgency 1 (1 - Resuscitation). Patient State: TRIAGED_WAITING.`
   > [!NOTE]
   > **Faculty Oral Defense Point:** `P501` is intaken at highest urgency (1), but its assigned arrival sequence is `#13`. Earlier urgency-1 arrivals (`P104` with sequence #4, `P107` with sequence #7, and `P112` with sequence #12) still outrank `P501` because the binary min-heap comparator strictly enforces FIFO arrival sequence tie-breaking among equal-acuity records.
10. Type `3` to demonstrate **Update Patient Clinical Urgency (Preserves Arrival Sequence)**:
    - Patient ID: `P108` *(Natasha Romanoff, seed urgency 3)*
    - Enter New Urgency Level: `2` *(2 - Emergent)*
    - Observe:
      ```
      [SUCCESS] Urgency for patient P108 successfully updated to 2 (2 - Emergent).
      Preserved Arrival Sequence: #8 (maintains stable FIFO tie-breaking).
      ```
11. Type `0` to return to the Main Menu.

---

#### STAGE 2: Outpatient Appointments, FIFO Turns & Condition Conversion (1:30 – 3:00)
**Goal:** Demonstrate custom FIFO queue processing, encapsulated node deletion, and queue-to-heap state migration.

1. At the Main Menu, type `3` and press `Enter` for **Outpatient Clinic Appointment Services**.
2. Type `2` to **View Scheduled Outpatient Appointment Roster**:
   - Observe the 3 actual seeded outpatient appointments:
     ```
     Queue # Patient ID      Patient Name            Arrival Clock 
     --------------------------------------------------------------------------------
     1       P102            Arthur Dent             15s           
     2       P106            Diana Prince            35s           
     3       P111            Walter White            60s           
     ```
     *(Note: P110 Ellen Ripley is an emergency waiting patient in the triage heap, not an appointment).*
3. Type `3` to **Call Next Appointment for Consultation**:
   - Observe:
     ```
     [SUCCESS] Consult Turn Called: Patient P102 (Arthur Dent).
     Updated Patient State: UNDER_TREATMENT (Consultation underway at Node 1).
     ```
   - `P102` is called in strict FIFO order and transitions to `UNDER_TREATMENT`.
4. Type `4` to demonstrate **Cancel Outpatient Appointment**:
   - Patient ID: `P111` *(Walter White)*
   - Observe: `[SUCCESS] Appointment for patient P111 cancelled and removed from queue. Patient State: CANCELLED.`
5. Type `5` to demonstrate **Convert Outpatient to Emergency Triage (Condition Deterioration)**:
   - Patient ID: `P106` *(Diana Prince)*
   - Enter Deteriorated Urgency Acuity: `1` *(1 - Resuscitation)*
   - Observe: `[SUCCESS] Patient P106 condition deteriorated; converted to Emergency Triage with Urgency 1 (1 - Resuscitation). Patient State: TRIAGED_WAITING.`
6. Type `2` to view the roster again:
   - Observe: `Total Scheduled Outpatient Appointments: 0` (all 3 appointments were consulted, cancelled, or escalated).
7. Type `0` to return to Main Menu.

---

#### STAGE 3: Two-Leg Dispatch Simulation & Handover (3:00 – 4:30)
**Goal:** Show priority candidate selection, nearest resource matching, Leg 1 (pickup), Leg 2 (transport), and patient discharge.

1. At Main Menu, type `4` and press `Enter` for **Ambulance Dispatch & Patient Transit Operations**.
2. Type `1` to **Select Next Priority Emergency Candidate for Dispatch**:
   - The system evaluates the triage priority queue.
   - Observe that `P104` (Marcus Brody) is selected:
     ```
     [SELECTED CANDIDATE]
       Patient ID:       P104 (Marcus Brody)
       Clinical Urgency: 1 - 1 - Resuscitation
       Arrival Sequence: #4
       Current Node:     1 (ER Triage & Main Waiting Hall)
       Target Node:      2 (Emergency Resuscitation Bay)
       Patient State:    DISPATCH_PENDING
       Assigned Resource: Unassigned
     ```
     *Rationale: `P104` has Urgency 1 and Arrival Sequence #4, outranking all other candidates (`P107` #7, `P112` #12, `P106` #6 converted, and `P501` #13).*
3. Type `2` to **Assign Available Dispatch Resource**:
   - Patient ID: `P104`
   - Observe:
     ```
     [ASSIGNMENT SUCCESSFUL]
     Assigned Resource: GURNEY-03 (Rapid Triage Wheelchair)
     Patient State:     DISPATCH_PENDING (Awaiting Pickup Arrival)
     Resource State:    EN_ROUTE_PICKUP (En Route to Pickup)

     [DISPATCH ROUTE LEGS & ESTIMATED TRANSIT COSTS]
     * Leg 1 (Pickup Leg): Resource Node 1 -> Patient Node 1
       Transit Duration: 0 seconds
       Path: 1
     * Leg 2 (Transport Leg): Patient Node 1 -> Destination Node 2
       Transit Duration: 100 seconds
       Path: 1 -> 2
       Total Mission Transit Cost: 100 seconds
     ```
     *Resource selection rationale: `GURNEY-03` is already located at Node 1 (pickup transit cost: 0s), whereas `GURNEY-01` is at Node 0 (cost 60s) and `GURNEY-02` is at Node 6 (cost 240s).*
4. Type `3` to execute **Simulation Step 1: Resource Travels to Patient Pickup Point**:
   - Patient ID: `P104`
   - Observe:
     ```
     [PICKUP ARRIVAL COMPLETE]
     Leg 1 Elapsed Transit Time: 0 seconds
     Updated Simulation Clock:   65s
     Patient State:              IN_TRANSIT (Patient loaded onto transport)
     Resource State:             TRANSPORTING_PATIENT at Node 1
     ```
5. Type `4` to execute **Simulation Step 2: Transport Patient & Complete Clinical Handover**:
   - Patient ID: `P104`
   - Observe:
     ```
     [TRANSPORT & HANDOVER COMPLETE]
     Leg 2 Elapsed Transit Time: 100 seconds
     Updated Simulation Clock:   165s
     Patient State:              UNDER_TREATMENT at Treatment Destination Node 2 (Emergency Resuscitation Bay)
     Resource State:             AVAILABLE (Resource released at destination; ready for new dispatches)
     ```
     *Clock advances from 65s to 165s (+100s transit). `GURNEY-03` is released back to `AVAILABLE` immediately at destination Node 2.*
6. Type `6` to **Discharge Patient from Clinical Treatment**:
   - Patient ID: `P104`
   - Observe: `[SUCCESS] Patient P104 successfully discharged after medical treatment. Final State: DISCHARGED.`
7. Type `0` to return to Main Menu.

---

#### STAGE 4: Campus Routing, Corridor Closure & MST Verification (4:30 – 6:00)
**Goal:** Show Dijkstra shortest path, dynamic graph obstacle avoidance, and Kruskal vs Prim MST equivalence.

1. At Main Menu, type `5` and press `Enter` for **Campus Topology, Routing & Corridor Management**.
2. Type `5` to **Query Shortest Route between Locations (Dijkstra's Algorithm)**:
   - Origin Node: `0` *(Ambulance Reception Bay)*
   - Destination Node: `3` *(Surgical Operating Theatres)*
   - Observe:
     ```
     [ROUTE FOUND]
     Total Transit Cost: 270 seconds
     Hop Count:          3 corridor(s)
     Navigation Sequence:
       1. Node 0 (Ambulance Reception Bay)
       2. Node 1 (ER Triage & Main Waiting Hall)
       3. Node 5 (Central Diagnostic Radiology & CT)
       4. Node 3 (Surgical Operating Theatres)
     ```
     *(Cost derivation: 60s + 120s + 90s = 270s).*
3. Type `3` to **Block Corridor (Closure / Maintenance)**:
   - Node U: `1`
   - Node V: `5`
   - Observe: `[SUCCESS] Corridor between Node 1 and Node 5 is now BLOCKED. Dijkstra route planner will automatically avoid this corridor.`
4. Type `5` to **Re-query Shortest Route**:
   - Origin Node: `0`
   - Destination Node: `3`
   - Observe dynamic rerouting:
     ```
     [ROUTE FOUND]
     Total Transit Cost: 330 seconds
     Hop Count:          3 corridor(s)
     Navigation Sequence:
       1. Node 0 (Ambulance Reception Bay)
       2. Node 1 (ER Triage & Main Waiting Hall)
       3. Node 7 (Central Inpatient Pharmacy)
       4. Node 3 (Surgical Operating Theatres)
     ```
     *(Cost derivation: 60s [0->1] + 140s [1->7] + 130s [7->3] = 330s).*
5. Type `4` to **Reopen Corridor**:
   - Node U: `1`
   - Node V: `5`
   - Observe: Corridor (1 <-> 5) has been REOPENED.
6. Type `7` to **Compute Minimum Spanning Tree / Forest**:
   - Observe:
     ```
     [FULLY CONNECTED GRAPH]
     Result is a MINIMUM SPANNING TREE (MST).

     [ALGORITHM COMPARISON]
     * Kruskal's Algorithm (Edge-centric greedy with DSU):
       Total Spanning Weight: 1280 seconds
       Edge Count:            11
     * Prim's Algorithm (Vertex-centric greedy with Min-Heap):
       Total Spanning Weight: 1280 seconds
       Edge Count:            11
     [VERIFICATION PASS] Kruskal and Prim total weights match identically (1280s).
     ```
7. Type `0` to return to Main Menu.

---

#### STAGE 5: Patient Reports & Educational Demonstrations (6:00 – 7:15)
**Goal:** Show custom sorting algorithms (Merge, Quick, Heap) on independent value snapshots without altering live state, plus BST and Hash collision demos.

1. At Main Menu, type `6` and press `Enter` for **Patient Reports & Custom Sorting Analytics**.
2. Type `1` to **Generate Summary Report with Custom Sorting Field & Algorithm**:
   - Select Sort Field: `3` *(Clinical Urgency)*
   - Select Sorting Algorithm: `1` *(Custom Merge Sort)*
   - Observe the 13-patient report snapshot captured at timestamp `165s`.
   - Observe Sorting Telemetry:
     ```
     [SORTING TELEMETRY METRICS]
     Algorithm Applied:     Merge Sort (Stable)
     Element Record Count:  13
     Comparisons Count:     35
     Element Swaps Count:   0
     Standardized Moves:    98
     Algorithm Stability:   STABLE (Equal keys preserve relative order)
     Auxiliary Space Used:  O(n)
     ```
   - Notice that `P104` displays `N/A (DISCHARGED)` and `P102` displays `N/A (UNDER_TREATMENT)` for active waiting duration, while active triage patients display actual waiting times ($t_{snapshot} - t_{arrival}$).
   - Read-Only Guarantee: Live system registries and clocks remain 100% unaltered.
3. Type `0` to return to Main Menu.
4. Type `7` and press `Enter` for **Educational Data Structure Demonstrations (CO1 - CO6)**.
5. Type `2` to run **Demo 1: Sorting Algorithms Comparison Suite (Merge vs Quick vs Heap) (CO1, CO5)**:
   - Displays comparative metrics across Merge Sort (35 comps, 0 swaps, 98 moves, stable), Quicksort (52 comps, 25 swaps, 75 moves, unstable), and Heapsort (61 comps, 41 swaps, 123 moves, unstable, strictly $O(1)$ auxiliary space).
6. Type `5` to run **Demo 4: Binary Search Tree Operations (CO4)**:
   - Displays 7-node BST (`[50, 30, 70, 20, 40, 60, 80]`), 4 traversals (In-order, Pre-order, Post-order, Level-order), mirrored tree, and 3-case node deletion (leaf 20, single-child 30, two-child 70 replaced by in-order successor 80).
7. Type `0` to return to Main Menu.

---

#### STAGE 6: System Audit Trail, Invariant Verification & Exit (7:15 – 8:00)
**Goal:** Demonstrate cross-service consistency enforcement, immutable audit logging, and clean system shutdown.

1. At Main Menu, type `8` and press `Enter` for **System Audit Trail & Cross-Service Integrity Check**.
2. Type `1` to **View Chronological Audit Trail Log (DoublyLinkedList Event History)**:
   - Displays 37 recorded audit events tracking seed registrations, registration of P501, triage intake, acuity changes, appointment actions, dispatch transit legs, corridor closures, and patient discharge.
3. Type `2` to **Run Cross-Service System Consistency & Invariant Verification**:
   - Observe confirmation:
     ```
     [SYSTEM INTEGRITY: VERIFIED HEALTHY]
     * Triage binary min-heap invariant holds across all records.
     * All waiting triage patients exist in the heap and have TRIAGED_WAITING state.
     * Appointment queue FIFO order and reciprocal states match registry perfectly.
     * Pending dispatch pool contains all unassigned candidates with exactly 1 reference.
     * Assigned patients and vehicles have reciprocal 1-to-1 matching and compatible states.
     * Terminal patients (DISCHARGED, CANCELLED) are completely isolated from active queues.
     ```
4. Type `0` to return to Main Menu.
5. Type `9` to **Exit System**:
   - Observe clean exit message reminding user of volatile in-memory operation.

---

### Non-Interactive Automated Alternative

If presentation time is limited, faculty can verify all operations and data structure invariants in a single command via:
```powershell
.\bin\SmartHospitalSystem.exe --demo data/
```
*Executes the automated scenario, verifies all invariants, and exits with code 0.*
