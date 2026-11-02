# Final System Verification & Audit Report
## Smart Hospital Emergency Triage & Route Dispatch System (CS2305)

**Audit Date:** October 2026  
**Academic Term:** AY 2026–27 (B.Tech Computer Engineering, 2nd Year / Sem 1)  
**Verification Target:** Milestone 8 Final Submission  

---

### 1. Environment & Toolchain Details

| Parameter | Host Configuration | Notes |
| :--- | :--- | :--- |
| **Operating System** | Microsoft Windows 10/11 (x64) | Tested on local development host |
| **Shell Environment** | Windows PowerShell 5.1 / Windows Command Prompt (`cmd.exe`) | Standard Windows console shell |
| **Active Compiler** | MinGW GCC 6.3.0 (`g++.exe`) | Location: `C:\MinGW\bin\g++.exe` |
| **Language Dialect Flag** | `-std=c++1z` | GCC 6.3 experimental/draft C++17 support flag. Does **not** represent full ISO C++17 conformance, which requires newer toolchains (GCC 7+). |
| **Compiler Warning Flags** | `-Wall -Wextra -pedantic` | Zero compiler warnings emitted across all sources. |
| **Optimization Flags** | None specified (default MinGW compiler behavior, without `-O2` or `-g`) | Standard default compilation. Not a debug-symbol build. |
| **Build Mechanism** | Batch automation script (`build_and_test.bat`) | Direct MinGW compilation without third-party toolchain managers. |

---

### 2. Executed Compilation Verification

Both targets were rebuilt from source using clean target directories with the exact commands from `build_and_test.bat`:

```cmd
g++ -std=c++1z -Wall -Wextra -pedantic -I include src\models\Patient.cpp src\models\DispatchResource.cpp src\services\DataLoader.cpp src\main.cpp -o bin\SmartHospitalSystem.exe
g++ -std=c++1z -Wall -Wextra -pedantic -I include src\models\Patient.cpp src\models\DispatchResource.cpp src\services\DataLoader.cpp tests\TestRunner.cpp -o bin\TestRunner.exe
```

- **`bin\SmartHospitalSystem.exe`**: Compiled successfully. Warnings: **0**, Errors: **0**. Binary size: ~1.56 MB.
- **`bin\TestRunner.exe`**: Compiled successfully. Warnings: **0**, Errors: **0**. Binary size: ~2.23 MB.

---

### 3. Automated Test Suite Results (`TestRunner.exe`)

The automated test harness executes the 41 registered `RUN_TEST` modules covering data structures, domain services, exception safety, and transactional rollbacks.

**Execution Command:**
```cmd
.\bin\TestRunner.exe data/
```

**Results Summary:**
- **Total Tests Executed:** 41
- **Passed:** 41
- **Failed:** 0
- **Pass Rate:** 100%

#### Actual Registered Test Execution Breakdown:
1. `RUN_TEST(testPatientModelAndStateQueries)` — **PASS**
2. `RUN_TEST(testModelLifecycleAndHandoverDemo)` — **PASS**
3. `RUN_TEST(testProductionDistanceArithmetic)` — **PASS**
4. `RUN_TEST(testSeedDataIntegrity)` — **PASS**
5. `RUN_TEST(testDataLoaderNegativeValidation)` — **PASS**
6. `RUN_TEST(testSinglyLinkedList)` — **PASS**
7. `RUN_TEST(testDoublyLinkedList)` — **PASS**
8. `RUN_TEST(testCustomStack)` — **PASS**
9. `RUN_TEST(testCustomQueue)` — **PASS**
10. `RUN_TEST(testHashTableChaining)` — **PASS**
11. `RUN_TEST(testHashTableProbing)` — **PASS**
12. `RUN_TEST(testExceptionSafeCopySemantics)` — **PASS**
13. `RUN_TEST(testBinarySearchTree)` — **PASS**
14. `RUN_TEST(testBinaryMinHeapAndTriage)` — **PASS**
15. `RUN_TEST(testCampusGraphAndRouting)` — **PASS**
16. `RUN_TEST(testHospitalRegistryAndRegistration)` — **PASS**
17. `RUN_TEST(testSeedInitializationAndIndexes)` — **PASS**
18. `RUN_TEST(testEqualUrgencyFIFOAndUrgencyUpdates)` — **PASS**
19. `RUN_TEST(testAppointmentFIFOAndCancellationAndConversion)` — **PASS**
20. `RUN_TEST(testDispatchFailuresAndPreconditions)` — **PASS**
21. `RUN_TEST(testDeterministicResourceSelectionAndTieBreaking)` — **PASS**
22. `RUN_TEST(testSimulationLegsCorridorClosureAndRerouting)` — **PASS**
23. `RUN_TEST(testPickupCancellation)` — **PASS**
24. `RUN_TEST(testResourceReuseAndDischargeIsolation)` — **PASS**
25. `RUN_TEST(testTerminalStateRejectionAndAudit)` — **PASS**
26. `RUN_TEST(testPendingDispatchOrderingPolicy)` — **PASS**
27. `RUN_TEST(testCompleteRegistrationToDischargeEndToEnd)` — **PASS**
28. `RUN_TEST(testMilestone5MoveDisablingPolicy)` — **PASS**
29. `RUN_TEST(testConsistentInputValidationAndStatePreservation)` — **PASS**
30. `RUN_TEST(testPreserveAppointmentOrderOnFailedConversion)` — **PASS**
31. `RUN_TEST(testRejectedOperationHandlingAndReciprocalChecks)` — **PASS**
32. `RUN_TEST(testCoherentTimeSemanticsAndSeedSnapshot)` — **PASS**
33. `RUN_TEST(testRegistryRegistrationExceptionSafetyRollback)` — **PASS**
34. `RUN_TEST(testInitializationResetAndSeedValidation)` — **PASS**
35. `RUN_TEST(testSystemIntegrityCheckerVerification)` — **PASS**
36. `RUN_TEST(testMilestone6ConsistencyFixes)` — **PASS**
37. `RUN_TEST(testCustomSortingAlgorithms)` — **PASS**
38. `RUN_TEST(testSortingStability)` — **PASS**
39. `RUN_TEST(testPatientSnapshotAndReportingService)` — **PASS**
40. `RUN_TEST(testEducationalDemonstrations)` — **PASS**
41. `RUN_TEST(testConsoleAppScriptedWorkflows)` — **PASS**

---

### 4. Self-Validating Non-Interactive Demonstration (`--demo`)

The `--demo` mode executes a scripted verification scenario to validate core integrated operations and return an automated process exit status. It does **not** claim to test every permutation of user interaction; rather, it provides a deterministic, self-validating smoke test of primary workflows.

**Execution Command:**
```cmd
.\bin\SmartHospitalSystem.exe --demo data/
```

**Executed Evidence & Observations:**
- **Initial Dataset:** Loaded 12 synthetic patients (`P101` through `P112`), 3 gurneys (`GURNEY-01` at Node 0, `GURNEY-02` at Node 6, `GURNEY-03` at Node 1), 12 campus locations, and 15 corridors.
- **Initial Clock:** Initialized to **65s** (matching latest seed arrival time of Barry Allen, `P112`).
- **Data Structure Invariant Checks:**
  * Stack LIFO ordering: Verified push/pop integrity.
  * Queue FIFO ordering: Verified front/dequeue order.
  * DoublyLinkedList audit trail: Verified forward and backward link consistency.
  * HashTableChaining: Located `P104` (`Marcus Brody`, Urgency 1).
  * HashTableProbing: Located `GURNEY-02` (`Critical Care Mobile Unit`, state `AVAILABLE`).
  * BST: Verified 12 indexed patient nodes and searched key `P104`.
  * TriageMinHeap: Verified root element is highest acuity (`P104`, Urgency 1, Arrival Sequence #4).
- **Routing & MST Invariants:**
  * Dijkstra shortest path from Node 0 (Ambulance Reception Bay) to Node 3 (Surgical Operating Theatres):
    Path `0 -> 1 -> 5 -> 3`, Cost: **270 seconds** (60s + 120s + 90s).
  * Kruskal MST: Total transit weight = **1280 seconds**, Edge count = 11.
  * Prim MST: Total transit weight = **1280 seconds**, Edge count = 11.
  * Kruskal and Prim total weights match identically.
- **Integrated Clinical Workflow Execution:**
  1. Patient `P501` (`Jonathan Wick`) registered at Node 0 with target Node 3, Urgency 1.
  2. Patient `P501` intaken into triage min-heap with Urgency 1 (assigned sequence #13).
  3. Acuity update on `P108` (`Natasha Romanoff`) from Urgency 3 to Urgency 2, preserving arrival sequence `#8`.
  4. Outpatient consultation turn called for `P102` (`Arthur Dent`), transitioning to `UNDER_TREATMENT`.
  5. Dispatch priority evaluation: Selects highest priority candidate `P104` (`Marcus Brody`, Urgency 1, sequence #4).
  6. Dispatch assignment: Matches `P104` at Node 1 to `GURNEY-03` at Node 1 (pickup cost: 0s, transport cost to Node 2: 100s).
  7. Simulation Step 1: `GURNEY-03` reaches pickup point at Node 1 in 0s. Patient state: `IN_TRANSIT`.
  8. Simulation Step 2: `GURNEY-03` transports `P104` to Node 2 in 100s. Clock advances to 165s. Patient state: `UNDER_TREATMENT`. `GURNEY-03` released to `AVAILABLE` at Node 2.
  9. Clinical discharge: `P104` discharged to terminal `DISCHARGED` state.
- **Cross-Service Integrity Check:** `validateSystemIntegrity()` verified healthy after every lifecycle transition.
- **Completion Banner:** `[DEMO COMPLETE] Noninteractive demonstration completed and self-validated successfully.`
- **Program Exit Code:** `0` (Success).

---

### 5. Command-Line Error Handling & Argument Parsing Verification

Strict command-line parsing rules were tested across edge cases:

| Test Scenario | Executed Command | Expected Behavior | Actual Observation | Result |
| :--- | :--- | :--- | :--- | :--- |
| **Missing `--data-dir` argument** | `SmartHospitalSystem.exe --data-dir` | Display error requiring path; exit nonzero | `[CLI ERROR] Flag '--data-dir' specified without following path argument.` Exit code: 1 | **PASS** |
| **Repeated `--data-dir` flag** | `SmartHospitalSystem.exe --data-dir data/ --data-dir other/` | Reject conflicting/repeated flags; exit nonzero | `[CLI ERROR] Duplicate or conflicting data-directory flag encountered.` Exit code: 1 | **PASS** |
| **Conflicting flag and positional** | `SmartHospitalSystem.exe --data-dir data/ other/` | Reject conflicting data paths; exit nonzero | `[CLI ERROR] Conflicting data-directory specified: 'other/' conflicts with '--data-dir'.` Exit code: 1 | **PASS** |
| **Multiple positional arguments** | `SmartHospitalSystem.exe data/ extra_dir/` | Reject extraneous arguments; exit nonzero | `[CLI ERROR] Multiple positional data-directory arguments provided.` Exit code: 1 | **PASS** |
| **Unknown command-line flag** | `SmartHospitalSystem.exe --invalid-flag` | Display unknown flag error and usage; exit nonzero | `[CLI ERROR] Unknown or unsupported command-line argument: '--invalid-flag'.` Exit code: 1 | **PASS** |
| **Help flag** | `SmartHospitalSystem.exe --help` | Display full CLI usage manual; exit zero | Usage manual displayed. Exit code: 0 | **PASS** |
| **Quoted path with spaces** | `SmartHospitalSystem.exe --demo "data"` | Accept quoted paths cleanly; exit zero | Seed files loaded successfully. Exit code: 0 | **PASS** |

---

### 6. External Directory Execution Verification

The executable was verified by launching it from an external working directory (`C:\Users\kpvp0\Downloads`) using absolute executable and data paths:

**Execution Command:**
```powershell
& "c:\Users\kpvp0\Downloads\Kashish College\2ND Year\Sem1\DS\CP\bin\SmartHospitalSystem.exe" --demo "c:\Users\kpvp0\Downloads\Kashish College\2ND Year\Sem1\DS\CP\data"
```

**Result:**
- Path resolution succeeded.
- All seed files were located and parsed.
- Self-validation completed with exit code `0`.

---

### 7. Interactive Workflow & Operational Integrity Checks

The interactive console application was validated using automated stream feeding covering all system workflows:

1. **Patient Registration & Lookup:** Verified registration of `P501` (`Jonathan Wick`), duplicate rejection, and dossier formatting.
2. **Emergency Triage & Acuity Updating:** Verified that updating patient `P108` (`Natasha Romanoff`) from Urgency 3 to 2 preserves arrival sequence `#8` for FIFO tie-breaking.
3. **Outpatient Clinic Queue:** Verified FIFO consult turns for `P102` (`Arthur Dent`), appointment cancellation for `P111` (`Walter White`), and condition deterioration conversion for `P106` (`Diana Prince`) into the emergency triage min-heap.
4. **2-Leg Dispatch Operations:** Verified priority candidate selection of `P104` (`Marcus Brody`), assignment of `GURNEY-03`, whole-leg transit clock advancement (+100s to 165s), immediate resource release at Node 2, and terminal discharge decoupling.
5. **Campus Routing & Corridor Closures:** Verified Dijkstra shortest path (270s via `0 -> 1 -> 5 -> 3`), dynamic rerouting when corridor (1, 5) is blocked (detour via `0 -> 1 -> 7 -> 3`, cost 330s), corridor reopening, and Kruskal vs Prim MST calculation (1280s).
6. **Patient Reports:** Verified that sorting on independent value snapshots leaves the live hospital registry, triage heap, and simulation clock 100% unaltered.
7. **Input Validation:** Verified that malformed numbers, out-of-range values, invalid characters, and empty inputs are rejected with informative messages without program termination.
8. **Stream EOF Termination:** Verified that reaching EOF terminates the application cleanly without unhandled exceptions.

A complete transcript of this executed walkthrough is preserved in [docs/demo_transcript.txt](file:///c:/Users/kpvp0/Downloads/Kashish%20College/2ND%20Year/Sem1/DS/CP/docs/demo_transcript.txt).

---

### 8. Unverified Capabilities & Known Limitations

To maintain academic rigor and audit honesty, the following items are explicitly marked as **unverified**:

1. **CMake / CTest Build Pipeline:**
   - *Status:* **UNVERIFIED on current Windows host**.
   - *Rationale:* CMake and CTest were not invoked during testing because native MinGW GCC batch scripts were used. `CMakeLists.txt` is provided for standard multi-platform support, but its build configuration on this specific host remains unverified.
2. **Alternative Compilers (Clang, MSVC, GCC 11+):**
   - *Status:* **UNVERIFIED**.
   - *Rationale:* Only MinGW GCC 6.3.0 is installed on the testing host. Cross-compiler warnings or dialect discrepancies on Clang or MSVC have not been evaluated.
3. **Automated Dynamic Memory Diagnostics (Valgrind / AddressSanitizer):**
   - *Status:* **UNVERIFIED**.
   - *Rationale:* Valgrind is unsupported on native Windows. AddressSanitizer is unavailable in MinGW GCC 6.3.0. Although all data structures implement strict RAII, non-allocating rollbacks, and explicit destructuring, memory safety cannot be inferred solely from passing functional tests.
4. **Machine-Specific MinGW Script Limitation:**
   - *Status:* **DOCUMENTED LIMITATION**.
   - *Rationale:* `build_and_test.bat` explicitly references `C:\MinGW\bin\g++.exe`. On host machines where MinGW is installed in a different directory or where `g++` is in the system `PATH`, the script must be adjusted or invoked via `g++` directly.
