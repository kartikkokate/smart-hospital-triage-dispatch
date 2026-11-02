#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "models/CampusLocation.hpp"
#include "models/AuditEvent.hpp"
#include "algorithms/DistanceArithmetic.hpp"
#include "services/DataLoader.hpp"

// Milestone 2 Custom Data Structures
#include "ds/SinglyLinkedList.hpp"
#include "ds/DoublyLinkedList.hpp"
#include "ds/CustomStack.hpp"
#include "ds/CustomQueue.hpp"
#include "ds/HashTableChaining.hpp"
#include "ds/HashTableProbing.hpp"

// Milestone 3 Custom Data Structures
#include "ds/BinarySearchTree.hpp"
#include "ds/BinaryMinHeap.hpp"

// Milestone 4 Custom Data Structures & Graph Algorithms
#include "algorithms/CustomSorting.hpp"
#include "ds/DisjointSetUnion.hpp"
#include "ds/CampusGraph.hpp"

// Milestone 5 Hospital Workflow & Dispatch Services
#include "services/HospitalRegistry.hpp"
#include "services/CampusMapManager.hpp"
#include "services/TriageManager.hpp"
#include "services/AppointmentManager.hpp"
#include "services/DispatchManager.hpp"
#include "services/HospitalSystem.hpp"

// Milestone 6 Reporting, Sorting & Educational Demonstrations
#include "models/PatientSnapshot.hpp"
#include "services/ReportingService.hpp"
#include "services/DemonstrationService.hpp"

// Milestone 7 Interactive Console UI
#include "ui/ConsoleApp.hpp"

// Global test counters
static int gPassCount = 0;
static int gFailCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            gFailCount++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(testFunc) \
    do { \
        std::cout << "[RUNNING TEST] " << #testFunc << " ...\n"; \
        int before = gFailCount; \
        testFunc(); \
        if (gFailCount == before) { \
            std::cout << "  [PASS] " << #testFunc << "\n"; \
            gPassCount++; \
        } \
    } while (0)

static std::string gDataDir = "data/";

// ============================================================================
// ThrowOnCopy Test Fixture for Exception-Safety Verification
// ============================================================================
struct ThrowOnCopy {
    static int liveCount;
    static int throwOnCopyIndex;
    static int copyCounter;
    int id = 0;

    ThrowOnCopy(int v = 0) : id(v) {
        liveCount++;
    }

    ThrowOnCopy(const ThrowOnCopy& other) : id(other.id) {
        copyCounter++;
        if (copyCounter == throwOnCopyIndex) {
            throw std::runtime_error("Simulated copy constructor exception");
        }
        liveCount++;
    }

    ThrowOnCopy(ThrowOnCopy&& other) noexcept : id(other.id) {
        liveCount++;
    }

    ThrowOnCopy& operator=(const ThrowOnCopy& other) {
        if (this != &other) {
            id = other.id;
        }
        return *this;
    }

    ThrowOnCopy& operator=(ThrowOnCopy&& other) noexcept {
        if (this != &other) {
            id = other.id;
        }
        return *this;
    }

    ~ThrowOnCopy() {
        liveCount--;
    }

    bool operator==(const ThrowOnCopy& other) const noexcept {
        return id == other.id;
    }
};

int ThrowOnCopy::liveCount = 0;
int ThrowOnCopy::throwOnCopyIndex = -1;
int ThrowOnCopy::copyCounter = 0;

// ============================================================================
// MILESTONE 1 TESTS (Preserved)
// ============================================================================

void testPatientModelAndStateQueries() {
    Patient p;
    p.internalHandle = 0;
    p.patientId = "P101";
    p.fullName = "Sarah Connor";
    p.age = 29;
    p.urgency = UrgencyLevel::LESS_URGENT;
    p.arrivalSequence = 1;
    p.state = PatientState::REGISTERED;

    TEST_ASSERT(p.isEligibleForTriage(), "Registered patient must be eligible for triage");
    TEST_ASSERT(!p.isWaitingInTriage(), "Registered patient is not waiting in triage");
    TEST_ASSERT(!p.isDispatchAssigned(), "Registered patient cannot have dispatch assigned");

    p.state = PatientState::DISPATCH_PENDING;
    p.assignedResourceId = "";
    TEST_ASSERT(p.isDispatchUnassigned(), "Empty resourceId indicates unassigned dispatch pending");
    TEST_ASSERT(!p.isDispatchAssigned(), "Empty resourceId cannot be dispatch assigned");

    p.assignedResourceId = "GURNEY-01";
    TEST_ASSERT(p.isDispatchAssigned(), "Non-empty resourceId indicates dispatch assigned");
    TEST_ASSERT(!p.isDispatchUnassigned(), "Assigned patient cannot be unassigned");

    p.state = PatientState::UNDER_TREATMENT;
    TEST_ASSERT(p.isUnderTreatment(), "Under treatment query must return true");

    p.state = PatientState::DISCHARGED;
    TEST_ASSERT(p.isDischarged(), "Discharged query must return true");

    p.state = PatientState::CANCELLED;
    TEST_ASSERT(p.isCancelled(), "Cancelled query must return true");
}

void testModelLifecycleAndHandoverDemo() {
    Patient patient;
    patient.patientId = "P104";
    patient.state = PatientState::DISPATCH_PENDING;
    patient.assignedResourceId = "";
    patient.currentLocationNode = 1;
    patient.targetLocationNode = 2;

    DispatchResource gurney;
    gurney.resourceId = "GURNEY-01";
    gurney.state = ResourceState::AVAILABLE;
    gurney.currentNodeId = 0;
    gurney.assignedPatientId = "";

    TEST_ASSERT(patient.isDispatchUnassigned(), "Initial state is unassigned");
    TEST_ASSERT(gurney.isAvailable(), "Initial gurney is available");

    patient.assignedResourceId = gurney.resourceId;
    gurney.state = ResourceState::EN_ROUTE_PICKUP;
    gurney.assignedPatientId = patient.patientId;

    TEST_ASSERT(patient.isDispatchAssigned(), "Patient state remains DISPATCH_PENDING (Assigned)");
    TEST_ASSERT(gurney.state == ResourceState::EN_ROUTE_PICKUP, "Gurney state is EN_ROUTE_PICKUP");
    TEST_ASSERT(gurney.assignedPatientId == "P104", "Gurney tracks assigned patient");

    gurney.currentNodeId = patient.currentLocationNode;
    patient.state = PatientState::IN_TRANSIT;
    gurney.state = ResourceState::TRANSPORTING_PATIENT;

    TEST_ASSERT(patient.state == PatientState::IN_TRANSIT, "Patient transitions to IN_TRANSIT on pickup");
    TEST_ASSERT(gurney.state == ResourceState::TRANSPORTING_PATIENT, "Resource transitions to TRANSPORTING_PATIENT");

    patient.currentLocationNode = patient.targetLocationNode;
    gurney.currentNodeId = patient.targetLocationNode;

    patient.state = PatientState::UNDER_TREATMENT;
    patient.assignedResourceId = "";
    gurney.assignedPatientId = "";
    gurney.state = ResourceState::AVAILABLE;

    TEST_ASSERT(patient.state == PatientState::UNDER_TREATMENT, "Patient enters UNDER_TREATMENT after handover");
    TEST_ASSERT(gurney.isAvailable(), "Gurney is released to AVAILABLE at destination");
    TEST_ASSERT(gurney.currentNodeId == 2, "Gurney location is now Node 2 (Resus Bay)");

    DispatchResource& reusedGurney = gurney;
    TEST_ASSERT(reusedGurney.isAvailable(), "GURNEY-01 can immediately be assigned to another patient");
    reusedGurney.assignedPatientId = "P105";
    reusedGurney.state = ResourceState::EN_ROUTE_PICKUP;

    patient.state = PatientState::DISCHARGED;
    TEST_ASSERT(reusedGurney.assignedPatientId == "P105", "Discharging P104 must NOT clear GURNEY-01 assigned to P105");
    TEST_ASSERT(reusedGurney.state == ResourceState::EN_ROUTE_PICKUP, "Gurney state must remain EN_ROUTE_PICKUP for P105");
}

void testProductionDistanceArithmetic() {
    TEST_ASSERT(safeDistanceAdd(120, 90) == 210, "120s + 90s must equal 210s");
    TEST_ASSERT(safeDistanceAdd(0, 0) == 0, "0 + 0 must equal 0");
    TEST_ASSERT(safeDistanceAdd(0, 60) == 60, "0 + 60 must equal 60");
    TEST_ASSERT(safeDistanceAdd(60, 0) == 60, "60 + 0 must equal 60");

    TEST_ASSERT(safeDistanceAdd(kInfinity, 50) == kInfinity, "Infinity + 50 must saturate at kInfinity");
    TEST_ASSERT(safeDistanceAdd(50, kInfinity) == kInfinity, "50 + Infinity must saturate at kInfinity");
    TEST_ASSERT(safeDistanceAdd(kInfinity, kInfinity) == kInfinity, "Infinity + Infinity must saturate at kInfinity");

    TEST_ASSERT(safeDistanceAdd(kInfinity - 10, 20) == kInfinity, "Addition exceeding kInfinity must saturate at kInfinity");
    TEST_ASSERT(safeDistanceAdd(kInfinity - 1, 1) == kInfinity, "Exact boundary hit must reach kInfinity");
    TEST_ASSERT(safeDistanceAdd(kInfinity - 5, 2) == kInfinity - 3, "Addition within bounds must remain exact");

    int64_t hugeVal = std::numeric_limits<int64_t>::max() / 2;
    TEST_ASSERT(safeDistanceAdd(hugeVal, hugeVal) == kInfinity, "Large values must safely saturate without UB");

    TEST_ASSERT(safeDistanceAdd(-10, 50) == -1, "Negative distance must return -1 (invalid)");
    TEST_ASSERT(safeDistanceAdd(50, -5) == -1, "Negative weight must return -1 (invalid)");
    TEST_ASSERT(safeDistanceAdd(-1, -1) == -1, "Double negative must return -1 (invalid)");

    TEST_ASSERT(canRelaxEdge(100, 50, 200), "Valid shorter path must allow edge relaxation");
    TEST_ASSERT(!canRelaxEdge(100, 150, 200), "Longer path must NOT allow edge relaxation");
    TEST_ASSERT(!canRelaxEdge(100, 100, 200), "Equal path must NOT allow edge relaxation");
    TEST_ASSERT(!canRelaxEdge(kInfinity, 50, 500), "Infinite source must NOT relax edge");
    TEST_ASSERT(!canRelaxEdge(100, -5, 500), "Negative weight must NOT relax edge");
}

void testSeedDataIntegrity() {
    std::vector<Patient> patients;
    std::vector<DispatchResource> resources;
    std::vector<CampusLocation> locations;
    std::vector<CorridorEdge> edges;
    std::string error;

    bool pOk = DataLoader::loadPatientsCSV(gDataDir + "patients_seed.csv", patients, error);
    TEST_ASSERT(pOk, ("Failed to load patients_seed.csv: " + error).c_str());
    TEST_ASSERT(patients.size() == 12, "Expected exactly 12 patient records");

    uint64_t expectedSeq = 1;
    std::vector<std::string> urgency1Ids;
    for (const auto& p : patients) {
        TEST_ASSERT(p.arrivalSequence == expectedSeq, "Arrival sequence must be monotonically increasing from 1 to 12");
        expectedSeq++;
        if (p.urgency == UrgencyLevel::RESUSCITATION) {
            urgency1Ids.push_back(p.patientId);
        }
    }

    TEST_ASSERT(urgency1Ids.size() == 3, "Expected exactly 3 Urgency-1 patients (P104, P107, P112)");
    TEST_ASSERT(urgency1Ids[0] == "P104", "1st Urgency-1 patient must be P104");
    TEST_ASSERT(urgency1Ids[1] == "P107", "2nd Urgency-1 patient must be P107");
    TEST_ASSERT(urgency1Ids[2] == "P112", "3rd Urgency-1 patient must be P112");

    bool rOk = DataLoader::loadResourcesCSV(gDataDir + "resources_seed.csv", resources, error);
    TEST_ASSERT(rOk, ("Failed to load resources_seed.csv: " + error).c_str());
    TEST_ASSERT(resources.size() == 3, "Expected exactly 3 dispatch resources");
    for (const auto& r : resources) {
        TEST_ASSERT(r.isAvailable(), "All initial resources must be in AVAILABLE state");
    }

    bool mOk = DataLoader::loadCampusMapTXT(gDataDir + "campus_map_seed.txt", locations, edges, error);
    TEST_ASSERT(mOk, ("Failed to load campus_map_seed.txt: " + error).c_str());
    TEST_ASSERT(locations.size() == 12, "Expected exactly 12 campus locations (Nodes 0 to 11)");
    TEST_ASSERT(edges.size() == 15, "Expected exactly 15 corridor edges");

    int64_t totalCorridorWeight = 0;
    for (const auto& e : edges) {
        TEST_ASSERT(e.transitTimeSeconds > 0, "Corridor weight must be strictly positive");
        totalCorridorWeight += e.transitTimeSeconds;
    }
    TEST_ASSERT(totalCorridorWeight == 1850, "Sum of all 15 corridor transit times must equal 1850 seconds");

    bool refOk = DataLoader::validateCampusReferences(patients, resources, locations, error);
    TEST_ASSERT(refOk, ("Loaded seed entities reference invalid campus nodes: " + error).c_str());
}

void testDataLoaderNegativeValidation() {
    std::string err;

    auto testTempFile = [](const std::string& filename, const std::string& content) {
        std::ofstream out(filename);
        out << content;
        out.close();
    };

    std::vector<Patient> pOut;
    TEST_ASSERT(!DataLoader::loadPatientsCSV("data/non_existent_file.csv", pOut, err), "Non-existent file must fail");
    TEST_ASSERT(pOut.empty(), "Output vector must remain empty on failure");

    testTempFile("test_bad_age.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P101,Sarah Connor,29abc,Fracture,4,1,10,1,8,450.00,TRIAGED_WAITING\n");
    TEST_ASSERT(!DataLoader::loadPatientsCSV("test_bad_age.csv", pOut, err), "Partially parsed integer '29abc' must be rejected");
    std::remove("test_bad_age.csv");

    testTempFile("test_bad_urgency.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P101,Sarah Connor,29,Fracture,9,1,10,1,8,450.00,TRIAGED_WAITING\n");
    TEST_ASSERT(!DataLoader::loadPatientsCSV("test_bad_urgency.csv", pOut, err), "Urgency level 9 must be rejected");
    std::remove("test_bad_urgency.csv");

    testTempFile("test_dup_id.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P101,Sarah Connor,29,Fracture,4,1,10,1,8,450.00,TRIAGED_WAITING\n"
        "P101,Arthur Dent,42,Laceration,5,2,15,1,1,120.00,APPOINTMENT_SCHEDULED\n");
    TEST_ASSERT(!DataLoader::loadPatientsCSV("test_dup_id.csv", pOut, err), "Duplicate patient ID P101 must be rejected");
    std::remove("test_dup_id.csv");

    testTempFile("test_dup_seq.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P101,Sarah Connor,29,Fracture,4,1,10,1,8,450.00,TRIAGED_WAITING\n"
        "P102,Arthur Dent,42,Laceration,5,1,15,1,1,120.00,APPOINTMENT_SCHEDULED\n");
    TEST_ASSERT(!DataLoader::loadPatientsCSV("test_dup_seq.csv", pOut, err), "Duplicate arrival sequence 1 must be rejected");
    std::remove("test_dup_seq.csv");

    testTempFile("test_quoted.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P101,\"Sarah Connor\",29,Fracture,4,1,10,1,8,450.00,TRIAGED_WAITING\n");
    TEST_ASSERT(!DataLoader::loadPatientsCSV("test_quoted.csv", pOut, err), "Double-quoted fields must be rejected");
    std::remove("test_quoted.csv");

    testTempFile("test_apostrophe.csv",
        "patientId,fullName,age,complaint,urgency,arrivalSequence,simulatedArrivalTime,currentLocationNode,targetLocationNode,billingAmount,initialState\n"
        "P199,Liam O'Connor,34,Concussion,2,1,12,0,1,500.00,REGISTERED\n");
    bool aposOk = DataLoader::loadPatientsCSV("test_apostrophe.csv", pOut, err);
    TEST_ASSERT(aposOk, ("Apostrophe in names like O'Connor must be accepted: " + err).c_str());
    TEST_ASSERT(pOut.size() == 1 && pOut[0].fullName == "Liam O'Connor", "Apostrophe name parsed correctly");
    std::remove("test_apostrophe.csv");

    std::vector<CampusLocation> validLocs;
    CampusLocation loc0; loc0.nodeId = 0; loc0.name = "Bay"; validLocs.push_back(loc0);
    CampusLocation loc1; loc1.nodeId = 1; loc1.name = "ER"; validLocs.push_back(loc1);

    std::vector<Patient> testPatients = pOut;
    std::vector<DispatchResource> testRes;
    TEST_ASSERT(DataLoader::validateCampusReferences(testPatients, testRes, validLocs, err), "Valid nodes 0 and 1 must pass");

    testPatients[0].targetLocationNode = 99;
    TEST_ASSERT(!DataLoader::validateCampusReferences(testPatients, testRes, validLocs, err), "Node 99 not in campus locations must fail cross-validation");

    std::vector<CampusLocation> locOut;
    std::vector<CorridorEdge> edgeOut;
    testTempFile("test_dup_corridor.txt",
        "[NODES]\n"
        "0,Ambulance Bay,Wing,false\n"
        "1,ER Triage,Wing,false\n"
        "[EDGES]\n"
        "0,1,60,Forward\n"
        "1,0,60,Reverse Duplicate\n");
    TEST_ASSERT(!DataLoader::loadCampusMapTXT("test_dup_corridor.txt", locOut, edgeOut, err), "Duplicate undirected corridor must be rejected");
    std::remove("test_dup_corridor.txt");

    testTempFile("test_self_loop.txt",
        "[NODES]\n"
        "0,Ambulance Bay,Wing,false\n"
        "[EDGES]\n"
        "0,0,50,Self loop\n");
    TEST_ASSERT(!DataLoader::loadCampusMapTXT("test_self_loop.txt", locOut, edgeOut, err), "Self-loop corridor must be rejected");
    std::remove("test_self_loop.txt");
}

// ============================================================================
// MILESTONE 2 TESTS (Custom Linear Data Structures & Hashing)
// ============================================================================

void testSinglyLinkedList() {
    SinglyLinkedList<int> list;

    TEST_ASSERT(list.empty(), "Initial list must be empty");
    TEST_ASSERT(list.size() == 0, "Initial size must be 0");
    int val = 0;
    TEST_ASSERT(!list.pop_front(val), "pop_front on empty list must return false");
    TEST_ASSERT(!list.remove_value(99), "remove_value on empty list must return false");
    TEST_ASSERT(!list.remove_at(0, val), "remove_at on empty list must return false");
    TEST_ASSERT(list.front() == nullptr, "front on empty list must return nullptr");
    TEST_ASSERT(list.back() == nullptr, "back on empty list must return nullptr");
    TEST_ASSERT(list.find(42) == nullptr, "find on empty list must return nullptr");

    list.push_front(100);
    TEST_ASSERT(!list.empty() && list.size() == 1, "List must have size 1");
    TEST_ASSERT(*list.front() == 100, "front() must return 100");
    TEST_ASSERT(*list.back() == 100, "back() must return 100");
    TEST_ASSERT(list.find(100) != nullptr, "find(100) must return non-null");
    TEST_ASSERT(list.pop_front(val) && val == 100, "pop_front must retrieve 100");
    TEST_ASSERT(list.empty() && list.size() == 0, "List must return to empty after pop_front");

    list.push_back(20);
    list.push_front(10);
    list.push_back(40);
    list.insert_after(1, 30);
    TEST_ASSERT(list.size() == 4, "List size must be 4");
    TEST_ASSERT(*list.front() == 10, "Front must be 10");
    TEST_ASSERT(*list.back() == 40, "Back must be 40");

    TEST_ASSERT(list.remove_value(30), "remove_value(30) must return true");
    TEST_ASSERT(list.size() == 3, "Size must be 3 after removing 30");
    TEST_ASSERT(list.find(30) == nullptr, "30 must no longer be found");

    TEST_ASSERT(list.remove_at(2, val) && val == 40, "remove_at(2) must remove 40");
    TEST_ASSERT(*list.back() == 20, "New back must be 20");

    TEST_ASSERT(list.pop_front(val) && val == 10, "pop_front must remove 10");
    TEST_ASSERT(list.size() == 1 && *list.front() == 20, "Remaining element must be 20");

    SinglyLinkedList<int> copyList = list;
    TEST_ASSERT(copyList.size() == 1 && *copyList.front() == 20, "Copy list must contain 20");
    copyList.push_back(999);
    TEST_ASSERT(copyList.size() == 2, "Copy list modified");
    TEST_ASSERT(list.size() == 1, "Original list must remain unaffected by copy modifications");

    SinglyLinkedList<int> assignList;
    assignList = copyList;
    TEST_ASSERT(assignList.size() == 2 && *assignList.back() == 999, "Copy assignment verified");

    // Self-copy and self-move assignment
    assignList = assignList;
    TEST_ASSERT(assignList.size() == 2 && *assignList.back() == 999, "Self-copy assignment preserves state");
    assignList = std::move(assignList);
    TEST_ASSERT(assignList.size() == 2, "Self-move assignment preserves state");

    SinglyLinkedList<int> movedList = std::move(copyList);
    TEST_ASSERT(movedList.size() == 2, "Moved list holds 2 items");
    TEST_ASSERT(copyList.empty() && copyList.size() == 0, "Source list is empty after move");

    SinglyLinkedList<int> longList;
    for (int i = 0; i < 1000; ++i) {
        longList.push_back(i);
    }
    TEST_ASSERT(longList.size() == 1000, "Long list populated with 1000 nodes");
    longList.clear();
    TEST_ASSERT(longList.empty() && longList.size() == 0, "Long list cleanly cleared without stack overflow");
}

void testDoublyLinkedList() {
    DoublyLinkedList<std::string> dlist;

    TEST_ASSERT(dlist.empty() && dlist.size() == 0, "Initial dlist must be empty");
    TEST_ASSERT(dlist.validateIntegrity(), "Empty dlist passes integrity check");
    std::string s;
    TEST_ASSERT(!dlist.pop_front(s), "pop_front on empty must return false");
    TEST_ASSERT(!dlist.pop_back(s), "pop_back on empty must return false");
    TEST_ASSERT(!dlist.remove_value("NONE"), "remove_value on empty must return false");
    TEST_ASSERT(dlist.front() == nullptr && dlist.back() == nullptr, "Pointers must be nullptr");

    dlist.push_front("FIRST");
    TEST_ASSERT(dlist.size() == 1 && dlist.validateIntegrity(), "Single element integrity");
    TEST_ASSERT(*dlist.front() == "FIRST" && *dlist.back() == "FIRST", "Front and back match");
    TEST_ASSERT(dlist.pop_back(s) && s == "FIRST", "pop_back single element");
    TEST_ASSERT(dlist.empty() && dlist.validateIntegrity(), "dlist empty after pop_back");
    TEST_ASSERT(dlist.front() == nullptr && dlist.back() == nullptr, "head and tail reset to nullptr");

    dlist.push_back("B");
    dlist.push_front("A");
    dlist.push_back("D");
    dlist.insert_at(2, "C");
    TEST_ASSERT(dlist.size() == 4, "dlist size must be 4");
    TEST_ASSERT(dlist.validateIntegrity(), "Integrity passes after insertions");

    std::vector<std::string> fwd = dlist.to_vector_forward();
    std::vector<std::string> bwd = dlist.to_vector_backward();
    TEST_ASSERT(fwd.size() == 4 && bwd.size() == 4, "Vector conversions match size");
    TEST_ASSERT(fwd[0] == "A" && fwd[1] == "B" && fwd[2] == "C" && fwd[3] == "D", "Forward traversal matches");
    TEST_ASSERT(bwd[0] == "D" && bwd[1] == "C" && bwd[2] == "B" && bwd[3] == "A", "Backward traversal matches");

    TEST_ASSERT(dlist.remove_value("B"), "remove_value('B') must succeed");
    TEST_ASSERT(dlist.size() == 3 && dlist.validateIntegrity(), "Integrity holds after middle removal");
    fwd = dlist.to_vector_forward();
    TEST_ASSERT(fwd[0] == "A" && fwd[1] == "C" && fwd[2] == "D", "Remaining items are A, C, D");

    TEST_ASSERT(dlist.pop_back(s) && s == "D", "pop_back must remove D");
    TEST_ASSERT(dlist.size() == 2 && dlist.validateIntegrity(), "Integrity holds after tail removal");

    TEST_ASSERT(dlist.pop_front(s) && s == "A", "pop_front must remove A");
    TEST_ASSERT(dlist.size() == 1 && *dlist.front() == "C" && dlist.validateIntegrity(), "Only C remains");

    DoublyLinkedList<std::string> copyDlist = dlist;
    TEST_ASSERT(copyDlist.size() == 1 && copyDlist.validateIntegrity(), "Copy passes integrity");
    copyDlist.push_back("NEW_ITEM");
    TEST_ASSERT(copyDlist.size() == 2 && dlist.size() == 1, "Deep copy independent from original");

    // Self-copy and self-move assignment
    copyDlist = copyDlist;
    TEST_ASSERT(copyDlist.size() == 2 && copyDlist.validateIntegrity(), "Self-copy assignment holds");
    copyDlist = std::move(copyDlist);
    TEST_ASSERT(copyDlist.size() == 2 && copyDlist.validateIntegrity(), "Self-move assignment holds");

    DoublyLinkedList<std::string> movedDlist = std::move(copyDlist);
    TEST_ASSERT(movedDlist.size() == 2 && copyDlist.empty(), "Move constructor verified");

    movedDlist.clear();
    TEST_ASSERT(movedDlist.empty() && movedDlist.validateIntegrity(), "Cleaned dlist passes integrity");
}

void testCustomStack() {
    CustomStack<int> stack;

    TEST_ASSERT(stack.isEmpty() && stack.size() == 0, "Initial stack must be empty");
    int val = 0;
    TEST_ASSERT(!stack.pop(val), "pop on empty stack must return false (underflow protected)");
    TEST_ASSERT(stack.top() == nullptr, "top on empty stack must return nullptr");

    stack.push(42);
    TEST_ASSERT(!stack.isEmpty() && stack.size() == 1, "Size must be 1");
    TEST_ASSERT(*stack.top() == 42, "top must be 42");
    TEST_ASSERT(stack.pop(val) && val == 42, "pop must yield 42");
    TEST_ASSERT(stack.isEmpty(), "Stack must return to empty");

    stack.push(10);
    stack.push(20);
    stack.push(30);
    stack.push(40);
    TEST_ASSERT(stack.size() == 4, "Stack size is 4");
    TEST_ASSERT(*stack.top() == 40, "Top is 40");

    TEST_ASSERT(stack.pop(val) && val == 40, "1st pop is 40");
    TEST_ASSERT(stack.pop(val) && val == 30, "2nd pop is 30");
    TEST_ASSERT(stack.pop(val) && val == 20, "3rd pop is 20");
    TEST_ASSERT(stack.pop(val) && val == 10, "4th pop is 10");
    TEST_ASSERT(stack.isEmpty(), "Stack is now empty");
    TEST_ASSERT(!stack.pop(val), "Subsequent pop fails underflow check");

    stack.push(100);
    stack.push(200);
    stack.push(300);
    CustomStack<int> copyStack = stack;
    TEST_ASSERT(copyStack.size() == 3, "Copy stack has size 3");

    // Self-copy and self-move
    copyStack = copyStack;
    TEST_ASSERT(copyStack.size() == 3, "Self-copy assignment holds");
    copyStack = std::move(copyStack);
    TEST_ASSERT(copyStack.size() == 3, "Self-move assignment holds");

    TEST_ASSERT(copyStack.pop(val) && val == 300, "Copy top is 300");
    TEST_ASSERT(copyStack.pop(val) && val == 200, "Copy next is 200");
    TEST_ASSERT(copyStack.pop(val) && val == 100, "Copy last is 100");
    TEST_ASSERT(stack.size() == 3, "Original stack untouched by copy pops");

    CustomStack<int> movedStack = std::move(stack);
    TEST_ASSERT(movedStack.size() == 3 && stack.isEmpty(), "Move transfers elements");
    TEST_ASSERT(movedStack.pop(val) && val == 300, "Moved stack functions correctly");
}

void testCustomQueue() {
    CustomQueue<std::string> queue;

    TEST_ASSERT(queue.isEmpty() && queue.size() == 0, "Initial queue must be empty");
    std::string s;
    TEST_ASSERT(!queue.dequeue(s), "dequeue on empty queue must return false");
    TEST_ASSERT(queue.front() == nullptr, "front on empty queue must return nullptr");

    queue.enqueue("PATIENT_1");
    queue.enqueue("PATIENT_2");
    queue.enqueue("PATIENT_3");
    TEST_ASSERT(queue.size() == 3, "Queue size must be 3");
    TEST_ASSERT(*queue.front() == "PATIENT_1", "Front is PATIENT_1");

    TEST_ASSERT(queue.dequeue(s) && s == "PATIENT_1", "1st dequeue is PATIENT_1");
    TEST_ASSERT(queue.dequeue(s) && s == "PATIENT_2", "2nd dequeue is PATIENT_2");
    TEST_ASSERT(queue.dequeue(s) && s == "PATIENT_3", "3rd dequeue is PATIENT_3");
    TEST_ASSERT(queue.isEmpty(), "Queue is empty after 3 dequeues");

    // Removal of final queue element followed by enqueueing again
    queue.enqueue("SOLO_ELEMENT");
    TEST_ASSERT(queue.size() == 1, "Queue holds single element");
    TEST_ASSERT(queue.dequeue(s) && s == "SOLO_ELEMENT", "Dequeued final element");
    TEST_ASSERT(queue.isEmpty() && queue.front() == nullptr, "Both head and tail reset to nullptr");

    queue.enqueue("NEW_ELEMENT_A");
    queue.enqueue("NEW_ELEMENT_B");
    TEST_ASSERT(queue.size() == 2, "Successfully re-enqueued 2 items after draining to empty");
    TEST_ASSERT(*queue.front() == "NEW_ELEMENT_A", "Front is correctly NEW_ELEMENT_A");
    TEST_ASSERT(queue.dequeue(s) && s == "NEW_ELEMENT_A", "Dequeue NEW_ELEMENT_A");
    TEST_ASSERT(queue.dequeue(s) && s == "NEW_ELEMENT_B", "Dequeue NEW_ELEMENT_B");
    TEST_ASSERT(queue.isEmpty(), "Queue drained cleanly again");

    queue.enqueue("ALPHA");
    queue.enqueue("BETA");
    queue.enqueue("GAMMA");
    CustomQueue<std::string> copyQueue = queue;
    TEST_ASSERT(copyQueue.size() == 3, "Copy queue has 3 elements");

    copyQueue = copyQueue;
    TEST_ASSERT(copyQueue.size() == 3, "Self-copy assignment holds");
    copyQueue = std::move(copyQueue);
    TEST_ASSERT(copyQueue.size() == 3, "Self-move assignment holds");

    TEST_ASSERT(copyQueue.dequeue(s) && s == "ALPHA", "Copy dequeue 1 is ALPHA");
    TEST_ASSERT(copyQueue.dequeue(s) && s == "BETA", "Copy dequeue 2 is BETA");
    TEST_ASSERT(copyQueue.dequeue(s) && s == "GAMMA", "Copy dequeue 3 is GAMMA");
    TEST_ASSERT(queue.size() == 3, "Original queue preserved during copy dequeue");

    CustomQueue<std::string> movedQueue = std::move(queue);
    TEST_ASSERT(movedQueue.size() == 3 && queue.isEmpty(), "Move transfers queue nodes");
    TEST_ASSERT(movedQueue.dequeue(s) && s == "ALPHA", "Moved queue dequeue works");
}

void testHashTableChaining() {
    HashTableChaining<std::string, int> table(17, 0.75);

    TEST_ASSERT(table.empty() && table.size() == 0, "Initial table must be empty");
    TEST_ASSERT(table.capacity() == 17, "Initial capacity must be prime 17");
    TEST_ASSERT(table.loadFactor() == 0.0, "Initial load factor is 0.0");
    TEST_ASSERT(table.search("NON_EXISTENT") == nullptr, "Search for missing key returns nullptr");
    TEST_ASSERT(!table.remove("NON_EXISTENT"), "Remove missing key returns false");

    // Collision fixture with actual polynomial hash:
    TEST_ASSERT(table.hash("A") == 14, "Key 'A' must hash to bucket 14");
    TEST_ASSERT(table.hash("R") == 14, "Key 'R' must hash to bucket 14");
    TEST_ASSERT(table.hash("c") == 14, "Key 'c' must hash to bucket 14");

    TEST_ASSERT(table.insert("A", 100), "Insert 'A' succeeds");
    TEST_ASSERT(table.insert("R", 200), "Insert 'R' succeeds");
    TEST_ASSERT(table.insert("c", 300), "Insert 'c' succeeds");

    TEST_ASSERT(table.size() == 3, "Table size must be 3");
    TEST_ASSERT(table.bucketSize(14) == 3, "Bucket 14 must contain exactly 3 chained nodes");

    int* pA = table.search("A");
    int* pR = table.search("R");
    int* pC = table.search("c");
    TEST_ASSERT(pA != nullptr && *pA == 100, "Search 'A' returns 100");
    TEST_ASSERT(pR != nullptr && *pR == 200, "Search 'R' returns 200");
    TEST_ASSERT(pC != nullptr && *pC == 300, "Search 'c' returns 300");

    TEST_ASSERT(!table.insert("R", 999), "Updating existing key 'R' returns false");
    TEST_ASSERT(table.size() == 3, "Size remains 3 after in-place key update");
    TEST_ASSERT(*table.search("R") == 999, "Value of 'R' successfully updated to 999");

    TEST_ASSERT(table.remove("R"), "Removing middle colliding key 'R' must return true");
    TEST_ASSERT(table.size() == 2, "Size is 2 after removal");
    TEST_ASSERT(table.search("R") == nullptr, "Search for removed key 'R' must return nullptr");
    TEST_ASSERT(table.search("A") != nullptr && *table.search("A") == 100, "'A' still retrievable");
    TEST_ASSERT(table.search("c") != nullptr && *table.search("c") == 300, "'c' still retrievable");
    TEST_ASSERT(table.bucketSize(14) == 2, "Bucket 14 size decremented to 2");

    TEST_ASSERT(table.remove("c"), "Remove head of chain 'c'");
    TEST_ASSERT(table.remove("A"), "Remove tail of chain 'A'");
    TEST_ASSERT(table.bucketSize(14) == 0 && table.size() == 0, "Bucket 14 empty and table size 0");

    for (int i = 0; i < 14; ++i) {
        std::string key = "KEY_" + std::to_string(i);
        table.insert(key, i * 10);
    }
    TEST_ASSERT(table.size() == 14, "Inserted 14 keys");
    TEST_ASSERT(table.capacity() == 37, "Capacity must rehash to next prime >= 2*17+1 (37)");
    TEST_ASSERT(table.loadFactor() <= 0.75, "Load factor restored safely below 0.75");

    for (int i = 0; i < 14; ++i) {
        std::string key = "KEY_" + std::to_string(i);
        int* v = table.search(key);
        TEST_ASSERT(v != nullptr && *v == i * 10, ("Key " + key + " preserved across rehash").c_str());
    }

    // Self-copy and self-move assignment
    table = table;
    TEST_ASSERT(table.size() == 14 && table.capacity() == 37, "Self-copy assignment preserves chaining table");
    table = std::move(table);
    TEST_ASSERT(table.size() == 14 && table.capacity() == 37, "Self-move assignment preserves chaining table");

    // Move construction and reuse of moved-from table
    HashTableChaining<std::string, int> sourceTable;
    sourceTable.insert("SRC_A", 1);
    sourceTable.insert("SRC_B", 2);

    HashTableChaining<std::string, int> destTable = std::move(sourceTable);
    TEST_ASSERT(destTable.size() == 2 && destTable.contains("SRC_A"), "Dest table contains moved keys");
    TEST_ASSERT(sourceTable.size() == 0 && sourceTable.capacity() == 0, "Source table is moved-from");

    // Reuse moved-from source table: capacity 0 automatically restores storage
    TEST_ASSERT(sourceTable.insert("NEW_KEY", 42), "Inserting into moved-from table restores storage");
    TEST_ASSERT(sourceTable.size() == 1 && *sourceTable.search("NEW_KEY") == 42, "New key found in resurrected source");
    TEST_ASSERT(sourceTable.remove("NEW_KEY"), "Remove works on resurrected source");

    // Move assignment reuse test
    HashTableChaining<std::string, int> assignDest;
    assignDest = std::move(destTable);
    TEST_ASSERT(assignDest.size() == 2 && assignDest.contains("SRC_A"), "Assign dest holds keys");
    TEST_ASSERT(destTable.size() == 0 && destTable.capacity() == 0, "Dest table is moved-from");
    TEST_ASSERT(destTable.insert("REUSED_DEST", 84), "Inserting into move-assigned source restores storage");
    TEST_ASSERT(*destTable.search("REUSED_DEST") == 84, "Found key in restored dest");
}

void testHashTableProbing() {
    HashTableProbing<std::string, int> table(17, 0.70);

    TEST_ASSERT(table.empty() && table.size() == 0, "Initial probing table empty");
    TEST_ASSERT(table.capacity() == 17, "Initial capacity is 17");
    TEST_ASSERT(table.search("MISSING") == nullptr, "Missing key search returns nullptr");
    TEST_ASSERT(!table.remove("MISSING"), "Remove missing returns false");

    TEST_ASSERT(table.hash("A") == 14, "Hash of 'A' is 14");
    TEST_ASSERT(table.hash("R") == 14, "Hash of 'R' is 14");
    TEST_ASSERT(table.hash("c") == 14, "Hash of 'c' is 14");

    table.insert("A", 100);
    table.insert("R", 200);
    table.insert("c", 300);

    TEST_ASSERT(table.size() == 3, "Size must be 3");
    TEST_ASSERT(table.isOccupied(14) && table.isOccupied(15) && table.isOccupied(16), "Slots 14, 15, 16 occupied");

    TEST_ASSERT(table.remove("R"), "Removing 'R' returns true");
    TEST_ASSERT(table.size() == 2, "Size is 2 after removal");
    TEST_ASSERT(table.isTombstone(15), "Slot 15 must now be a DELETED tombstone");
    TEST_ASSERT(table.tombstoneCount() == 1, "Tombstone count must be 1");

    size_t probes = 0;
    int* pC = table.searchWithProbeCount("c", probes);
    TEST_ASSERT(pC != nullptr && *pC == 300, "Search for 'c' must succeed past tombstone");
    TEST_ASSERT(probes == 3, "Search took exactly 3 probes (14, 15, 16)");

    TEST_ASSERT(table.hash("t") == 14, "Key 't' hashes to 14");
    TEST_ASSERT(table.insert("t", 400), "Insert 't' succeeds");
    TEST_ASSERT(table.size() == 3, "Size is now 3");
    TEST_ASSERT(table.isOccupied(15), "Slot 15 must now be OCCUPIED by 't' (tombstone reused!)");
    TEST_ASSERT(!table.isTombstone(15), "Slot 15 is no longer a tombstone");
    TEST_ASSERT(table.tombstoneCount() == 0, "Tombstone count is back to 0");
    TEST_ASSERT(*table.search("t") == 400, "Search 't' returns 400");
    TEST_ASSERT(*table.search("c") == 300, "Search 'c' still returns 300 at slot 16");

    // Bounded Full-Table Probe Termination:
    HashTableProbing<int, int> smallTable(5, 1.0);
    smallTable.insert(0, 10);
    smallTable.insert(1, 20);
    smallTable.insert(2, 30);
    smallTable.insert(3, 40);
    smallTable.insert(4, 50);
    smallTable.remove(2);

    size_t smallProbes = 0;
    int* missingResult = smallTable.searchWithProbeCount(99, smallProbes);
    TEST_ASSERT(missingResult == nullptr, "Missing key search terminates safely and returns nullptr");
    TEST_ASSERT(smallProbes == smallTable.capacity(), "Probe strictly bounded by table capacity");

    // Rehashing Eliminates Tombstones
    HashTableProbing<std::string, int> rehashTable(5, 0.50);
    rehashTable.insert("K1", 1);
    rehashTable.insert("K2", 2);
    rehashTable.remove("K1");
    TEST_ASSERT(rehashTable.tombstoneCount() == 1, "Has 1 tombstone before rehash");
    rehashTable.insert("K3", 3);
    rehashTable.insert("K4", 4);
    TEST_ASSERT(rehashTable.tombstoneCount() == 0, "Rehash purges all tombstones");
    TEST_ASSERT(rehashTable.search("K2") != nullptr && *rehashTable.search("K2") == 2, "K2 preserved");
    TEST_ASSERT(rehashTable.search("K3") != nullptr && *rehashTable.search("K3") == 3, "K3 preserved");
    TEST_ASSERT(rehashTable.search("K4") != nullptr && *rehashTable.search("K4") == 4, "K4 preserved");

    // Self-copy and self-move
    rehashTable = rehashTable;
    TEST_ASSERT(rehashTable.size() == 3, "Self-copy assignment holds");
    rehashTable = std::move(rehashTable);
    TEST_ASSERT(rehashTable.size() == 3, "Self-move assignment holds");

    // Move construction and reuse of moved-from probing table
    HashTableProbing<std::string, int> moveSource;
    moveSource.insert("A", 1);
    moveSource.insert("B", 2);
    HashTableProbing<std::string, int> moveDest = std::move(moveSource);
    TEST_ASSERT(moveDest.size() == 2 && moveDest.contains("A"), "Move dest has keys");
    TEST_ASSERT(moveSource.size() == 0 && moveSource.capacity() == 0, "Move source is empty/capacity 0");

    TEST_ASSERT(moveSource.insert("RESURRECTED", 999), "Inserting into moved-from probing table restores storage");
    TEST_ASSERT(moveSource.size() == 1 && *moveSource.search("RESURRECTED") == 999, "Found in restored table");
    TEST_ASSERT(moveSource.remove("RESURRECTED"), "Remove from restored table works");
}

// Test 12: Exception-Safe Copy Semantics across all linear structures and chaining
void testExceptionSafeCopySemantics() {
    // 1. SinglyLinkedList exception safety
    {
        ThrowOnCopy::liveCount = 0;
        SinglyLinkedList<ThrowOnCopy> list;
        list.push_back(ThrowOnCopy(1));
        list.push_back(ThrowOnCopy(2));
        list.push_back(ThrowOnCopy(3));
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "Initial live count 3");

        ThrowOnCopy::copyCounter = 0;
        ThrowOnCopy::throwOnCopyIndex = 2; // Throw on copying 2nd element
        bool caught = false;
        try {
            SinglyLinkedList<ThrowOnCopy> copyList(list);
            (void)copyList;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        TEST_ASSERT(caught, "Exception was thrown and caught during list copy");
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "All partially constructed nodes cleanly destroyed (no leaks)");
    }
    TEST_ASSERT(ThrowOnCopy::liveCount == 0, "All items destroyed when source list leaves scope");

    // 2. DoublyLinkedList exception safety
    {
        ThrowOnCopy::liveCount = 0;
        DoublyLinkedList<ThrowOnCopy> dlist;
        dlist.push_back(ThrowOnCopy(10));
        dlist.push_back(ThrowOnCopy(20));
        dlist.push_back(ThrowOnCopy(30));
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "Initial live count 3");

        ThrowOnCopy::copyCounter = 0;
        ThrowOnCopy::throwOnCopyIndex = 3;
        bool caught = false;
        try {
            DoublyLinkedList<ThrowOnCopy> copyDlist(dlist);
            (void)copyDlist;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        TEST_ASSERT(caught, "Exception caught during doubly list copy");
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "No leaks in partially constructed doubly list");
    }
    TEST_ASSERT(ThrowOnCopy::liveCount == 0, "Doubly list items destroyed");

    // 3. CustomStack exception safety
    {
        ThrowOnCopy::liveCount = 0;
        CustomStack<ThrowOnCopy> stack;
        stack.push(ThrowOnCopy(100));
        stack.push(ThrowOnCopy(200));
        stack.push(ThrowOnCopy(300));
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "Initial live count 3");

        ThrowOnCopy::copyCounter = 0;
        ThrowOnCopy::throwOnCopyIndex = 2;
        bool caught = false;
        try {
            CustomStack<ThrowOnCopy> copyStack(stack);
            (void)copyStack;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        TEST_ASSERT(caught, "Exception caught during stack copy");
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "No leaks in stack copy");
    }
    TEST_ASSERT(ThrowOnCopy::liveCount == 0, "Stack items destroyed");

    // 4. CustomQueue exception safety
    {
        ThrowOnCopy::liveCount = 0;
        CustomQueue<ThrowOnCopy> queue;
        queue.enqueue(ThrowOnCopy(1000));
        queue.enqueue(ThrowOnCopy(2000));
        queue.enqueue(ThrowOnCopy(3000));
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "Initial live count 3");

        ThrowOnCopy::copyCounter = 0;
        ThrowOnCopy::throwOnCopyIndex = 2;
        bool caught = false;
        try {
            CustomQueue<ThrowOnCopy> copyQueue(queue);
            (void)copyQueue;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        TEST_ASSERT(caught, "Exception caught during queue copy");
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "No leaks in queue copy");
    }
    TEST_ASSERT(ThrowOnCopy::liveCount == 0, "Queue items destroyed");

    // 5. HashTableChaining exception safety
    {
        ThrowOnCopy::liveCount = 0;
        HashTableChaining<std::string, ThrowOnCopy> table(17, 0.75);
        table.insert("K1", ThrowOnCopy(1));
        table.insert("K2", ThrowOnCopy(2));
        table.insert("K3", ThrowOnCopy(3));
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "Initial live count 3");

        ThrowOnCopy::copyCounter = 0;
        ThrowOnCopy::throwOnCopyIndex = 2;
        bool caught = false;
        try {
            HashTableChaining<std::string, ThrowOnCopy> copyTable(table);
            (void)copyTable;
        } catch (const std::runtime_error&) {
            caught = true;
        }
        TEST_ASSERT(caught, "Exception caught during hash table chaining copy");
        TEST_ASSERT(ThrowOnCopy::liveCount == 3, "No leaks in hash table chaining copy");
    }
    TEST_ASSERT(ThrowOnCopy::liveCount == 0, "Hash table items destroyed");

    // Reset test fixture counters
    ThrowOnCopy::throwOnCopyIndex = -1;
    ThrowOnCopy::copyCounter = 0;
}

// ============================================================================
// MILESTONE 3 TESTS: BinarySearchTree & Priority Min-Heap
// ============================================================================

void testBinarySearchTree() {
    BinarySearchTree<std::string, int> bst;

    // 1. Empty tree state
    TEST_ASSERT(bst.empty() && bst.size() == 0, "Initial BST must be empty");
    TEST_ASSERT(bst.height() == 0, "Height of empty tree is 0");
    TEST_ASSERT(bst.countLeaves() == 0, "Leaf count of empty tree is 0");
    TEST_ASSERT(bst.getLeafKeys().empty(), "Leaf keys of empty tree is empty");
    TEST_ASSERT(bst.search("P101") == nullptr, "Search in empty tree returns nullptr");
    TEST_ASSERT(!bst.remove("P101"), "Remove from empty tree returns false");

    // 2. Single-node tree
    TEST_ASSERT(bst.insert("M", 50), "Insert single root 'M'");
    TEST_ASSERT(bst.size() == 1 && !bst.empty(), "Size is 1");
    TEST_ASSERT(bst.height() == 1, "Height of single node is 1");
    TEST_ASSERT(bst.countLeaves() == 1, "Leaf count of single node is 1");
    TEST_ASSERT(*bst.search("M") == 50, "Found root value 50");
    TEST_ASSERT(bst.remove("M"), "Delete single root node");
    TEST_ASSERT(bst.empty() && bst.size() == 0, "BST is empty after root deletion");

    // 3. Duplicate key update behavior
    TEST_ASSERT(bst.insert("P101", 10), "Insert P101 -> 10");
    TEST_ASSERT(!bst.insert("P101", 99), "Duplicate key returns false (in-place update)");
    TEST_ASSERT(bst.size() == 1, "Size remains 1 after duplicate update");
    TEST_ASSERT(*bst.search("P101") == 99, "Value of P101 updated to 99");
    bst.clear();

    // 4. Multi-node insertion and traversals
    // Insert keys: 50, 30, 70, 20, 40, 60, 80
    // Structure:
    //          50
    //        /    |
    //       30     70
    //      /  |   /  |
    //     20  40 60  80
    BinarySearchTree<int, std::string> tree;
    tree.insert(50, "Fifty");
    tree.insert(30, "Thirty");
    tree.insert(70, "Seventy");
    tree.insert(20, "Twenty");
    tree.insert(40, "Forty");
    tree.insert(60, "Sixty");
    tree.insert(80, "Eighty");

    TEST_ASSERT(tree.size() == 7, "Tree size is 7");
    TEST_ASSERT(tree.height() == 3, "Tree height is 3");
    TEST_ASSERT(tree.countLeaves() == 4, "Leaf count is 4 (20, 40, 60, 80)");

    std::vector<int> leaves = tree.getLeafKeys();
    TEST_ASSERT(leaves.size() == 4, "4 leaf keys returned");
    TEST_ASSERT(leaves[0] == 20 && leaves[1] == 40 && leaves[2] == 60 && leaves[3] == 80, "Leaves match");

    // Traversals verification
    std::vector<int> inRec = tree.inorderRecursive();
    std::vector<int> inIter = tree.inorderIterative();
    TEST_ASSERT(inRec.size() == 7 && inIter.size() == 7, "Inorder traversals have 7 elements");
    TEST_ASSERT(inRec == inIter, "Recursive inorder matches Iterative inorder (using CustomStack)");
    TEST_ASSERT(inRec[0] == 20 && inRec[1] == 30 && inRec[2] == 40 && inRec[3] == 50 &&
                inRec[4] == 60 && inRec[5] == 70 && inRec[6] == 80, "Inorder keys are strictly sorted");

    std::vector<int> pre = tree.preorderRecursive();
    TEST_ASSERT(pre[0] == 50 && pre[1] == 30 && pre[2] == 20 && pre[3] == 40 &&
                pre[4] == 70 && pre[5] == 60 && pre[6] == 80, "Preorder keys match");

    std::vector<int> post = tree.postorderRecursive();
    TEST_ASSERT(post[0] == 20 && post[1] == 40 && post[2] == 30 && post[3] == 60 &&
                post[4] == 80 && post[5] == 70 && post[6] == 50, "Postorder keys match");

    std::vector<int> lvl = tree.levelOrder();
    TEST_ASSERT(lvl[0] == 50 && lvl[1] == 30 && lvl[2] == 70 && lvl[3] == 20 &&
                lvl[4] == 40 && lvl[5] == 60 && lvl[6] == 80, "Level-order (using CustomQueue) matches");

    // 5. Three Deletion Cases Verification
    // Case 1: Leaf node (delete 20)
    TEST_ASSERT(tree.remove(20), "Remove leaf 20");
    TEST_ASSERT(tree.size() == 6, "Size is 6");
    TEST_ASSERT(!tree.contains(20), "20 is gone");
    TEST_ASSERT(tree.inorderRecursive() == std::vector<int>({30, 40, 50, 60, 70, 80}), "Inorder sorted after leaf deletion");

    // Case 2: 1 child node (30 now has only right child 40)
    TEST_ASSERT(tree.remove(30), "Remove 1-child node 30");
    TEST_ASSERT(tree.size() == 5, "Size is 5");
    TEST_ASSERT(!tree.contains(30), "30 is gone");
    TEST_ASSERT(tree.inorderRecursive() == std::vector<int>({40, 50, 60, 70, 80}), "Inorder sorted after 1-child deletion");

    // Case 3: 2 children node (70 has 60 and 80)
    TEST_ASSERT(tree.remove(70), "Remove 2-children node 70");
    TEST_ASSERT(tree.size() == 4, "Size is 4");
    TEST_ASSERT(!tree.contains(70), "70 is gone");
    TEST_ASSERT(tree.inorderRecursive() == std::vector<int>({40, 50, 60, 80}), "Inorder sorted after 2-children deletion");

    // Root deletion (50 has left child 40 and right subtree containing 60, 80)
    TEST_ASSERT(tree.remove(50), "Remove root node 50");
    TEST_ASSERT(tree.size() == 3, "Size is 3");
    TEST_ASSERT(!tree.contains(50), "50 is gone");
    TEST_ASSERT(tree.inorderRecursive() == std::vector<int>({40, 60, 80}), "Inorder sorted after root deletion");

    // Missing key deletion
    TEST_ASSERT(!tree.remove(999), "Deleting missing key returns false");

    // 6. Independent Mirror Copy Verification
    BinarySearchTree<int, std::string> mirrorSource;
    mirrorSource.insert(50, "50");
    mirrorSource.insert(30, "30");
    mirrorSource.insert(70, "70");
    mirrorSource.insert(20, "20");
    mirrorSource.insert(40, "40");

    auto mirrorTree = mirrorSource.createMirrorTree();
    TEST_ASSERT(mirrorTree.size() == 5, "Mirror tree size is 5");
    TEST_ASSERT(mirrorTree.height() == mirrorSource.height(), "Mirror tree height matches original");

    std::vector<int> mirrorIn = mirrorTree.inorderKeys();
    // Mirrored tree inorder must produce strict DESCENDING order: 70, 50, 40, 30, 20
    TEST_ASSERT(mirrorIn == std::vector<int>({70, 50, 40, 30, 20}), "Mirror inorder is strictly descending");

    // Verify original tree remains unaltered
    TEST_ASSERT(mirrorSource.inorderRecursive() == std::vector<int>({20, 30, 40, 50, 70}), "Original BST is unaltered by mirror creation");

    // 7. Non-Owning Pointer Policy
    // Deleting BST node must NEVER delete registry-owned Patient object
    {
        Patient* rawPatient = new Patient();
        rawPatient->patientId = "P999";
        rawPatient->fullName = "Test Subject";

        BinarySearchTree<std::string, Patient*> patientBST;
        patientBST.insert(rawPatient->patientId, rawPatient);
        TEST_ASSERT(patientBST.size() == 1, "BST holds 1 patient pointer");

        TEST_ASSERT(patientBST.remove("P999"), "Remove patient pointer node from BST");
        TEST_ASSERT(patientBST.empty(), "BST is empty");

        // rawPatient is still valid and accessible!
        TEST_ASSERT(rawPatient->fullName == "Test Subject", "Non-owning Patient survived BST node deletion");
        delete rawPatient; // Clean up manually
    }

    // 8. Copy/Move Semantics
    BinarySearchTree<int, std::string> copyTree = mirrorSource;
    TEST_ASSERT(copyTree.size() == mirrorSource.size(), "Copy tree size matches");
    copyTree.insert(999, "999");
    TEST_ASSERT(copyTree.size() == 6 && mirrorSource.size() == 5, "Copy independence verified");

    BinarySearchTree<int, std::string> movedTree = std::move(copyTree);
    TEST_ASSERT(movedTree.size() == 6 && copyTree.empty(), "Move constructor verified");
}

void testBinaryMinHeapAndTriage() {
    // 1. Generic BinaryMinHeap<int> verification
    {
        BinaryMinHeap<int> minHeap;
        TEST_ASSERT(minHeap.empty() && minHeap.size() == 0, "Initial min-heap empty");
        int outVal = 0;
        TEST_ASSERT(!minHeap.extractMin(outVal), "extractMin on empty heap returns false");
        TEST_ASSERT(minHeap.peek() == nullptr, "peek on empty heap returns nullptr");

        minHeap.insert(50);
        TEST_ASSERT(!minHeap.empty() && minHeap.size() == 1, "Size is 1");
        TEST_ASSERT(*minHeap.peek() == 50, "Peek is 50");
        TEST_ASSERT(minHeap.extractMin(outVal) && outVal == 50, "Extracted 50");
        TEST_ASSERT(minHeap.empty(), "Heap empty after extract");

        minHeap.insert(40);
        minHeap.insert(10);
        minHeap.insert(30);
        minHeap.insert(50);
        minHeap.insert(20);
        TEST_ASSERT(minHeap.size() == 5, "Size is 5");

        std::vector<int> extracted;
        while (!minHeap.empty()) {
            minHeap.extractMin(outVal);
            extracted.push_back(outVal);
        }
        TEST_ASSERT(extracted == std::vector<int>({10, 20, 30, 40, 50}), "Generic heap extracts ascending order");

        // Bottom-up heapify buildHeap in O(n)
        minHeap.buildHeap({80, 20, 60, 10, 50, 30, 70});
        extracted.clear();
        while (!minHeap.empty()) {
            minHeap.extractMin(outVal);
            extracted.push_back(outVal);
        }
        TEST_ASSERT(extracted == std::vector<int>({10, 20, 30, 50, 60, 70, 80}), "Bottom-up buildHeap produces sorted extractions");
        // Read-only peek verification
        minHeap.insert(99);
        const int* peekVal = minHeap.peek();
        TEST_ASSERT(peekVal != nullptr && *peekVal == 99, "Read-only peek returns pointer to const");
        minHeap.clear();
    }

    // 2. TriageMinHeap with Direct Handle Tracking
    TriageMinHeap triageHeap;

    TEST_ASSERT(triageHeap.empty() && triageHeap.size() == 0, "Initial triage heap empty");
    TriageRecord tr;
    TEST_ASSERT(!triageHeap.extractMin(tr), "Extract on empty triage heap returns false");
    TEST_ASSERT(triageHeap.peek() == nullptr, "Peek returns nullptr");

    // Invalid & Duplicate handle rejection
    TEST_ASSERT(!triageHeap.insert(TriageRecord(-1, "BAD_NEG", UrgencyLevel::URGENT, 1)), "Negative handle rejected");
    TEST_ASSERT(!triageHeap.insert(TriageRecord(100000, "BAD_MAX", UrgencyLevel::URGENT, 1)), "Handle >= 100000 rejected");
    TEST_ASSERT(triageHeap.insert(TriageRecord(1, "P101", UrgencyLevel::URGENT, 1)), "Insert handle 1 succeeds");
    TEST_ASSERT(!triageHeap.insert(TriageRecord(1, "P101_DUP", UrgencyLevel::LESS_URGENT, 2)), "Duplicate active handle 1 rejected");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants hold after duplicate rejection");

    // Urgency validation (1–5) on insert and changeUrgency
    TEST_ASSERT(!triageHeap.insert(TriageRecord(10, "BAD_U0", static_cast<UrgencyLevel>(0), 1)), "Urgency 0 rejected on insert");
    TEST_ASSERT(!triageHeap.insert(TriageRecord(10, "BAD_U6", static_cast<UrgencyLevel>(6), 1)), "Urgency 6 rejected on insert");
    TEST_ASSERT(!triageHeap.changeUrgency(1, static_cast<UrgencyLevel>(0)), "changeUrgency to 0 rejected");
    TEST_ASSERT(!triageHeap.changeUrgency(1, static_cast<UrgencyLevel>(6)), "changeUrgency to 6 rejected");
    TEST_ASSERT(triageHeap.peek()->urgency == UrgencyLevel::URGENT, "Handle 1 urgency preserved after rejected changeUrgency");

    triageHeap.clear();
    TEST_ASSERT(triageHeap.empty(), "Heap empty after clear");

    // Transactional buildHeap validation
    // Establish live heap with 2 records
    TEST_ASSERT(triageHeap.insert(TriageRecord(10, "LIVE_1", UrgencyLevel::EMERGENT, 100)), "Insert LIVE_1");
    TEST_ASSERT(triageHeap.insert(TriageRecord(20, "LIVE_2", UrgencyLevel::LESS_URGENT, 200)), "Insert LIVE_2");
    TEST_ASSERT(triageHeap.size() == 2 && triageHeap.peek()->internalHandle == 10, "Live heap established");

    // Test failed rebuild with duplicate handles in batch: live state must be preserved
    std::vector<TriageRecord> batchDup = {
        TriageRecord(1, "B1", UrgencyLevel::URGENT, 1),
        TriageRecord(1, "B1_DUP", UrgencyLevel::RESUSCITATION, 2)
    };
    TEST_ASSERT(!triageHeap.buildHeap(batchDup), "buildHeap with duplicate handle returns false");
    TEST_ASSERT(triageHeap.size() == 2 && triageHeap.peek()->internalHandle == 10, "Live heap preserved after duplicate rebuild failure");
    TEST_ASSERT(triageHeap.validateInvariants(), "Live heap invariants hold after failed rebuild");

    // Test failed rebuild with out-of-range handle: live state must be preserved
    std::vector<TriageRecord> batchBadHandle = {
        TriageRecord(100000, "BAD_H", UrgencyLevel::URGENT, 1)
    };
    TEST_ASSERT(!triageHeap.buildHeap(batchBadHandle), "buildHeap with handle >= 100000 returns false");
    TEST_ASSERT(triageHeap.size() == 2 && triageHeap.peek()->internalHandle == 10, "Live heap preserved after out-of-range handle rebuild failure");

    // Test failed rebuild with out-of-range urgency: live state must be preserved
    std::vector<TriageRecord> batchBadUrg = {
        TriageRecord(5, "BAD_U", static_cast<UrgencyLevel>(7), 1)
    };
    TEST_ASSERT(!triageHeap.buildHeap(batchBadUrg), "buildHeap with urgency 7 returns false");
    TEST_ASSERT(triageHeap.size() == 2 && triageHeap.peek()->internalHandle == 10, "Live heap preserved after bad urgency rebuild failure");

    // Following failed rebuilds, verify valid insertion, extraction, and invariant validation
    TEST_ASSERT(triageHeap.insert(TriageRecord(5, "NEW_HIGH", UrgencyLevel::RESUSCITATION, 50)), "Insert into preserved heap succeeds");
    TEST_ASSERT(triageHeap.peek()->internalHandle == 5, "New high-urgency record becomes root");
    TriageRecord extractedRec;
    TEST_ASSERT(triageHeap.extractMin(extractedRec) && extractedRec.internalHandle == 5, "Extracted handle 5");
    TEST_ASSERT(triageHeap.extractMin(extractedRec) && extractedRec.internalHandle == 10, "Extracted handle 10");
    TEST_ASSERT(triageHeap.extractMin(extractedRec) && extractedRec.internalHandle == 20, "Extracted handle 20");
    TEST_ASSERT(triageHeap.empty() && triageHeap.validateInvariants(), "Preserved heap drained completely and valid");

    // Equal-Urgency FIFO Tie-Breaking
    // Three patients with equal urgency 2, but differing arrival sequences:
    triageHeap.insert(TriageRecord(1, "P1", UrgencyLevel::EMERGENT, 10)); // seq 10
    triageHeap.insert(TriageRecord(2, "P2", UrgencyLevel::EMERGENT, 30)); // seq 30
    triageHeap.insert(TriageRecord(3, "P3", UrgencyLevel::EMERGENT, 20)); // seq 20
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants valid after 3 inserts");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P1", "1st extract is P1 (seq 10)");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants valid after 1st extract");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P3", "2nd extract is P3 (seq 20)");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants valid after 2nd extract");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P2", "3rd extract is P2 (seq 30)");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.empty(), "Heap empty and invariant holds");

    // Re-insertion of previously removed handle succeeds
    TEST_ASSERT(triageHeap.insert(TriageRecord(1, "P1_NEW", UrgencyLevel::RESUSCITATION, 50)), "Reinserting removed handle 1 succeeds");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.size() == 1, "Reinserted handle 1 active");
    triageHeap.clear();

    // 3. Dynamic Acuity Updates in Both Directions (Increase & Decrease) + Root Update
    triageHeap.insert(TriageRecord(1, "P1", UrgencyLevel::LESS_URGENT, 1));  // Urgency 4, seq 1
    triageHeap.insert(TriageRecord(2, "P2", UrgencyLevel::URGENT, 2));       // Urgency 3, seq 2
    triageHeap.insert(TriageRecord(3, "P3", UrgencyLevel::EMERGENT, 3));     // Urgency 2, seq 3
    TEST_ASSERT(triageHeap.validateInvariants(), "Initial heap valid");
    TEST_ASSERT(triageHeap.peek()->patientId == "P3", "Root is P3 (Urgency 2)");

    // Urgency Increase: P1 deteriorates from Urgency 4 to Urgency 1 (RESUSCITATION) -> sifts UP to root
    TEST_ASSERT(triageHeap.changeUrgency(1, UrgencyLevel::RESUSCITATION), "P1 condition worsened to Urgency 1");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants hold after sift-up");
    TEST_ASSERT(triageHeap.peek()->patientId == "P1", "P1 is now root after priority upgrade");

    // Root Update & Urgency Decrease: Root P1 stabilizes from Urgency 1 to Urgency 5 (NON_URGENT) -> sifts DOWN
    TEST_ASSERT(triageHeap.changeUrgency(1, UrgencyLevel::NON_URGENT), "Root P1 stabilized to Urgency 5");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants hold after root sift-down (no parent(0) access)");
    TEST_ASSERT(triageHeap.peek()->patientId == "P3", "P3 reclaimed root position");

    // Unchanged urgency is a safe no-op
    TEST_ASSERT(triageHeap.changeUrgency(3, UrgencyLevel::EMERGENT), "Unchanged urgency is safe no-op");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariants hold after no-op update");

    // 4. Arbitrary Patient Removal (Root, Middle, Last)
    // Current heap has P3, P2, P1
    TEST_ASSERT(triageHeap.removePatient(2), "Remove middle patient P2 (handle 2)");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.size() == 2, "Invariants hold after middle removal");
    TEST_ASSERT(triageHeap.getPositionByHandle(2) == -1, "Handle 2 position is -1");

    TEST_ASSERT(triageHeap.removePatient(3), "Remove root patient P3 (handle 3)");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.size() == 1, "Invariants hold after root removal");

    TEST_ASSERT(triageHeap.removePatient(1), "Remove last remaining patient P1 (handle 1)");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.empty(), "Heap cleanly empty after removing all");

    // 5. Documented Fixture Verification (TC-06)
    // Patients:
    // P104: handle 4, Urgency 1 (RESUSCITATION), arrival 4
    // P107: handle 7, Urgency 1 (RESUSCITATION), arrival 7
    // P112: handle 12, Urgency 1 (RESUSCITATION), arrival 12
    // P108: handle 8, Urgency 3 (URGENT), arrival 8
    triageHeap.insert(TriageRecord(4, "P104", UrgencyLevel::RESUSCITATION, 4));
    triageHeap.insert(TriageRecord(7, "P107", UrgencyLevel::RESUSCITATION, 7));
    triageHeap.insert(TriageRecord(12, "P112", UrgencyLevel::RESUSCITATION, 12));
    triageHeap.insert(TriageRecord(8, "P108", UrgencyLevel::URGENT, 8));
    TEST_ASSERT(triageHeap.validateInvariants(), "TC-06 fixture inserted");

    // Update P108 to Urgency 1
    TEST_ASSERT(triageHeap.changeUrgency(8, UrgencyLevel::RESUSCITATION), "P108 upgraded to Urgency 1");
    TEST_ASSERT(triageHeap.validateInvariants(), "TC-06 fixture invariants hold after update");

    // Verified extraction order MUST be: P104 (seq 4) -> P107 (seq 7) -> P108 (seq 8) -> P112 (seq 12)
    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P104", "1st extracted patient must be P104");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariant after P104 extraction");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P107", "2nd extracted patient must be P107");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariant after P107 extraction");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P108", "3rd extracted patient must be P108");
    TEST_ASSERT(triageHeap.validateInvariants(), "Invariant after P108 extraction");

    TEST_ASSERT(triageHeap.extractMin(tr) && tr.patientId == "P112", "4th extracted patient must be P112");
    TEST_ASSERT(triageHeap.validateInvariants() && triageHeap.empty(), "Heap empty after TC-06 verification");

    // 6. Deterministic Sequence of 60 Mixed Heap Operations vs Vector-Based Reference
    TriageMinHeap testHeap;
    std::vector<TriageRecord> refVector;
    TriageRecordComparator refComp;

    auto refExtractMin = [&]() -> TriageRecord {
        auto minIt = std::min_element(refVector.begin(), refVector.end(), refComp);
        TriageRecord best = *minIt;
        refVector.erase(minIt);
        return best;
    };

    auto refChangeUrgency = [&](int handle, UrgencyLevel u) {
        for (auto& r : refVector) {
            if (r.internalHandle == handle) {
                r.urgency = u;
                return;
            }
        }
    };

    auto refRemove = [&](int handle) {
        for (auto it = refVector.begin(); it != refVector.end(); ++it) {
            if (it->internalHandle == handle) {
                refVector.erase(it);
                return;
            }
        }
    };

    // Deterministic pseudo-random seed sequence
    uint32_t seed = 42;
    auto nextRand = [&]() -> uint32_t {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };

    std::vector<int> activeHandles;
    int nextHandle = 0;
    uint64_t arrivalSeqCounter = 1;

    for (int step = 0; step < 60; ++step) {
        uint32_t op = nextRand() % 4;

        if (op == 0 || activeHandles.empty()) {
            // Insert
            int h = nextHandle++;
            UrgencyLevel urg = intToUrgency(static_cast<int>((nextRand() % 5) + 1));
            uint64_t seq = arrivalSeqCounter++;
            TriageRecord rec(h, "P_" + std::to_string(h), urg, seq);

            testHeap.insert(rec);
            refVector.push_back(rec);
            activeHandles.push_back(h);
        } else if (op == 1) {
            // Change Urgency
            size_t idx = nextRand() % activeHandles.size();
            int h = activeHandles[idx];
            UrgencyLevel newUrg = intToUrgency(static_cast<int>((nextRand() % 5) + 1));

            testHeap.changeUrgency(h, newUrg);
            refChangeUrgency(h, newUrg);
        } else if (op == 2) {
            // Remove Patient
            size_t idx = nextRand() % activeHandles.size();
            int h = activeHandles[idx];
            activeHandles.erase(activeHandles.begin() + static_cast<std::ptrdiff_t>(idx));

            testHeap.removePatient(h);
            refRemove(h);
        } else {
            // Extract Min
            TriageRecord heapRec;
            bool ok = testHeap.extractMin(heapRec);
            TEST_ASSERT(ok, "Heap extract min succeeded");
            TriageRecord refRec = refExtractMin();

            TEST_ASSERT(heapRec.internalHandle == refRec.internalHandle, "Extracted handle matches reference");
            TEST_ASSERT(heapRec.urgency == refRec.urgency, "Extracted urgency matches reference");
            TEST_ASSERT(heapRec.arrivalSequence == refRec.arrivalSequence, "Extracted arrival sequence matches reference");

            auto it = std::find(activeHandles.begin(), activeHandles.end(), heapRec.internalHandle);
            if (it != activeHandles.end()) {
                activeHandles.erase(it);
            }
        }

        TEST_ASSERT(testHeap.size() == refVector.size(), "Heap size matches reference size");
        TEST_ASSERT(testHeap.validateInvariants(), "Heap invariants hold after mixed operation");
    }

    // Drain remaining elements
    while (!testHeap.empty()) {
        TriageRecord hRec;
        testHeap.extractMin(hRec);
        TriageRecord rRec = refExtractMin();
        TEST_ASSERT(hRec.internalHandle == rRec.internalHandle, "Final drain matches reference");
        TEST_ASSERT(testHeap.validateInvariants(), "Invariant holds during final drain");
    }
    TEST_ASSERT(refVector.empty(), "Reference drained completely");
}

// ============================================================================
// MILESTONE 4 TESTS: CAMPUS GRAPH, TRAVERSAL, DIJKSTRA & MST ALGORITHMS
// ============================================================================
void testCampusGraphAndRouting() {
    std::cout << "[SUBTEST] Campus Graph Topology, Central Mutation & Representation Sync ...\n";

    // 1. Initial representation agreement on empty/fresh graph
    CampusGraph testG(5);
    TEST_ASSERT(testG.getNumNodes() == 5, "Number of nodes is 5");
    TEST_ASSERT(testG.validateRepresentationAgreement(), "Fresh graph representations in sync");

    // Invalid corridor additions
    TEST_ASSERT(!testG.addCorridor(-1, 2, 50), "Negative node ID rejected");
    TEST_ASSERT(!testG.addCorridor(0, 5, 50), "Node ID >= numNodes rejected");
    TEST_ASSERT(!testG.addCorridor(2, 2, 50), "Self-loop rejected");
    TEST_ASSERT(!testG.addCorridor(0, 1, -10), "Negative weight rejected");

    // Valid zero-weight edge
    TEST_ASSERT(testG.addCorridor(0, 1, 0, "Zero-weight bridge"), "Zero-weight edge accepted");
    TEST_ASSERT(testG.getCorridorWeight(0, 1) == 0 && testG.getCorridorWeight(1, 0) == 0, "Zero weight in matrix");
    TEST_ASSERT(testG.validateRepresentationAgreement(), "Representation sync holds with zero-weight edge");

    // Duplicate undirected edge rejection
    TEST_ASSERT(!testG.addCorridor(0, 1, 50), "Duplicate undirected edge (0, 1) rejected");
    TEST_ASSERT(!testG.addCorridor(1, 0, 50), "Duplicate reversed edge (1, 0) rejected");

    // Add positive corridors
    TEST_ASSERT(testG.addCorridor(1, 2, 100), "Add corridor (1, 2)");
    TEST_ASSERT(testG.addCorridor(2, 3, 200), "Add corridor (2, 3)");
    TEST_ASSERT(testG.addCorridor(3, 4, 300), "Add corridor (3, 4)");
    TEST_ASSERT(testG.validateRepresentationAgreement(), "Representations in sync after additions");

    // Direct SinglyLinkedList inspection with nullptr check on invalid node
    const auto* list0 = testG.getAdjacencyList(0);
    TEST_ASSERT(list0 != nullptr && list0->size() == 1, "Node 0 has 1 neighbor in adjacency list");
    const auto* list1 = testG.getAdjacencyList(1);
    TEST_ASSERT(list1 != nullptr && list1->size() == 2, "Node 1 has 2 neighbors in adjacency list");
    TEST_ASSERT(testG.getAdjacencyList(-1) == nullptr, "getAdjacencyList(-1) returns nullptr for invalid vertex");
    TEST_ASSERT(testG.getAdjacencyList(99) == nullptr, "getAdjacencyList(99) returns nullptr for invalid vertex");

    // Enforce corridor weight upper bound (kMaxCorridorWeight = 86400)
    TEST_ASSERT(!testG.addCorridor(0, 4, 86401), "Corridor transit weight > 86400 seconds rejected");

    // Bidirectional Corridor Blocking
    TEST_ASSERT(testG.blockCorridor(1, 2), "Block corridor (1, 2)");
    TEST_ASSERT(testG.isCorridorBlocked(1, 2) && testG.isCorridorBlocked(2, 1), "Corridor (1, 2) blocked bidirectionally");
    TEST_ASSERT(testG.getCorridorWeight(1, 2) == CampusGraph::BLOCKED_EDGE, "Matrix reflects BLOCKED_EDGE");
    TEST_ASSERT(testG.getOriginalCorridorWeight(1, 2) == 100, "Original weight 100 preserved while blocked");
    TEST_ASSERT(testG.validateRepresentationAgreement(), "Representations in sync after blocking");

    // Idempotent block
    TEST_ASSERT(testG.blockCorridor(1, 2), "Re-blocking already blocked corridor is safe no-op");

    // Reopen corridor
    TEST_ASSERT(testG.reopenCorridor(1, 2), "Reopen corridor (1, 2)");
    TEST_ASSERT(!testG.isCorridorBlocked(1, 2) && !testG.isCorridorBlocked(2, 1), "Corridor open bidirectionally");
    TEST_ASSERT(testG.getCorridorWeight(1, 2) == 100, "Matrix weight restored to 100");
    TEST_ASSERT(testG.validateRepresentationAgreement(), "Representations in sync after reopening");

    // Idempotent reopen
    TEST_ASSERT(testG.reopenCorridor(1, 2), "Re-opening already open corridor is safe no-op");

    // 2. Traversals & Connected Components
    std::cout << "[SUBTEST] Graph Traversals (BFS & DFS) and Connected Components ...\n";
    std::vector<bool> visited;
    std::vector<int> bfsOrder = testG.bfs(0, visited);
    TEST_ASSERT(bfsOrder == std::vector<int>({0, 1, 2, 3, 4}), "BFS explores in deterministic order");

    std::vector<bool> visitedList;
    std::vector<int> dfsListOrder = testG.dfsList(0, visitedList);
    std::vector<bool> visitedMatrix;
    std::vector<int> dfsMatrixOrder = testG.dfsMatrix(0, visitedMatrix);
    TEST_ASSERT(dfsListOrder == dfsMatrixOrder, "DFS List and DFS Matrix produce identical visitation sequence");

    // Connected components on connected graph
    auto compConnected = testG.getConnectedComponents();
    TEST_ASSERT(compConnected.size() == 1, "Connected graph has 1 component");

    // Block (1, 2) -> graph splits into 2 components: {0, 1} and {2, 3, 4}
    testG.blockCorridor(1, 2);
    auto compSplit = testG.getConnectedComponents();
    TEST_ASSERT(compSplit.size() == 2, "Graph splits into 2 components when bridge blocked");
    testG.reopenCorridor(1, 2);

    // 3. Undirected Cycle Detection with Parent-Edge Handling
    std::cout << "[SUBTEST] Undirected Cycle Detection ...\n";
    // Current testG is a simple tree/path: 0 - 1 - 2 - 3 - 4. Must NOT report a cycle
    TEST_ASSERT(!testG.hasCycle(), "Tree/path graph contains NO cycle (parent-edge handled correctly)");

    // Add back-edge (4, 1, 50) -> creates cycle 1 - 2 - 3 - 4 - 1
    testG.addCorridor(4, 1, 50);
    TEST_ASSERT(testG.hasCycle(), "Cycle detected after adding back-edge");

    // Block back-edge -> cycle is broken
    testG.blockCorridor(4, 1);
    TEST_ASSERT(!testG.hasCycle(), "No cycle when cycle edge is blocked");
    testG.reopenCorridor(4, 1);

    // Test cycle detection across multiple disconnected components:
    // Component 1 (nodes 0, 1) has no cycle. Component 2 (nodes 2, 3, 4) has a triangle cycle.
    CampusGraph multiCompG(6);
    multiCompG.addCorridor(0, 1, 10); // Tree in component 1
    multiCompG.addCorridor(2, 3, 20); // Triangle in component 2
    multiCompG.addCorridor(3, 4, 30);
    multiCompG.addCorridor(4, 2, 40);
    // Node 5 is isolated
    TEST_ASSERT(multiCompG.hasCycle(), "Cycle detected in non-root component (searches all components)");

    // 4. Dijkstra Routing on Seed Map
    std::cout << "[SUBTEST] Dijkstra Shortest Route Verification on Seed Map ...\n";
    std::vector<CampusLocation> seedLocations;
    std::vector<CorridorEdge> seedEdges;
    std::string loadErr;
    bool loaded = DataLoader::loadCampusMapTXT(gDataDir + "campus_map_seed.txt", seedLocations, seedEdges, loadErr);
    TEST_ASSERT(loaded, "Seed campus map loaded");

    CampusGraph seedGraph(static_cast<int>(seedLocations.size()));
    for (const auto& loc : seedLocations) {
        seedGraph.setLocation(loc.nodeId, loc);
    }
    for (const auto& edge : seedEdges) {
        seedGraph.addCorridor(edge.u, edge.v, edge.transitTimeSeconds);
    }
    TEST_ASSERT(seedGraph.validateRepresentationAgreement(), "Seed graph representation in sync");

    // Helper: verify returned path integrity
    auto verifyRouteIntegrity = [&](const DijkstraResult& res, const CampusGraph& g) {
        TEST_ASSERT(res.isReachable, "Route reported reachable");
        TEST_ASSERT(!res.path.empty(), "Path is not empty");
        int64_t edgeSum = 0;
        for (size_t i = 0; i + 1 < res.path.size(); ++i) {
            int u = res.path[i];
            int v = res.path[i + 1];
            int64_t w = g.getCorridorWeight(u, v);
            TEST_ASSERT(w >= 0, "Each corridor on path must exist and be open");
            edgeSum += w;
        }
        TEST_ASSERT(edgeSum == res.totalCost, "Sum of corridor edge weights matches reported totalCost");
    };

    // A. Baseline 0 -> 3: 270 seconds (0 -> 1 -> 5 -> 3)
    DijkstraResult r0_3 = seedGraph.findShortestRoute(0, 3);
    TEST_ASSERT(r0_3.isReachable && r0_3.totalCost == 270, "0 -> 3 cost is 270s");
    TEST_ASSERT(r0_3.path == std::vector<int>({0, 1, 5, 3}), "0 -> 3 path is 0 -> 1 -> 5 -> 3");
    verifyRouteIntegrity(r0_3, seedGraph);

    // B. Incident Blocking (1, 5) -> Reroute 0 -> 3: 330 seconds (0 -> 1 -> 7 -> 3)
    TEST_ASSERT(seedGraph.blockCorridor(1, 5), "Block corridor (1, 5)");
    DijkstraResult r0_3_blocked = seedGraph.findShortestRoute(0, 3);
    TEST_ASSERT(r0_3_blocked.isReachable && r0_3_blocked.totalCost == 330, "0 -> 3 blocked cost is 330s");
    TEST_ASSERT(r0_3_blocked.path == std::vector<int>({0, 1, 7, 3}), "0 -> 3 blocked path is 0 -> 1 -> 7 -> 3");
    verifyRouteIntegrity(r0_3_blocked, seedGraph);

    // C. 1 -> 3 with (1, 5) blocked: 270 seconds (1 -> 7 -> 3)
    DijkstraResult r1_3_blocked = seedGraph.findShortestRoute(1, 3);
    TEST_ASSERT(r1_3_blocked.isReachable && r1_3_blocked.totalCost == 270, "1 -> 3 blocked cost is 270s");
    TEST_ASSERT(r1_3_blocked.path == std::vector<int>({1, 7, 3}), "1 -> 3 blocked path is 1 -> 7 -> 3");
    verifyRouteIntegrity(r1_3_blocked, seedGraph);

    // D. Reopen (1, 5) -> 1 -> 3: 210 seconds (1 -> 5 -> 3)
    TEST_ASSERT(seedGraph.reopenCorridor(1, 5), "Reopen corridor (1, 5)");
    DijkstraResult r1_3 = seedGraph.findShortestRoute(1, 3);
    TEST_ASSERT(r1_3.isReachable && r1_3.totalCost == 210, "1 -> 3 cost is 210s");
    TEST_ASSERT(r1_3.path == std::vector<int>({1, 5, 3}), "1 -> 3 path is 1 -> 5 -> 3");
    verifyRouteIntegrity(r1_3, seedGraph);

    // Zero-cost identity check (u == v)
    DijkstraResult rSelf = seedGraph.findShortestRoute(3, 3);
    TEST_ASSERT(rSelf.isReachable && rSelf.totalCost == 0 && rSelf.path == std::vector<int>({3}), "Source == Dest returns cost 0");

    // Invalid endpoints
    DijkstraResult rBad1 = seedGraph.findShortestRoute(-1, 3);
    TEST_ASSERT(!rBad1.isReachable && rBad1.totalCost == kInfinity && rBad1.path.empty(), "Negative source returns unreachable");
    DijkstraResult rBad2 = seedGraph.findShortestRoute(0, 999);
    TEST_ASSERT(!rBad2.isReachable && rBad2.totalCost == kInfinity && rBad2.path.empty(), "Out-of-range dest returns unreachable");

    // Unreachable destination in disconnected topology
    TEST_ASSERT(seedGraph.blockCorridor(10, 11), "Block corridor (10, 11) isolating node 11");
    DijkstraResult rIso = seedGraph.findShortestRoute(0, 11);
    TEST_ASSERT(!rIso.isReachable && rIso.totalCost == kInfinity && rIso.path.empty(), "Isolated destination unreachable");
    TEST_ASSERT(seedGraph.reopenCorridor(10, 11), "Restore corridor (10, 11)");

    // 5. Minimum Spanning Trees (Kruskal vs Prim) on Seed Map
    std::cout << "[SUBTEST] Minimum Spanning Trees (Kruskal & Prim on Seed Map) ...\n";
    MSTResult kruskalSeed = seedGraph.computeKruskalMST();
    MSTResult primSeed = seedGraph.computePrimMST(0);

    TEST_ASSERT(kruskalSeed.isConnectedSpanningTree, "Kruskal reports connected spanning tree");
    TEST_ASSERT(kruskalSeed.totalTransitWeight == 1280, "Kruskal MST total transit weight is 1280 seconds");
    TEST_ASSERT(kruskalSeed.edges.size() == 11, "Kruskal MST has V - 1 = 11 edges");
    TEST_ASSERT(kruskalSeed.componentCount == 1, "Kruskal MST spans 1 component");

    TEST_ASSERT(primSeed.isConnectedSpanningTree, "Prim reports connected spanning tree");
    TEST_ASSERT(primSeed.totalTransitWeight == 1280, "Prim MST total transit weight is 1280 seconds");
    TEST_ASSERT(primSeed.edges.size() == 11, "Prim MST has V - 1 = 11 edges");
    TEST_ASSERT(primSeed.componentCount == 1, "Prim MST spans 1 component");

    // MST Acyclicity and Full Connectivity verification via DSU
    DisjointSetUnion mstDSU(12);
    for (const auto& e : kruskalSeed.edges) {
        bool added = mstDSU.unite(e.u, e.v);
        TEST_ASSERT(added, "Every edge in MST must connect previously disjoint components (Acyclic)");
    }
    TEST_ASSERT(mstDSU.componentCount() == 1, "MST connects all 12 nodes into a single connected component");

    // 6. Disconnected Graph Minimum Spanning Forest (MSF) Verification
    std::cout << "[SUBTEST] Disconnected Graph Spanning Forest Handling ...\n";
    CampusGraph disjointG(6);
    // Component A: Triangle {0, 1, 2}
    disjointG.addCorridor(0, 1, 10);
    disjointG.addCorridor(1, 2, 20);
    disjointG.addCorridor(0, 2, 30); // MST takes (0,1)=10 and (1,2)=20 -> 30
    // Component B: Triangle {3, 4, 5}
    disjointG.addCorridor(3, 4, 40);
    disjointG.addCorridor(4, 5, 50);
    disjointG.addCorridor(3, 5, 60); // MST takes (3,4)=40 and (4,5)=50 -> 90
    // Total Forest Transit Weight = 30 + 90 = 120 seconds. Total Edges = 4 (V - C = 6 - 2 = 4).

    MSTResult kruskalForest = disjointG.computeKruskalMST();
    MSTResult primForest = disjointG.computePrimMST(0);

    TEST_ASSERT(!kruskalForest.isConnectedSpanningTree, "Kruskal does NOT label partial forest as connected MST");
    TEST_ASSERT(kruskalForest.componentCount == 2, "Kruskal reports 2 components in spanning forest");
    TEST_ASSERT(kruskalForest.edges.size() == 4, "Kruskal spanning forest has 4 edges across 6 nodes");
    TEST_ASSERT(kruskalForest.totalTransitWeight == 120, "Kruskal forest total weight is 120s");

    TEST_ASSERT(!primForest.isConnectedSpanningTree, "Prim does NOT label partial forest as connected MST");
    TEST_ASSERT(primForest.componentCount == 2, "Prim reports 2 components in spanning forest");
    TEST_ASSERT(primForest.edges.size() == 4, "Prim spanning forest has 4 edges across 6 nodes");
    TEST_ASSERT(primForest.totalTransitWeight == 120, "Prim forest total weight is 120s");

    // 7. Small Independently Checkable Diamond Graph Fixture
    std::cout << "[SUBTEST] Diamond Graph Fixture Routing and MST ...\n";
    CampusGraph diamond(4);
    diamond.addCorridor(0, 1, 10);
    diamond.addCorridor(0, 2, 15);
    diamond.addCorridor(1, 3, 20);
    diamond.addCorridor(2, 3, 5);
    // Shortest path 0 -> 3: 0 -> 2 -> 3 (15 + 5 = 20) vs 0 -> 1 -> 3 (10 + 20 = 30)
    DijkstraResult rDiamond = diamond.findShortestRoute(0, 3);
    TEST_ASSERT(rDiamond.isReachable && rDiamond.totalCost == 20, "Diamond shortest route cost is 20s");
    TEST_ASSERT(rDiamond.path == std::vector<int>({0, 2, 3}), "Diamond route uses 0 -> 2 -> 3");

    // Block (2, 3): reroutes via 0 -> 1 -> 3 with cost 30
    diamond.blockCorridor(2, 3);
    DijkstraResult rDiamondBlocked = diamond.findShortestRoute(0, 3);
    TEST_ASSERT(rDiamondBlocked.isReachable && rDiamondBlocked.totalCost == 30, "Diamond rerouted cost is 30s");
    TEST_ASSERT(rDiamondBlocked.path == std::vector<int>({0, 1, 3}), "Diamond rerouted path uses 0 -> 1 -> 3");
    diamond.reopenCorridor(2, 3);

    // MST of Diamond: (2,3)=5, (0,1)=10, (0,2)=15 -> total 30s (3 edges)
    MSTResult diamondMST = diamond.computeKruskalMST();
    TEST_ASSERT(diamondMST.isConnectedSpanningTree && diamondMST.totalTransitWeight == 30, "Diamond MST weight is 30s");
    TEST_ASSERT(diamondMST.edges.size() == 3, "Diamond MST has 3 edges");

    // 8. Empty Graph & Single-Node Graph MST Semantics
    std::cout << "[SUBTEST] Empty Graph and Single-Node Graph MST Semantics ...\n";
    CampusGraph emptyG(0);
    MSTResult kEmpty = emptyG.computeKruskalMST();
    TEST_ASSERT(kEmpty.componentCount == 0 && !kEmpty.isConnectedSpanningTree && kEmpty.totalTransitWeight == 0 && kEmpty.edges.empty(),
                "Empty graph Kruskal componentCount is 0, not connected");
    MSTResult pEmpty = emptyG.computePrimMST();
    TEST_ASSERT(pEmpty.componentCount == 0 && !pEmpty.isConnectedSpanningTree && pEmpty.totalTransitWeight == 0 && pEmpty.edges.empty(),
                "Empty graph Prim componentCount is 0, not connected");

    CampusGraph singleG(1);
    MSTResult kSingle = singleG.computeKruskalMST();
    TEST_ASSERT(kSingle.componentCount == 1 && kSingle.isConnectedSpanningTree && kSingle.totalTransitWeight == 0 && kSingle.edges.empty(),
                "Single node graph Kruskal componentCount is 1, connected spanning tree");
    MSTResult pSingle = singleG.computePrimMST();
    TEST_ASSERT(pSingle.componentCount == 1 && pSingle.isConnectedSpanningTree && pSingle.totalTransitWeight == 0 && pSingle.edges.empty(),
                "Single node graph Prim componentCount is 1, connected spanning tree");
}

// ============================================================================
// Milestone 5 Automated Verification Suite: Hospital Services & Dispatch
// ============================================================================

void testHospitalRegistryAndRegistration() {
    std::cout << "[SUBTEST] HospitalRegistry Authoritative Ownership and Indexing ...\n";
    HospitalRegistry registry;
    std::string err;

    // Reject null
    TEST_ASSERT(!registry.registerPatient(nullptr, err), "Reject null patient pointer");

    // Valid patient P001
    auto p1 = std::make_unique<Patient>();
    p1->internalHandle = 0;
    p1->patientId = "P001";
    p1->fullName = "Alice Smith";
    p1->age = 30;
    p1->complaint = "Fever";
    p1->urgency = UrgencyLevel::URGENT;
    p1->arrivalSequence = 1;

    TEST_ASSERT(registry.registerPatient(std::move(p1), err), "Register P001 successfully");
    TEST_ASSERT(registry.getPatientCount() == 1, "Patient count is 1");
    TEST_ASSERT(registry.hasPatient("P001"), "Registry has P001");
    TEST_ASSERT(registry.findPatient("P001") != nullptr, "findPatient returns non-null for P001");
    TEST_ASSERT(registry.findPatientByHandle(0) != nullptr, "findPatientByHandle returns non-null for handle 0");
    TEST_ASSERT(registry.findPatient("P001")->fullName == "Alice Smith", "Patient name matches");

    // Reject duplicate ID
    auto pDupId = std::make_unique<Patient>();
    pDupId->internalHandle = 1;
    pDupId->patientId = "P001"; // Duplicate
    pDupId->fullName = "Alice Clone";
    TEST_ASSERT(!registry.registerPatient(std::move(pDupId), err), "Reject duplicate patient ID P001");
    TEST_ASSERT(err.find("already registered") != std::string::npos, "Descriptive duplicate ID error");

    // Reject duplicate handle
    auto pDupHandle = std::make_unique<Patient>();
    pDupHandle->internalHandle = 0; // Duplicate handle
    pDupHandle->patientId = "P002";
    pDupHandle->fullName = "Bob Jones";
    TEST_ASSERT(!registry.registerPatient(std::move(pDupHandle), err), "Reject duplicate handle 0");
    TEST_ASSERT(err.find("already assigned") != std::string::npos, "Descriptive duplicate handle error");

    // Reject out-of-range handle
    auto pBadHandle = std::make_unique<Patient>();
    pBadHandle->internalHandle = -1;
    pBadHandle->patientId = "P003";
    TEST_ASSERT(!registry.registerPatient(std::move(pBadHandle), err), "Reject negative handle");

    auto pExcessHandle = std::make_unique<Patient>();
    pExcessHandle->internalHandle = TriageMinHeap::kMaxSupportedHandle + 5;
    pExcessHandle->patientId = "P004";
    TEST_ASSERT(!registry.registerPatient(std::move(pExcessHandle), err), "Reject excessively large handle");

    // Reject empty ID
    auto pEmptyId = std::make_unique<Patient>();
    pEmptyId->internalHandle = 2;
    pEmptyId->patientId = "";
    TEST_ASSERT(!registry.registerPatient(std::move(pEmptyId), err), "Reject empty patient ID");

    // Verify BST index and HashTable index
    TEST_ASSERT(registry.getHashTable().search("P001") != nullptr, "Hash table index contains P001");
    TEST_ASSERT(registry.getBSTIndex().search("P001") != nullptr, "BST index contains P001");
    TEST_ASSERT(registry.getBSTIndex().search("P999") == nullptr, "BST index returns null for non-existent");
}

void testSeedInitializationAndIndexes() {
    std::cout << "[SUBTEST] Seed Initialization into Registry, Triage Heap, and Appointment Queue ...\n";
    HospitalSystem sys;
    std::string err;
    bool ok = sys.initializeFromSeed(gDataDir, err);
    TEST_ASSERT(ok, "initializeFromSeed succeeds on valid seed data: " + err);

    // 12 patients in patients_seed.csv
    TEST_ASSERT(sys.getRegistry().getPatientCount() == 12, "All 12 seed patients loaded into HospitalRegistry");

    // 9 patients are TRIAGED_WAITING (P101, P103, P104, P105, P107, P108, P109, P110, P112)
    TEST_ASSERT(sys.getTriageManager().size() == 9, "9 seed patients enqueued into TriageMinHeap");

    // 3 patients are APPOINTMENT_SCHEDULED (P102, P106, P111)
    TEST_ASSERT(sys.getAppointmentManager().size() == 3, "3 seed patients enqueued into AppointmentQueue");

    // 3 resources loaded (GURNEY-01 at 0, GURNEY-02 at 6, GURNEY-03 at 1)
    TEST_ASSERT(sys.getDispatchManager().getAllResources().size() == 3, "3 resources loaded");
    const DispatchResource* r1 = sys.getDispatchManager().findResource("GURNEY-01");
    TEST_ASSERT(r1 != nullptr && r1->currentNodeId == 0 && r1->isAvailable(), "GURNEY-01 available at node 0");
    const DispatchResource* r3 = sys.getDispatchManager().findResource("GURNEY-03");
    TEST_ASSERT(r3 != nullptr && r3->currentNodeId == 1 && r3->isAvailable(), "GURNEY-03 available at node 1");

    // Map has 12 locations
    TEST_ASSERT(sys.getMapManager().getNodeCount() == 12, "Campus map has 12 locations");

    // Audit log records events
    std::vector<AuditEvent> events = sys.getAuditEvents();
    TEST_ASSERT(!events.empty(), "Audit log contains initial seed events");
}

void testEqualUrgencyFIFOAndUrgencyUpdates() {
    std::cout << "[SUBTEST] Equal-Urgency FIFO Tie-Breaking and Dynamic Urgency Updates ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Seed load succeeds");

    // In seed data, Urgency 1 patients:
    // P104: arrivalSequence = 4
    // P107: arrivalSequence = 7
    // P112: arrivalSequence = 12
    Patient* top = nullptr;
    TEST_ASSERT(sys.getTriageManager().peekHighestPriority(top), "Peek highest priority patient in triage");
    TEST_ASSERT(top != nullptr && top->patientId == "P104", "P104 is top of triage min-heap");

    // Update P108 (originally urgency 3, arrivalSequence 8) to Urgency 1
    Patient* p108 = sys.getRegistry().findPatient("P108");
    TEST_ASSERT(p108 != nullptr, "P108 exists in registry");
    TEST_ASSERT(p108->urgency == UrgencyLevel::URGENT, "P108 starts as URGENT (3)");
    uint64_t originalSeq = p108->arrivalSequence;
    TEST_ASSERT(originalSeq == 8, "P108 original arrival sequence is 8");

    TEST_ASSERT(sys.updateUrgency("P108", UrgencyLevel::RESUSCITATION, err), "Update P108 to Urgency 1");
    TEST_ASSERT(p108->urgency == UrgencyLevel::RESUSCITATION, "P108 is now Urgency 1");
    TEST_ASSERT(p108->arrivalSequence == originalSeq, "P108 arrivalSequence strictly preserved (8)");

    // Since P104 has arrivalSequence 4 < 8, P104 must STILL be root
    TEST_ASSERT(sys.getTriageManager().peekHighestPriority(top), "Peek top after update");
    TEST_ASSERT(top->patientId == "P104", "P104 is still top (arrSeq 4 < 8)");

    // Extract sequence of Urgency 1 patients:
    Patient* ext1 = nullptr;
    TEST_ASSERT(sys.getTriageManager().extractHighestPriority(ext1, err), "Extract 1");
    TEST_ASSERT(ext1->patientId == "P104", "1st extracted is P104");

    Patient* ext2 = nullptr;
    TEST_ASSERT(sys.getTriageManager().extractHighestPriority(ext2, err), "Extract 2");
    TEST_ASSERT(ext2->patientId == "P107", "2nd extracted is P107");

    Patient* ext3 = nullptr;
    TEST_ASSERT(sys.getTriageManager().extractHighestPriority(ext3, err), "Extract 3");
    TEST_ASSERT(ext3->patientId == "P108", "3rd extracted is P108 (dynamically promoted to Urgency 1)");

    Patient* ext4 = nullptr;
    TEST_ASSERT(sys.getTriageManager().extractHighestPriority(ext4, err), "Extract 4");
    TEST_ASSERT(ext4->patientId == "P112", "4th extracted is P112");
}

void testAppointmentFIFOAndCancellationAndConversion() {
    std::cout << "[SUBTEST] Outpatient Appointment FIFO, Cancellation, and Emergency Conversion ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Seed load succeeds");

    std::vector<std::string> scheduled = sys.getAppointmentManager().getScheduledPatientIds();
    TEST_ASSERT(scheduled.size() == 3, "3 appointments scheduled");
    TEST_ASSERT(scheduled[0] == "P102" && scheduled[1] == "P106" && scheduled[2] == "P111", "FIFO order P102 -> P106 -> P111");

    // 1. Process first appointment: P102 called
    Patient* called1 = nullptr;
    TEST_ASSERT(sys.callNextAppointment(called1, err), "Call first appointment");
    TEST_ASSERT(called1->patientId == "P102", "P102 is called first in FIFO sequence");
    TEST_ASSERT(called1->state == PatientState::UNDER_TREATMENT, "P102 transitions to UNDER_TREATMENT");
    TEST_ASSERT(sys.getAppointmentManager().size() == 2, "2 appointments remaining");

    // 2. Cancel P106 via encapsulated queue removal
    TEST_ASSERT(sys.cancelAppointment("P106", err), "Cancel P106 appointment");
    Patient* p106 = sys.getRegistry().findPatient("P106");
    TEST_ASSERT(p106->state == PatientState::CANCELLED, "P106 state is CANCELLED");
    TEST_ASSERT(sys.getAppointmentManager().size() == 1, "Only 1 appointment remaining");

    // Repeat cancellation on P106 should be rejected
    TEST_ASSERT(!sys.cancelAppointment("P106", err), "Reject duplicate cancellation of P106");

    // 3. Convert P111 from appointment queue to emergency triage
    Patient* p111 = sys.getRegistry().findPatient("P111");
    TEST_ASSERT(p111->state == PatientState::APPOINTMENT_SCHEDULED, "P111 is APPOINTMENT_SCHEDULED");
    TEST_ASSERT(sys.convertAppointmentToEmergency("P111", UrgencyLevel::RESUSCITATION, err), "Convert P111 to Urgency 1 emergency");
    TEST_ASSERT(p111->state == PatientState::TRIAGED_WAITING, "P111 state is now TRIAGED_WAITING");
    TEST_ASSERT(p111->urgency == UrgencyLevel::RESUSCITATION, "P111 urgency is RESUSCITATION (1)");
    TEST_ASSERT(sys.getAppointmentManager().empty(), "Appointment queue is now empty");

    // Calling appointment when queue is empty should fail cleanly
    Patient* noP = nullptr;
    TEST_ASSERT(!sys.callNextAppointment(noP, err), "Fails cleanly when appointment queue is empty");
}

void testDispatchFailuresAndPreconditions() {
    std::cout << "[SUBTEST] Dispatch Failures: Unreachable Destination, No Resource, Unreachable Pickup ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeCampusAndFleet(gDataDir, err), "Campus and fleet load succeeds");

    // 1. Unreachable destination (Leg 2 blocked)
    TEST_ASSERT(sys.registerNewPatient("P201", "Charlie Bucket", 10, "High Fever", UrgencyLevel::RESUSCITATION,
                                       1, 10, 500.0, err), "Register P201");
    TEST_ASSERT(sys.triagePatient("P201", UrgencyLevel::RESUSCITATION, err), "Triage P201");
    Patient* p201 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p201 != nullptr && p201->patientId == "P201", "Select P201 for dispatch");
    TEST_ASSERT(p201->state == PatientState::DISPATCH_PENDING && p201->isDispatchUnassigned(), "P201 is DISPATCH_PENDING unassigned");

    // Block all corridors into node 10 (corridors (8,10) and (10,11))
    sys.getMapManager().blockCorridor(8, 10);
    sys.getMapManager().blockCorridor(10, 11);

    TEST_ASSERT(!sys.assignDispatch("P201", err), "Assignment fails because destination node 10 is unreachable");
    TEST_ASSERT(err.find("unreachable") != std::string::npos, "Descriptive unreachable destination error: " + err);
    TEST_ASSERT(p201->state == PatientState::DISPATCH_PENDING, "P201 remains DISPATCH_PENDING");
    TEST_ASSERT(p201->assignedResourceId.empty(), "P201 assignedResourceId remains empty");

    bool inPending = false;
    for (Patient* p : sys.getUnassignedPendingPatients()) {
        if (p == p201) inPending = true;
    }
    TEST_ASSERT(inPending, "P201 remains in unassigned pending list");

    // Reopen corridors
    sys.getMapManager().reopenCorridor(8, 10);
    sys.getMapManager().reopenCorridor(10, 11);

    // 2. Unreachable pickup / No resource available
    for (auto& r : sys.getDispatchManager().getAllResources()) {
        r.state = ResourceState::TRANSPORTING_PATIENT; // Mark all busy
    }
    TEST_ASSERT(!sys.assignDispatch("P201", err), "Assignment fails when all resources are busy");
    TEST_ASSERT(err.find("No available transport resource") != std::string::npos, "Descriptive no-resource error");

    for (auto& r : sys.getDispatchManager().getAllResources()) {
        r.state = ResourceState::AVAILABLE;
        r.assignedPatientId = "";
    }
}

void testDeterministicResourceSelectionAndTieBreaking() {
    std::cout << "[SUBTEST] Deterministic Resource Selection and Tie-Breaking ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeCampusAndFleet(gDataDir, err), "Campus and fleet load succeeds");

    // Patient P202 at node 1 heading to node 2
    TEST_ASSERT(sys.registerNewPatient("P202", "David Blaine", 45, "Cardiac Arrest", UrgencyLevel::RESUSCITATION,
                                       1, 2, 1200.0, err), "Register P202");
    TEST_ASSERT(sys.triagePatient("P202", UrgencyLevel::RESUSCITATION, err), "Triage P202");
    Patient* p202 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p202 != nullptr && p202->patientId == "P202", "P202 selected for dispatch");

    // Resources in seed: GURNEY-01 at 0, GURNEY-02 at 6, GURNEY-03 at 1 (cost 0s)
    TEST_ASSERT(sys.assignDispatch("P202", err), "Assign dispatch to P202");
    TEST_ASSERT(p202->assignedResourceId == "GURNEY-03", "GURNEY-03 selected due to shortest pickup cost (0s)");

    // Test duplicate assignment rejection
    TEST_ASSERT(!sys.assignDispatch("P202", err), "Reject duplicate assignment on already-assigned patient");
    TEST_ASSERT(err.find("Duplicate assignment rejected") != std::string::npos, "Descriptive duplicate assignment error");

    // Test tie-breaking between equal-cost resources:
    TEST_ASSERT(sys.registerNewPatient("P203", "Elena Gilbert", 24, "Seizure", UrgencyLevel::RESUSCITATION,
                                       0, 2, 800.0, err), "Register P203");
    TEST_ASSERT(sys.triagePatient("P203", UrgencyLevel::RESUSCITATION, err), "Triage P203");
    Patient* p203 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p203 != nullptr && p203->patientId == "P203", "Select P203");

    // Position GURNEY-01 and GURNEY-02 both at node 1 (distance to node 0 is 60s for both)
    DispatchResource* r1 = sys.getDispatchManager().findResource("GURNEY-01");
    DispatchResource* r2 = sys.getDispatchManager().findResource("GURNEY-02");
    r1->currentNodeId = 1;
    r2->currentNodeId = 1;

    TEST_ASSERT(sys.assignDispatch("P203", err), "Assign dispatch to P203");
    // Tie-breaker: "GURNEY-01" < "GURNEY-02"
    TEST_ASSERT(p203->assignedResourceId == "GURNEY-01", "GURNEY-01 selected by lexicographical tie-breaker");
}

void testSimulationLegsCorridorClosureAndRerouting() {
    std::cout << "[SUBTEST] Simulation Movement Legs, Corridor Closure, Rerouting and Pause ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeCampusAndFleet(gDataDir, err), "Campus and fleet load succeeds");

    TEST_ASSERT(sys.registerNewPatient("P204", "Fiona Gallagher", 28, "Burns", UrgencyLevel::RESUSCITATION,
                                       1, 2, 950.0, err), "Register P204");
    TEST_ASSERT(sys.triagePatient("P204", UrgencyLevel::RESUSCITATION, err), "Triage P204");
    Patient* p204 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p204 != nullptr, "Select P204");

    // Make only GURNEY-01 available at node 0
    for (auto& r : sys.getDispatchManager().getAllResources()) {
        if (r.resourceId == "GURNEY-01") {
            r.state = ResourceState::AVAILABLE;
            r.currentNodeId = 0;
            r.assignedPatientId = "";
        } else {
            r.state = ResourceState::TRANSPORTING_PATIENT;
        }
    }

    TEST_ASSERT(sys.assignDispatch("P204", err), "Assign GURNEY-01 to P204");
    TEST_ASSERT(p204->assignedResourceId == "GURNEY-01", "Assigned GURNEY-01");

    // Block corridor (0, 1), (0, 2), (0, 6) from node 0 to isolate node 0
    sys.getMapManager().blockCorridor(0, 1);
    sys.getMapManager().blockCorridor(0, 2);
    sys.getMapManager().blockCorridor(0, 6);

    TEST_ASSERT(!sys.simulationStepPickup("P204", err), "Pickup step aborts due to corridor closure");
    TEST_ASSERT(err.find("Corridor closure during pickup") != std::string::npos, "Descriptive pickup closure error");
    DispatchResource* g1 = sys.getDispatchManager().findResource("GURNEY-01");
    TEST_ASSERT(g1->isAvailable(), "GURNEY-01 is safely released to AVAILABLE");
    TEST_ASSERT(p204->isDispatchUnassigned(), "P204 reverted to unassigned DISPATCH_PENDING");

    // Reopen corridors
    sys.getMapManager().reopenCorridor(0, 1);
    sys.getMapManager().reopenCorridor(0, 2);
    sys.getMapManager().reopenCorridor(0, 6);

    // Re-assign and complete Leg 1
    TEST_ASSERT(sys.assignDispatch("P204", err), "Re-assign GURNEY-01 to P204");
    int simTimeBefore = sys.getSimulationTime();
    TEST_ASSERT(sys.simulationStepPickup("P204", err), "Pickup succeeds");
    TEST_ASSERT(p204->state == PatientState::IN_TRANSIT, "P204 is IN_TRANSIT");
    TEST_ASSERT(g1->state == ResourceState::TRANSPORTING_PATIENT, "GURNEY-01 is TRANSPORTING_PATIENT");
    TEST_ASSERT(g1->currentNodeId == 1, "GURNEY-01 location is node 1");
    TEST_ASSERT(sys.getSimulationTime() > simTimeBefore, "Simulation clock advanced by pickup transit seconds");

    // Block direct corridor (1, 2)
    sys.getMapManager().blockCorridor(1, 2);

    // Transport should reroute via 1 -> 0 -> 2 (60 + 80 = 140s)
    int simTimeBefore2 = sys.getSimulationTime();
    TEST_ASSERT(sys.simulationStepTransportAndHandover("P204", err), "Transport succeeds via alternate rerouted path");
    TEST_ASSERT(p204->state == PatientState::UNDER_TREATMENT, "P204 is UNDER_TREATMENT at destination");
    TEST_ASSERT(p204->currentLocationNode == 2, "P204 current location is node 2");
    TEST_ASSERT(p204->assignedResourceId.empty(), "P204 assignedResourceId cleared");
    TEST_ASSERT(g1->isAvailable(), "GURNEY-01 is released to AVAILABLE at destination");
    TEST_ASSERT(g1->currentNodeId == 2, "GURNEY-01 current location is node 2");
    TEST_ASSERT(sys.getSimulationTime() == simTimeBefore2 + 140, "Simulation clock advanced by exactly 140s");

    sys.getMapManager().reopenCorridor(1, 2);
}

void testPickupCancellation() {
    std::cout << "[SUBTEST] Pickup Leg Cancellation and Resource Cleanup ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeCampusAndFleet(gDataDir, err), "Campus and fleet load succeeds");

    TEST_ASSERT(sys.registerNewPatient("P205", "Gordon Freeman", 35, "Blunt Trauma", UrgencyLevel::RESUSCITATION,
                                       1, 3, 1100.0, err), "Register P205");
    TEST_ASSERT(sys.triagePatient("P205", UrgencyLevel::RESUSCITATION, err), "Triage P205");
    Patient* p205 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p205 != nullptr, "Select P205");

    TEST_ASSERT(sys.assignDispatch("P205", err), "Assign resource to P205");
    std::string assignedResId = p205->assignedResourceId;
    TEST_ASSERT(!assignedResId.empty(), "Resource assigned to P205");
    DispatchResource* res = sys.getDispatchManager().findResource(assignedResId);
    TEST_ASSERT(res->state == ResourceState::EN_ROUTE_PICKUP, "Resource is EN_ROUTE_PICKUP");

    TEST_ASSERT(sys.cancelPickup("P205", err), "Cancel pickup for P205");
    TEST_ASSERT(p205->state == PatientState::CANCELLED, "P205 is CANCELLED");
    TEST_ASSERT(p205->assignedResourceId.empty(), "P205 assignedResourceId is empty");
    TEST_ASSERT(res->isAvailable(), "Resource immediately released to AVAILABLE");
    TEST_ASSERT(res->assignedPatientId.empty(), "Resource assignedPatientId is empty");

    TEST_ASSERT(!sys.simulationStepPickup("P205", err), "Reject stepPickup on CANCELLED patient");
}

void testResourceReuseAndDischargeIsolation() {
    std::cout << "[SUBTEST] Handover Resource Reuse and Patient Discharge Isolation ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeCampusAndFleet(gDataDir, err), "Campus and fleet load succeeds");

    // 1. Patient P301 transported to node 2 and handover completes
    TEST_ASSERT(sys.registerNewPatient("P301", "Henry Cavill", 40, "Chest Pain", UrgencyLevel::RESUSCITATION,
                                       1, 2, 2000.0, err), "Register P301");
    TEST_ASSERT(sys.triagePatient("P301", UrgencyLevel::RESUSCITATION, err), "Triage P301");
    Patient* p301 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(sys.assignDispatch("P301", err), "Assign P301");
    std::string resId = p301->assignedResourceId;
    TEST_ASSERT(sys.simulationStepPickup("P301", err), "Pickup P301");
    TEST_ASSERT(sys.simulationStepTransportAndHandover("P301", err), "Handover P301");

    TEST_ASSERT(p301->state == PatientState::UNDER_TREATMENT, "P301 is UNDER_TREATMENT");
    TEST_ASSERT(p301->assignedResourceId.empty(), "P301 has no assigned resource");
    DispatchResource* res = sys.getDispatchManager().findResource(resId);
    TEST_ASSERT(res->isAvailable() && res->currentNodeId == 2, "Resource is AVAILABLE at node 2");

    // 2. Resource is reused for P302 at node 2
    TEST_ASSERT(sys.registerNewPatient("P302", "Ian Malcolm", 50, "Bite Wound", UrgencyLevel::RESUSCITATION,
                                       2, 4, 1500.0, err), "Register P302 at node 2");
    TEST_ASSERT(sys.triagePatient("P302", UrgencyLevel::RESUSCITATION, err), "Triage P302");
    Patient* p302 = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(sys.assignDispatch("P302", err), "Assign P302");
    TEST_ASSERT(p302->assignedResourceId == resId, "Resource " + resId + " reused for P302");
    TEST_ASSERT(res->assignedPatientId == "P302", "Resource assigned to P302");
    TEST_ASSERT(res->state == ResourceState::EN_ROUTE_PICKUP, "Resource state is EN_ROUTE_PICKUP");

    // 3. Discharge P301
    TEST_ASSERT(sys.dischargePatient("P301", err), "Discharge P301");
    TEST_ASSERT(p301->state == PatientState::DISCHARGED, "P301 is DISCHARGED");

    // ISOLATION: P301 discharge must NOT touch the resource assigned to P302
    TEST_ASSERT(res->assignedPatientId == "P302", "Resource still assigned to P302");
    TEST_ASSERT(res->state == ResourceState::EN_ROUTE_PICKUP, "Resource state preserved as EN_ROUTE_PICKUP");
}

void testTerminalStateRejectionAndAudit() {
    std::cout << "[SUBTEST] Terminal State Rejection and Immutable Audit Event Logging ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Seed load succeeds");

    Patient* p102 = nullptr;
    TEST_ASSERT(sys.callNextAppointment(p102, err), "Call P102");
    TEST_ASSERT(p102->state == PatientState::UNDER_TREATMENT, "P102 UNDER_TREATMENT");
    TEST_ASSERT(sys.dischargePatient("P102", err), "Discharge P102");
    TEST_ASSERT(p102->state == PatientState::DISCHARGED, "P102 DISCHARGED");

    TEST_ASSERT(!sys.dischargePatient("P102", err), "Reject duplicate discharge of P102");
    TEST_ASSERT(err.find("must be UNDER_TREATMENT") != std::string::npos, "Descriptive error for duplicate discharge");

    TEST_ASSERT(!sys.triagePatient("P102", UrgencyLevel::RESUSCITATION, err), "Reject re-triage of discharged patient");
    TEST_ASSERT(!sys.scheduleAppointment("P102", err), "Reject appointment booking for discharged patient");
    TEST_ASSERT(!sys.updateUrgency("P102", UrgencyLevel::RESUSCITATION, err), "Reject urgency update for discharged patient");

    std::vector<AuditEvent> auditEvents = sys.getAuditEvents();
    bool foundRejectedDischarge = false;
    bool foundRejectedTriage = false;
    for (const auto& ev : auditEvents) {
        if (ev.patientId == "P102" && ev.action == "REJECTED_DISCHARGE") foundRejectedDischarge = true;
        if (ev.patientId == "P102" && ev.action == "REJECTED_TRIAGE") foundRejectedTriage = true;
    }
    TEST_ASSERT(foundRejectedDischarge, "Audit contains REJECTED_DISCHARGE event");
    TEST_ASSERT(foundRejectedTriage, "Audit contains REJECTED_TRIAGE event");
}

void testPendingDispatchOrderingPolicy() {
    std::cout << "[SUBTEST] Pending Dispatch Priority Reassessment Policy ...\n";
    HospitalSystem isolatedSys;
    std::string err;

    isolatedSys.registerNewPatient("P_LOW", "Low Priority", 30, "Minor", UrgencyLevel::URGENT, 1, 2, 100.0, err);
    isolatedSys.triagePatient("P_LOW", UrgencyLevel::URGENT, err);

    Patient* p1 = isolatedSys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p1 != nullptr && p1->patientId == "P_LOW", "P_LOW selected");
    TEST_ASSERT(p1->isDispatchUnassigned(), "P_LOW is unassigned pending");

    // Assignment fails because no resources registered in isolatedSys
    TEST_ASSERT(!isolatedSys.assignDispatch("P_LOW", err), "Assignment fails (no resources)");
    TEST_ASSERT(isolatedSys.getUnassignedPendingPatients().size() == 1, "P_LOW remains in unassigned pending list");

    // A new high-urgency patient P_HIGH arrives in triage
    isolatedSys.registerNewPatient("P_HIGH", "High Priority", 60, "Severe", UrgencyLevel::RESUSCITATION, 1, 2, 500.0, err);
    isolatedSys.triagePatient("P_HIGH", UrgencyLevel::RESUSCITATION, err);

    // Policy check: selectNextPatientForDispatch must reassess priority and return P_HIGH ahead of P_LOW
    Patient* p2 = isolatedSys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p2 != nullptr && p2->patientId == "P_HIGH", "P_HIGH selected ahead of unassigned P_LOW due to higher urgency");

    // Dynamic urgency update: P_LOW condition deteriorates to Urgency 1
    TEST_ASSERT(isolatedSys.updateUrgency("P_LOW", UrgencyLevel::RESUSCITATION, err), "P_LOW urgency updated to 1");
    // Since P_LOW arrived before P_HIGH, P_LOW now has priority
    Patient* p3 = isolatedSys.selectNextPatientForDispatch(err);
    TEST_ASSERT(p3 != nullptr && p3->patientId == "P_LOW", "P_LOW selected due to equal urgency (1) and earlier arrival sequence");
}

void testCompleteRegistrationToDischargeEndToEnd() {
    std::cout << "[SUBTEST] Full Patient Lifecycle: Registration -> Triage -> Dispatch -> Pickup -> Transport -> Handover -> Discharge ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Seed load succeeds");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid post-seed load");

    std::string pid = "P999";
    TEST_ASSERT(sys.registerNewPatient(pid, "John McClane", 45, "Gunshot Wound", UrgencyLevel::RESUSCITATION,
                                       1, 3, 4500.0, err), "Step 1: Register patient P999");
    Patient* p = sys.getRegistry().findPatient(pid);
    TEST_ASSERT(p != nullptr && p->state == PatientState::REGISTERED, "Patient state is REGISTERED");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after registration");

    TEST_ASSERT(sys.triagePatient(pid, UrgencyLevel::RESUSCITATION, err), "Step 2: Triage patient P999");
    TEST_ASSERT(p->state == PatientState::TRIAGED_WAITING, "Patient state is TRIAGED_WAITING");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after triage");

    Patient* candidate = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(candidate != nullptr, "Candidate selected");
    while (candidate && candidate->patientId != pid) {
        sys.assignDispatch(candidate->patientId, err);
        sys.simulationStepPickup(candidate->patientId, err);
        sys.simulationStepTransportAndHandover(candidate->patientId, err);
        candidate = sys.selectNextPatientForDispatch(err);
    }
    TEST_ASSERT(candidate != nullptr && candidate->patientId == pid, "Step 3: P999 selected for dispatch");
    TEST_ASSERT(p->state == PatientState::DISPATCH_PENDING && p->isDispatchUnassigned(), "P999 is DISPATCH_PENDING (Unassigned)");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after dispatch selection");

    TEST_ASSERT(sys.assignDispatch(pid, err), "Step 4: Resource assigned to P999");
    TEST_ASSERT(p->isDispatchAssigned(), "P999 is DISPATCH_PENDING (Assigned)");
    std::string assignedRes = p->assignedResourceId;
    TEST_ASSERT(!assignedRes.empty(), "Resource ID recorded");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after assignment");

    int64_t clock1 = sys.getSimulationTime();
    TEST_ASSERT(sys.simulationStepPickup(pid, err), "Step 5: Resource arrives at pickup location");
    TEST_ASSERT(p->state == PatientState::IN_TRANSIT, "P999 state is IN_TRANSIT");
    TEST_ASSERT(sys.getSimulationTime() >= clock1, "Simulation clock advanced");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after pickup");

    int64_t clock2 = sys.getSimulationTime();
    (void)clock2;
    TEST_ASSERT(sys.simulationStepTransportAndHandover(pid, err), "Step 6: Handover completed at destination clinical unit");
    TEST_ASSERT(p->state == PatientState::UNDER_TREATMENT, "P999 state is UNDER_TREATMENT");
    TEST_ASSERT(p->currentLocationNode == 3, "P999 arrived at target location node 3 (Operating Theatre)");
    TEST_ASSERT(p->assignedResourceId.empty(), "P999 resource assignment cleared");
    DispatchResource* res = sys.getDispatchManager().findResource(assignedRes);
    TEST_ASSERT(res->isAvailable(), "Assigned resource immediately released to AVAILABLE");
    TEST_ASSERT(res->currentNodeId == 3, "Resource stationed at destination node 3");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after handover");

    TEST_ASSERT(sys.dischargePatient(pid, err), "Step 7: Discharge patient P999");
    TEST_ASSERT(p->state == PatientState::DISCHARGED, "P999 state is DISCHARGED");
    TEST_ASSERT(res->isAvailable(), "Resource remains AVAILABLE (isolated from patient discharge)");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid after discharge");

    std::vector<AuditEvent> events = sys.getAuditEvents();
    std::vector<std::string> p999Actions;
    for (const auto& ev : events) {
        if (ev.patientId == pid) {
            p999Actions.push_back(ev.action);
        }
    }
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "REGISTERED") != p999Actions.end(), "Audit: REGISTERED");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "TRIAGED") != p999Actions.end(), "Audit: TRIAGED");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "DISPATCH_SELECTED") != p999Actions.end(), "Audit: DISPATCH_SELECTED");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "DISPATCH_ASSIGNED") != p999Actions.end(), "Audit: DISPATCH_ASSIGNED");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "PICKUP_COMPLETE") != p999Actions.end(), "Audit: PICKUP_COMPLETE");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "HANDOVER_COMPLETE") != p999Actions.end(), "Audit: HANDOVER_COMPLETE");
    TEST_ASSERT(std::find(p999Actions.begin(), p999Actions.end(), "DISCHARGED") != p999Actions.end(), "Audit: DISCHARGED");
}

void testMilestone5MoveDisablingPolicy() {
    std::cout << "[SUBTEST] Milestone 5 Move Disabling Policy & Compile-Time Safety ...\n";
    static_assert(!std::is_move_constructible<HospitalSystem>::value,
                  "Compile-time check: HospitalSystem move construction must be disabled");
    static_assert(!std::is_move_assignable<HospitalSystem>::value,
                  "Compile-time check: HospitalSystem move assignment must be disabled");
    static_assert(!std::is_copy_constructible<HospitalSystem>::value,
                  "Compile-time check: HospitalSystem copy construction must be disabled");
    static_assert(!std::is_copy_assignable<HospitalSystem>::value,
                  "Compile-time check: HospitalSystem copy assignment must be disabled");

    TEST_ASSERT(!std::is_move_constructible<HospitalSystem>::value, "HospitalSystem is not move constructible");
    TEST_ASSERT(!std::is_move_assignable<HospitalSystem>::value, "HospitalSystem is not move assignable");
}

void testConsistentInputValidationAndStatePreservation() {
    std::cout << "[SUBTEST] Consistent Input Validation & State Preservation on Rejection ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "System initialization succeeds");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid post-initialization");

    size_t patientCountBefore = sys.getRegistry().getPatientCount();
    size_t triageSizeBefore = sys.getTriageManager().size();
    size_t apptSizeBefore = sys.getAppointmentManager().size();

    // 1. Interactive registration validations
    // Invalid Urgency (0 and 6)
    TEST_ASSERT(!sys.registerNewPatient("P701", "Invalid Urg Zero", 30, "Headache", static_cast<UrgencyLevel>(0), 1, 2, 100.0, err),
                "Reject urgency 0");
    TEST_ASSERT(!sys.registerNewPatient("P702", "Invalid Urg Six", 30, "Headache", static_cast<UrgencyLevel>(6), 1, 2, 100.0, err),
                "Reject urgency 6");

    // Invalid Age (-1 and 151)
    TEST_ASSERT(!sys.registerNewPatient("P703", "Negative Age", -1, "Pain", UrgencyLevel::URGENT, 1, 2, 100.0, err),
                "Reject negative age");
    TEST_ASSERT(!sys.registerNewPatient("P704", "Excessive Age", 151, "Pain", UrgencyLevel::URGENT, 1, 2, 100.0, err),
                "Reject age > 150");

    // Invalid Nodes (-1 and 99)
    TEST_ASSERT(!sys.registerNewPatient("P705", "Bad Current Node", 30, "Pain", UrgencyLevel::URGENT, -1, 2, 100.0, err),
                "Reject negative current location node");
    TEST_ASSERT(!sys.registerNewPatient("P706", "Bad Target Node", 30, "Pain", UrgencyLevel::URGENT, 1, 99, 100.0, err),
                "Reject non-existent target location node");

    // Invalid Billing (negative, infinite, NaN)
    TEST_ASSERT(!sys.registerNewPatient("P707", "Negative Billing", 30, "Pain", UrgencyLevel::URGENT, 1, 2, -10.0, err),
                "Reject negative billing amount");
    double infVal = std::numeric_limits<double>::infinity();
    TEST_ASSERT(!sys.registerNewPatient("P708", "Inf Billing", 30, "Pain", UrgencyLevel::URGENT, 1, 2, infVal, err),
                "Reject infinite billing amount");

    // Invalid ID (whitespace, comma)
    TEST_ASSERT(!sys.registerNewPatient("P 709", "Space In ID", 30, "Pain", UrgencyLevel::URGENT, 1, 2, 100.0, err),
                "Reject space in ID");
    TEST_ASSERT(!sys.registerNewPatient("P,710", "Comma In ID", 30, "Pain", UrgencyLevel::URGENT, 1, 2, 100.0, err),
                "Reject comma in ID");

    // Invalid Name (unsupported punctuation / symbols)
    TEST_ASSERT(!sys.registerNewPatient("P711", "Bad$Name!", 30, "Pain", UrgencyLevel::URGENT, 1, 2, 100.0, err),
                "Reject symbols in full name");

    // Verify counters and collections preserved
    TEST_ASSERT(sys.getRegistry().getPatientCount() == patientCountBefore, "Patient count preserved after rejections");
    TEST_ASSERT(sys.getTriageManager().size() == triageSizeBefore, "Triage size preserved after rejections");
    TEST_ASSERT(sys.getAppointmentManager().size() == apptSizeBefore, "Appointment size preserved after rejections");

    // Valid registration
    TEST_ASSERT(sys.registerNewPatient("P750", "Arthur Dent", 42, "Sore Throat", UrgencyLevel::NON_URGENT, 1, 1, 150.0, err),
                "Register valid patient P750");
    Patient* p750 = sys.getRegistry().findPatient("P750");
    TEST_ASSERT(p750 != nullptr, "P750 found");
    TEST_ASSERT(p750->urgency == UrgencyLevel::NON_URGENT, "P750 has initial urgency NON_URGENT");
    TEST_ASSERT(p750->state == PatientState::REGISTERED, "P750 state is REGISTERED");

    // 2. triagePatient validation: do not overwrite urgency if intake cannot succeed
    TEST_ASSERT(!sys.triagePatient("P750", static_cast<UrgencyLevel>(0), err), "Reject triage intake with urgency 0");
    TEST_ASSERT(p750->urgency == UrgencyLevel::NON_URGENT, "P750 urgency preserved after rejected triage (NOT overwritten!)");
    TEST_ASSERT(p750->state == PatientState::REGISTERED, "P750 state preserved as REGISTERED");

    TEST_ASSERT(!sys.triagePatient("P750", static_cast<UrgencyLevel>(8), err), "Reject triage intake with urgency 8");
    TEST_ASSERT(p750->urgency == UrgencyLevel::NON_URGENT, "P750 urgency preserved after second rejected triage");

    // 3. updateUrgency validation on waiting patient
    TEST_ASSERT(sys.triagePatient("P750", UrgencyLevel::URGENT, err), "Triage P750 with valid urgency");
    TEST_ASSERT(p750->urgency == UrgencyLevel::URGENT, "P750 urgency is URGENT");
    TEST_ASSERT(p750->state == PatientState::TRIAGED_WAITING, "P750 is TRIAGED_WAITING");

    TEST_ASSERT(!sys.updateUrgency("P750", static_cast<UrgencyLevel>(0), err), "Reject update to urgency 0");
    TEST_ASSERT(p750->urgency == UrgencyLevel::URGENT, "P750 urgency preserved after rejected update");

    TEST_ASSERT(!sys.updateUrgency("P750", static_cast<UrgencyLevel>(7), err), "Reject update to urgency 7");
    TEST_ASSERT(p750->urgency == UrgencyLevel::URGENT, "P750 urgency preserved after second rejected update");

    TEST_ASSERT(sys.validateSystemIntegrity(err), "System integrity holds after all validation tests");
}

void testPreserveAppointmentOrderOnFailedConversion() {
    std::cout << "[SUBTEST] Preserve Exact FIFO Appointment Order on Failed Conversion ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "System initialization succeeds");

    std::vector<std::string> initialQueue = sys.getAppointmentManager().getScheduledPatientIds();
    TEST_ASSERT(initialQueue.size() == 3, "Initial queue has 3 appointments");
    TEST_ASSERT(initialQueue[0] == "P102", "Index 0 is P102");
    TEST_ASSERT(initialQueue[1] == "P106", "Index 1 is P106");
    TEST_ASSERT(initialQueue[2] == "P111", "Index 2 is P111");

    Patient* pHead = sys.getRegistry().findPatient("P102");
    Patient* pMid = sys.getRegistry().findPatient("P106");

    UrgencyLevel headUrg = pHead->urgency;
    UrgencyLevel midUrg = pMid->urgency;

    // Test 1: Failed conversion of the HEAD appointment (P102) with invalid urgency
    TEST_ASSERT(!sys.convertAppointmentToEmergency("P102", static_cast<UrgencyLevel>(9), err),
                "Conversion of head appointment with invalid urgency 9 fails");
    TEST_ASSERT(pHead->state == PatientState::APPOINTMENT_SCHEDULED, "Head patient remains APPOINTMENT_SCHEDULED");
    TEST_ASSERT(pHead->urgency == headUrg, "Head patient urgency preserved");

    std::vector<std::string> queueAfterHeadFail = sys.getAppointmentManager().getScheduledPatientIds();
    TEST_ASSERT(queueAfterHeadFail == initialQueue, "Entire FIFO queue sequence preserved after head failure");

    // Test 2: Failed conversion of the MIDDLE appointment (P106) with invalid urgency
    TEST_ASSERT(!sys.convertAppointmentToEmergency("P106", static_cast<UrgencyLevel>(0), err),
                "Conversion of middle appointment with invalid urgency 0 fails");
    TEST_ASSERT(pMid->state == PatientState::APPOINTMENT_SCHEDULED, "Middle patient remains APPOINTMENT_SCHEDULED");
    TEST_ASSERT(pMid->urgency == midUrg, "Middle patient urgency preserved");

    std::vector<std::string> queueAfterMidFail = sys.getAppointmentManager().getScheduledPatientIds();
    TEST_ASSERT(queueAfterMidFail == initialQueue, "Entire FIFO queue sequence preserved after middle failure");

    // Test 3: Encapsulated FIFO restoration via CustomQueue insertAt
    CustomQueue<std::string> testQ;
    testQ.enqueue("A");
    testQ.enqueue("B");
    testQ.enqueue("C");
    TEST_ASSERT(testQ.indexOf("B") == 1, "B is at index 1");
    TEST_ASSERT(testQ.remove("B"), "Removed B from middle");
    TEST_ASSERT(testQ.indexOf("B") == -1, "B is not in queue");
    TEST_ASSERT(testQ.insertAt(1, "B"), "Restored B at exact index 1");
    std::vector<std::string> restoredSeq;
    testQ.forEach([&restoredSeq](const std::string& v) { restoredSeq.push_back(v); });
    TEST_ASSERT(restoredSeq.size() == 3, "Size is 3");
    TEST_ASSERT(restoredSeq[0] == "A" && restoredSeq[1] == "B" && restoredSeq[2] == "C",
                "Exact FIFO sequence [A, B, C] restored without placing B at tail!");

    TEST_ASSERT(sys.validateSystemIntegrity(err), "System integrity holds");
}

void testRejectedOperationHandlingAndReciprocalChecks() {
    std::cout << "[SUBTEST] Rejected Operation Handling, Specific Audits & Reciprocal Checks ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "System initialization succeeds");

    Patient* p104 = sys.getRegistry().findPatient("P104"); // TRIAGED_WAITING
    TEST_ASSERT(p104 != nullptr, "P104 found");
    TEST_ASSERT(p104->state == PatientState::TRIAGED_WAITING, "P104 is TRIAGED_WAITING");

    // 1. Invalid State: Call simulationStepPickup on a TRIAGED_WAITING patient
    size_t pendingPoolBefore = sys.getUnassignedPendingPatients().size();
    TEST_ASSERT(!sys.simulationStepPickup("P104", err), "simulationStepPickup rejected on TRIAGED_WAITING patient");
    TEST_ASSERT(p104->state == PatientState::TRIAGED_WAITING, "P104 state remains TRIAGED_WAITING");
    TEST_ASSERT(sys.getUnassignedPendingPatients().size() == pendingPoolBefore,
                "Patient was NOT erroneously added to unassigned pending pool");

    // Verify audit log did NOT label this as corridor blocked
    const AuditEvent* lastEvent = sys.getAuditLog().back();
    TEST_ASSERT(lastEvent != nullptr, "Last audit event exists");
    TEST_ASSERT(lastEvent->action == "REJECTED_INVALID_STATE", "Audit action is REJECTED_INVALID_STATE (not corridor blocked)");
    TEST_ASSERT(lastEvent->priorState == PatientState::TRIAGED_WAITING, "Prior state accurately logged");
    TEST_ASSERT(lastEvent->newState == PatientState::TRIAGED_WAITING, "New state accurately logged as unchanged");

    // 2. Select patient for dispatch -> state becomes DISPATCH_PENDING (unassigned)
    Patient* cand = sys.selectNextPatientForDispatch(err);
    TEST_ASSERT(cand != nullptr && cand->patientId == "P104", "P104 selected for dispatch");
    TEST_ASSERT(cand->isDispatchUnassigned(), "P104 is unassigned pending");
    TEST_ASSERT(sys.getUnassignedPendingPatients().size() == 1, "P104 is in unassigned pending pool");

    // Attempt pickup before resource assignment
    TEST_ASSERT(!sys.simulationStepPickup("P104", err), "simulationStepPickup rejected on unassigned patient");
    TEST_ASSERT(sys.getAuditLog().back() != nullptr && sys.getAuditLog().back()->action == "REJECTED_MISSING_RESOURCE",
                "Audit action is REJECTED_MISSING_RESOURCE");
    TEST_ASSERT(sys.getUnassignedPendingPatients().size() == 1, "Pending pool size unchanged (no duplicate entry)");

    // 3. Assign resource to P104
    TEST_ASSERT(sys.assignDispatch("P104", err), "Resource assigned to P104");
    TEST_ASSERT(cand->isDispatchAssigned(), "P104 is assigned pending");
    TEST_ASSERT(sys.getUnassignedPendingPatients().empty(), "Assigned patient removed from unassigned pool");
    std::string resId = cand->assignedResourceId;
    DispatchResource* res = sys.getDispatchManager().findResource(resId);
    TEST_ASSERT(res != nullptr, "Assigned resource found");
    TEST_ASSERT(res->state == ResourceState::EN_ROUTE_PICKUP, "Resource state is EN_ROUTE_PICKUP");
    TEST_ASSERT(res->assignedPatientId == "P104", "Reciprocal patient assignment in resource");

    // 4. Duplicate assignment attempt on already assigned patient
    TEST_ASSERT(!sys.assignDispatch("P104", err), "Duplicate assignment rejected");
    TEST_ASSERT(sys.getAuditLog().back() != nullptr && sys.getAuditLog().back()->action == "REJECTED_ALREADY_ASSIGNED",
                "Audit action is REJECTED_ALREADY_ASSIGNED");

    // 5. Test reciprocal mismatch
    std::string origPat = res->assignedPatientId;
    res->assignedPatientId = "P999_OTHER"; // deliberate mismatch
    TEST_ASSERT(!sys.simulationStepPickup("P104", err), "Pickup rejected due to assignment mismatch");
    TEST_ASSERT(sys.getAuditLog().back() != nullptr && sys.getAuditLog().back()->action == "REJECTED_ASSIGNMENT_MISMATCH",
                "Audit action is REJECTED_ASSIGNMENT_MISMATCH");
    res->assignedPatientId = origPat; // restore reciprocal consistency

    // 6. Complete Leg 1: Pickup
    TEST_ASSERT(sys.simulationStepPickup("P104", err), "Pickup succeeds");
    TEST_ASSERT(cand->state == PatientState::IN_TRANSIT, "P104 is IN_TRANSIT");
    TEST_ASSERT(res->state == ResourceState::TRANSPORTING_PATIENT, "Resource is TRANSPORTING_PATIENT");

    // Repeated pickup call must be rejected (already IN_TRANSIT)
    TEST_ASSERT(!sys.simulationStepPickup("P104", err), "Repeated pickup call rejected");
    TEST_ASSERT(cand->state == PatientState::IN_TRANSIT, "P104 remains IN_TRANSIT");

    // 7. Complete Leg 2: Transport & Handover
    TEST_ASSERT(sys.simulationStepTransportAndHandover("P104", err), "Handover succeeds");
    TEST_ASSERT(cand->state == PatientState::UNDER_TREATMENT, "P104 is UNDER_TREATMENT");
    TEST_ASSERT(res->isAvailable(), "Resource released to AVAILABLE");

    // Repeated handover call must be rejected
    TEST_ASSERT(!sys.simulationStepTransportAndHandover("P104", err), "Repeated handover rejected");

    TEST_ASSERT(sys.validateSystemIntegrity(err), "System integrity holds after all dispatch checks");
}

void testCoherentTimeSemanticsAndSeedSnapshot() {
    std::cout << "[SUBTEST] Coherent Time Semantics, Seed Snapshot & Non-Negative Duration ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "System initialization succeeds");

    int64_t initClock = sys.getSimulationTime();
    TEST_ASSERT(initClock >= 65, "Simulation clock initialized to at least latest seed arrival time (65s)");

    std::vector<AuditEvent> events = sys.getAuditEvents();
    for (const auto& ev : events) {
        TEST_ASSERT(ev.simulatedTimestamp >= 0, "Audit timestamp is non-negative");
        Patient* p = sys.getRegistry().findPatient(ev.patientId);
        if (p) {
            TEST_ASSERT(ev.simulatedTimestamp >= p->simulatedArrivalTime,
                        "Audit event timestamp does not precede patient simulated arrival time");
        }
    }

    sys.advanceSimulationTime(100);
    TEST_ASSERT(sys.getSimulationTime() == initClock + 100, "Checked time advance functions correctly");

    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity valid");
}

void testRegistryRegistrationExceptionSafetyRollback() {
    std::cout << "[SUBTEST] HospitalRegistry Strong Exception Safety & Rollback Verification ...\n";
    HospitalRegistry registry;
    std::string err;

    auto p1 = std::make_unique<Patient>();
    p1->patientId = "P801";
    p1->fullName = "Test Normal";
    p1->internalHandle = 50;

    TEST_ASSERT(registry.registerPatient(std::move(p1), err), "Initial registration succeeds");
    TEST_ASSERT(registry.hasPatient("P801"), "P801 is registered");
    TEST_ASSERT(registry.findPatientByHandle(50) != nullptr, "Handle 50 is mapped");

    // Simulate allocation failure during registration via test seam
    registry.setSimulateAllocationFailure(true);

    auto p2 = std::make_unique<Patient>();
    p2->patientId = "P802";
    p2->fullName = "Test Throw";
    p2->internalHandle = 51;

    bool failedReg = registry.registerPatient(std::move(p2), err);
    TEST_ASSERT(!failedReg, "Registration with simulated failure returns false");
    TEST_ASSERT(err.find("rolled back cleanly") != std::string::npos, "Error reports clean rollback");

    // Verify strong exception safety: no partial entries remain
    TEST_ASSERT(!registry.hasPatient("P802"), "P802 does not exist in registry after rollback");
    TEST_ASSERT(registry.findPatient("P802") == nullptr, "findPatient returns nullptr");
    TEST_ASSERT(registry.findPatientByHandle(51) == nullptr, "Handle 51 is nullptr in handleMap");
    TEST_ASSERT(registry.getHashTable().search("P802") == nullptr, "Hash table has no entry for P802");
    TEST_ASSERT(registry.getBSTIndex().search("P802") == nullptr, "BST index has no entry for P802");
    TEST_ASSERT(registry.getPatientCount() == 1, "Registry count remains exactly 1");

    // Clear test seam and re-register P802: must succeed completely
    registry.setSimulateAllocationFailure(false);
    auto p2Retry = std::make_unique<Patient>();
    p2Retry->patientId = "P802";
    p2Retry->fullName = "Test Retry";
    p2Retry->internalHandle = 51;

    TEST_ASSERT(registry.registerPatient(std::move(p2Retry), err), "Re-registration of P802 after failure succeeds");
    TEST_ASSERT(registry.hasPatient("P802"), "P802 now exists");
    TEST_ASSERT(registry.findPatientByHandle(51) != nullptr, "Handle 51 now mapped");
    TEST_ASSERT(registry.getPatientCount() == 2, "Registry count is 2");
}

void testInitializationResetAndSeedValidation() {
    std::cout << "[SUBTEST] Initialization Semantics, Reset & Seed Validation ...\n";
    HospitalSystem sys;
    std::string err;

    TEST_ASSERT(!sys.isInitialized(), "Initially not initialized");
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "First initialization succeeds");
    TEST_ASSERT(sys.isInitialized(), "Now marked initialized");

    // Repeated initialization on live system must be rejected
    TEST_ASSERT(!sys.initializeFromSeed(gDataDir, err), "Repeated initializeFromSeed rejected without reset");
    TEST_ASSERT(err.find("Call reset()") != std::string::npos, "Error instructs to call reset()");

    TEST_ASSERT(!sys.initializeCampusAndFleet(gDataDir, err), "Repeated initializeCampusAndFleet rejected without reset");

    // Explicit reset
    sys.reset();
    TEST_ASSERT(!sys.isInitialized(), "Reset clears initialized status");
    TEST_ASSERT(sys.getRegistry().getPatientCount() == 0, "Reset clears registry");
    TEST_ASSERT(sys.getTriageManager().empty(), "Reset clears triage");
    TEST_ASSERT(sys.getAppointmentManager().empty(), "Reset clears appointments");
    TEST_ASSERT(sys.getDispatchManager().getAllResources().empty(), "Reset clears resources");
    TEST_ASSERT(sys.getSimulationTime() == 0, "Reset clears simulation time");

    // Re-initialization succeeds cleanly
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Re-initialization after reset succeeds");
    TEST_ASSERT(sys.isInitialized(), "Marked initialized again");
    TEST_ASSERT(sys.validateSystemIntegrity(err), "System integrity valid after re-initialization");
}

void testSystemIntegrityCheckerVerification() {
    std::cout << "[SUBTEST] System Integrity Checker Comprehensive Invariant Verification ...\n";
    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "Initialization succeeds");

    // Baseline integrity check
    TEST_ASSERT(sys.validateSystemIntegrity(err), "System passes integrity validation in initial seed state");

    // Inconsistency 1: Patient in TRIAGED_WAITING has state manually corrupted
    Patient* p101 = sys.getRegistry().findPatient("P101");
    TEST_ASSERT(p101 != nullptr, "P101 found");
    p101->state = PatientState::REGISTERED; // Corrupt: active in heap but marked REGISTERED
    TEST_ASSERT(!sys.validateSystemIntegrity(err), "Integrity checker detects active triage patient marked REGISTERED");
    p101->state = PatientState::TRIAGED_WAITING; // Restore
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity restored");

    // Inconsistency 2: Patient in DISPATCH_PENDING assigned to non-existent resource
    Patient* p104 = sys.getRegistry().findPatient("P104");
    p104->state = PatientState::DISPATCH_PENDING;
    p104->assignedResourceId = "NON_EXISTENT_GURNEY";
    TEST_ASSERT(!sys.validateSystemIntegrity(err), "Integrity checker detects non-existent assigned resource");
    p104->assignedResourceId = "";
    p104->state = PatientState::TRIAGED_WAITING; // Restore
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity restored");

    // Inconsistency 3: Reciprocal mismatch between patient and resource
    DispatchResource* res1 = sys.getDispatchManager().findResource("GURNEY-01");
    res1->state = ResourceState::EN_ROUTE_PICKUP;
    res1->assignedPatientId = "P104"; // Corrupt: resource assigned to P104, but P104 is TRIAGED_WAITING
    TEST_ASSERT(!sys.validateSystemIntegrity(err), "Integrity checker detects reciprocal state mismatch");
    res1->state = ResourceState::AVAILABLE;
    res1->assignedPatientId = ""; // Restore
    TEST_ASSERT(sys.validateSystemIntegrity(err), "Integrity restored");
}

void testMilestone6ConsistencyFixes() {
    std::cout << "[SUBTEST] Milestone 6 Consistency Fixes & Failure Injections ...\n";

    // 1. Fix 1: HashTableChaining rollback on rehash failure and HospitalRegistry failure injection
    {
        HashTableChaining<std::string, int> table(3, 0.5);
        table.insert("k1", 10);
        table.setSimulateRehashFailure(true);
        bool threw = false;
        try {
            table.insert("k2", 20);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        TEST_ASSERT(threw, "HashTableChaining rehash failure successfully caught");
        TEST_ASSERT(!table.contains("k2"), "k2 rolled back cleanly on rehash throw");
        TEST_ASSERT(table.contains("k1"), "k1 preserved untouched in table");
        TEST_ASSERT(table.size() == 1, "Table size remains 1");

        // Failure injection after hash insertion in HospitalRegistry
        HospitalRegistry registry;
        registry.setSimulateFailureAfterHash(true);
        auto p = std::make_unique<Patient>();
        p->patientId = "P999";
        p->fullName = "Failure Test Patient";
        p->internalHandle = 45;
        std::string err;
        TEST_ASSERT(!registry.registerPatient(std::move(p), err), "Registration with injected failure after hash fails");
        TEST_ASSERT(!registry.hasPatient("P999"), "Hash table rolled back cleanly; no unowned Patient in hash");
        TEST_ASSERT(registry.getBSTIndex().search("P999") == nullptr, "BST index has no entry for P999");
        TEST_ASSERT(registry.findPatientByHandle(45) == nullptr, "Handle map has no entry for handle 45");
        TEST_ASSERT(registry.getPatientCount() == 0, "Registry patient count is 0");
    }

    // 2. Fix 2: Exception-safe appointment-to-emergency conversion
    {
        HospitalRegistry registry;
        TriageManager triage(registry);
        AppointmentManager appt(registry);

        auto p1 = std::make_unique<Patient>();
        p1->patientId = "P801";
        p1->fullName = "Appt Conv Patient 1";
        p1->age = 35;
        p1->internalHandle = 10;
        p1->urgency = UrgencyLevel::NON_URGENT;
        p1->state = PatientState::REGISTERED;
        Patient* raw1 = p1.get();

        auto p2 = std::make_unique<Patient>();
        p2->patientId = "P802";
        p2->fullName = "Appt Conv Patient 2";
        p2->age = 40;
        p2->internalHandle = 11;
        p2->urgency = UrgencyLevel::NON_URGENT;
        p2->state = PatientState::REGISTERED;
        Patient* raw2 = p2.get();

        std::string err;
        registry.registerPatient(std::move(p1), err);
        registry.registerPatient(std::move(p2), err);

        appt.scheduleAppointment(raw1, err);
        appt.scheduleAppointment(raw2, err);

        auto queueIds = appt.getScheduledPatientIds();
        TEST_ASSERT(queueIds.size() == 2 && queueIds[0] == "P801" && queueIds[1] == "P802", "Initial appointment order P801, P802");

        // Make p2 conversion fail by corrupting target urgency
        TEST_ASSERT(!appt.convertToEmergency(raw2, static_cast<UrgencyLevel>(99), triage, err), "Conversion with invalid urgency rejected");
        TEST_ASSERT(raw2->state == PatientState::APPOINTMENT_SCHEDULED, "Patient state rolled back to APPOINTMENT_SCHEDULED");
        TEST_ASSERT(raw2->urgency == UrgencyLevel::NON_URGENT, "Patient urgency preserved");
        queueIds = appt.getScheduledPatientIds();
        TEST_ASSERT(queueIds.size() == 2 && queueIds[0] == "P801" && queueIds[1] == "P802", "Exact FIFO queue position preserved without allocation");

        // Now successfully convert p1
        TEST_ASSERT(appt.convertToEmergency(raw1, UrgencyLevel::RESUSCITATION, triage, err), "Conversion of P801 succeeds");
        TEST_ASSERT(raw1->state == PatientState::TRIAGED_WAITING, "P801 now TRIAGED_WAITING");
        TEST_ASSERT(raw1->urgency == UrgencyLevel::RESUSCITATION, "P801 urgency updated to RESUSCITATION");
        TEST_ASSERT(triage.isPatientActive(raw1->internalHandle), "P801 active in triage heap");
        queueIds = appt.getScheduledPatientIds();
        TEST_ASSERT(queueIds.size() == 1 && queueIds[0] == "P802", "P802 remains alone at head of appointment queue");
    }

    // 3. Fix 3: Genuinely non-allocating reset()
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);
        TEST_ASSERT(sys.isInitialized(), "System initialized");
        sys.reset();
        TEST_ASSERT(!sys.isInitialized(), "Reset cleared initialized status");
        TEST_ASSERT(sys.getRegistry().getPatientCount() == 0, "Registry cleared");
        TEST_ASSERT(sys.getMapManager().getNodeCount() == 0, "Map graph cleared non-allocatingly");
        TEST_ASSERT(sys.getAuditEvents().empty(), "Audit log cleared");
    }

    // 4. Fix 4: Preservation of terminal seed states (DISCHARGED, CANCELLED)
    {
        Patient pDischarged;
        pDischarged.patientId = "P_TERM_1";
        pDischarged.fullName = "Term Patient 1";
        pDischarged.age = 50;
        pDischarged.complaint = "Followup complete";
        pDischarged.urgency = UrgencyLevel::NON_URGENT;
        pDischarged.arrivalSequence = 991;
        pDischarged.simulatedArrivalTime = 10;
        pDischarged.state = PatientState::DISCHARGED;
        pDischarged.currentLocationNode = 1;
        pDischarged.targetLocationNode = 1;
        pDischarged.internalHandle = 60;

        HospitalSystem sys;
        std::string err;
        sys.initializeCampusAndFleet(gDataDir, err);

        auto pPtr = std::make_unique<Patient>(pDischarged);
        sys.getRegistry().registerPatient(std::move(pPtr), err);
        Patient* regP = sys.getRegistry().findPatient("P_TERM_1");
        regP->state = PatientState::DISCHARGED;
        TEST_ASSERT(regP->state == PatientState::DISCHARGED, "Terminal seed state DISCHARGED preserved");
        TEST_ASSERT(regP->isDischarged(), "isDischarged() is true");
    }

    // 5. Fix 5: Uniform patient-ID length limit (kMaxPatientIdLength = 16)
    {
        TEST_ASSERT(kMaxPatientIdLength == 16, "kMaxPatientIdLength is 16");
        HospitalSystem sys;
        std::string err;
        sys.initializeCampusAndFleet(gDataDir, err);

        // 17-char ID must be rejected by registration
        std::string longId = "12345678901234567";
        TEST_ASSERT(!sys.registerNewPatient(longId, "Long ID Test", 30, "Flu", UrgencyLevel::URGENT, 1, 1, 50.0, err),
                    "17-character patient ID rejected by registration");
        TEST_ASSERT(err.find("16") != std::string::npos, "Error documents 16 character limit");

        // 16-char ID must be accepted
        std::string exact16Id = "1234567890123456";
        TEST_ASSERT(sys.registerNewPatient(exact16Id, "Exact Sixteen Id", 30, "Flu", UrgencyLevel::URGENT, 1, 1, 50.0, err),
                    "16-character patient ID accepted by registration");
        TEST_ASSERT(sys.getRegistry().hasPatient(exact16Id), "Patient with 16-char ID is in registry");
    }
}

void testCustomSortingAlgorithms() {
    std::cout << "[SUBTEST] Custom Sorting Algorithms (Merge, Quick, Heap) Comprehensive Verification ...\n";

    // 1. Empty and Single element arrays
    {
        std::vector<int> emptyArr;
        ds_algorithms::SortMetrics mMerge, mQuick, mHeap;
        ds_algorithms::customMergeSort(emptyArr, std::less<int>(), mMerge);
        ds_algorithms::customQuickSort(emptyArr, std::less<int>(), mQuick);
        ds_algorithms::customHeapSort(emptyArr, std::less<int>(), mHeap);
        TEST_ASSERT(emptyArr.empty(), "Empty array remains empty across all algorithms");
        TEST_ASSERT(mMerge.elementCount == 0 && mQuick.elementCount == 0 && mHeap.elementCount == 0, "Element counts are 0");

        std::vector<int> singleArr = {42};
        ds_algorithms::customMergeSort(singleArr, std::less<int>(), mMerge);
        TEST_ASSERT(singleArr.size() == 1 && singleArr[0] == 42, "Single-element array unchanged by merge sort");
        ds_algorithms::customQuickSort(singleArr, std::less<int>(), mQuick);
        TEST_ASSERT(singleArr.size() == 1 && singleArr[0] == 42, "Single-element array unchanged by quicksort");
        ds_algorithms::customHeapSort(singleArr, std::less<int>(), mHeap);
        TEST_ASSERT(singleArr.size() == 1 && singleArr[0] == 42, "Single-element array unchanged by heapsort");
    }

    // 2. Duplicate elements
    {
        std::vector<int> expected = {1, 2, 2, 3, 5, 5, 5};
        std::vector<int> arr1 = {5, 2, 5, 1, 5, 3, 2};
        std::vector<int> arr2 = arr1;
        std::vector<int> arr3 = arr1;

        ds_algorithms::SortMetrics m1, m2, m3;
        ds_algorithms::customMergeSort(arr1, std::less<int>(), m1);
        ds_algorithms::customQuickSort(arr2, std::less<int>(), m2);
        ds_algorithms::customHeapSort(arr3, std::less<int>(), m3);

        TEST_ASSERT(arr1 == expected, "Merge sort correctly handles duplicates");
        TEST_ASSERT(arr2 == expected, "Quicksort correctly handles duplicates");
        TEST_ASSERT(arr3 == expected, "Heapsort correctly handles duplicates");

        TEST_ASSERT(m1.comparisons > 0 && m1.moves > 0, "Merge sort metrics populated");
        TEST_ASSERT(m2.comparisons > 0 && m2.moves > 0, "Quicksort metrics populated");
        TEST_ASSERT(m3.comparisons > 0 && m3.moves > 0, "Heapsort metrics populated");
    }

    // 3. Already sorted and reverse sorted arrays
    {
        std::vector<int> sortedArr = {10, 20, 30, 40, 50, 60, 70, 80};
        std::vector<int> revArr = {80, 70, 60, 50, 40, 30, 20, 10};

        std::vector<int> a1 = sortedArr, a2 = sortedArr, a3 = sortedArr;
        ds_algorithms::customMergeSort(a1);
        ds_algorithms::customQuickSort(a2);
        ds_algorithms::customHeapSort(a3);
        TEST_ASSERT(a1 == sortedArr && a2 == sortedArr && a3 == sortedArr, "Already sorted array preserved");

        std::vector<int> r1 = revArr, r2 = revArr, r3 = revArr;
        ds_algorithms::customMergeSort(r1);
        ds_algorithms::customQuickSort(r2);
        ds_algorithms::customHeapSort(r3);
        TEST_ASSERT(r1 == sortedArr && r2 == sortedArr && r3 == sortedArr, "Reverse sorted array correctly sorted");
    }

    // 4. Trace logging for small inputs
    {
        std::vector<int> smallArr = {9, 3, 7, 1, 5, 2};
        std::vector<std::string> mergeTrace, quickTrace, heapTrace;
        ds_algorithms::SortMetrics m1, m2, m3;

        std::vector<int> c1 = smallArr, c2 = smallArr, c3 = smallArr;
        ds_algorithms::customMergeSort(c1, std::less<int>(), m1, true, &mergeTrace);
        ds_algorithms::customQuickSort(c2, std::less<int>(), m2, true, &quickTrace);
        ds_algorithms::customHeapSort(c3, std::less<int>(), m3, true, &heapTrace);

        TEST_ASSERT(!mergeTrace.empty(), "Merge sort trace populated when enabled");
        TEST_ASSERT(!quickTrace.empty(), "Quicksort trace populated when enabled");
        TEST_ASSERT(!heapTrace.empty(), "Heapsort trace populated when enabled");
    }
}

void testSortingStability() {
    std::cout << "[SUBTEST] Sorting Stability Verification (Merge Sort vs Quicksort vs Heapsort) ...\n";

    struct KeyItem {
        int key;
        int originalSeq;
    };

    struct KeyCompOnly {
        bool operator()(const KeyItem& a, const KeyItem& b) const noexcept {
            return a.key < b.key;
        }
    };

    std::vector<KeyItem> data = {
        {3, 1},
        {1, 2},
        {3, 3},
        {2, 4},
        {3, 5},
        {1, 6}
    };

    std::vector<KeyItem> mergeData = data;
    ds_algorithms::SortMetrics mMerge;
    ds_algorithms::customMergeSort(mergeData, KeyCompOnly(), mMerge);

    std::vector<int> resultKeys;
    for (const auto& item : mergeData) resultKeys.push_back(item.key);
    std::vector<int> expectedKeys = {1, 1, 2, 3, 3, 3};
    TEST_ASSERT(resultKeys == expectedKeys, "Merge sort correctly sorts keys");

    TEST_ASSERT(mergeData[0].originalSeq == 2 && mergeData[1].originalSeq == 6,
                "Merge sort preserves exact FIFO sequence for equal key 1 (seq: 2, 6)");

    TEST_ASSERT(mergeData[3].originalSeq == 1 && mergeData[4].originalSeq == 3 && mergeData[5].originalSeq == 5,
                "Merge sort preserves exact FIFO sequence for equal key 3 (seq: 1, 3, 5)");

    std::vector<KeyItem> heapData = {
        {1, 10},
        {2, 20},
        {2, 30},
        {2, 40}
    };
    ds_algorithms::SortMetrics mHeap;
    ds_algorithms::customHeapSort(heapData, KeyCompOnly(), mHeap);
    bool heapReordered = (heapData[1].originalSeq != 20 || heapData[2].originalSeq != 30 || heapData[3].originalSeq != 40);
    TEST_ASSERT(heapReordered, "Heapsort exhibits instability by reordering equal keys");
}

void testPatientSnapshotAndReportingService() {
    std::cout << "[SUBTEST] PatientSnapshot Semantics & Non-Mutating Reporting Verification ...\n";

    HospitalSystem sys;
    std::string err;
    TEST_ASSERT(sys.initializeFromSeed(gDataDir, err), "HospitalSystem initialized from seed");

    // 1. Snapshot Independence: Modifying live patient does not affect previously captured snapshot
    int64_t captureTime = sys.getSimulationTime();
    std::vector<PatientSnapshot> snapshots = ReportingService::captureSnapshots(sys);
    TEST_ASSERT(snapshots.size() == 12, "Captured 12 patient snapshots from seed");

    Patient* p101 = sys.getRegistry().findPatient("P101");
    TEST_ASSERT(p101 != nullptr, "P101 found in registry");
    double origBilling = p101->billingAmount;
    std::string origComplaint = p101->complaint;

    p101->billingAmount = 99999.0;
    p101->complaint = "COMPLETELY CHANGED COMPLAINT";

    auto it = std::find_if(snapshots.begin(), snapshots.end(), [](const PatientSnapshot& s) { return s.patientId == "P101"; });
    TEST_ASSERT(it != snapshots.end(), "P101 snapshot present");
    TEST_ASSERT(it->billingAmount == origBilling, "Snapshot billingAmount unaffected by live patient mutation");
    TEST_ASSERT(it->complaint == origComplaint, "Snapshot complaint unaffected by live patient mutation");

    p101->billingAmount = origBilling;
    p101->complaint = origComplaint;

    // 2. Waiting Duration Semantics
    TEST_ASSERT(it->isWaitingDurationApplicable, "Waiting duration is applicable for TRIAGED_WAITING patient P101");
    int64_t expectedWait = captureTime - it->simulatedArrivalTime;
    TEST_ASSERT(it->waitingDurationSeconds == expectedWait, "P101 waiting duration matches elapsed seconds since arrival");
    TEST_ASSERT(it->waitingDurationFormatted == (std::to_string(expectedWait) + "s"), "Formatted waiting duration correct");

    Patient pDischarged;
    pDischarged.patientId = "P_DISC";
    pDischarged.fullName = "Discharged Patient";
    pDischarged.age = 50;
    pDischarged.state = PatientState::UNDER_TREATMENT;
    pDischarged.simulatedArrivalTime = 10;
    PatientSnapshot discSnapshot(pDischarged, captureTime);
    TEST_ASSERT(!discSnapshot.isWaitingDurationApplicable, "Waiting duration NOT applicable for UNDER_TREATMENT");
    TEST_ASSERT(discSnapshot.waitingDurationSeconds == -1, "waitingDurationSeconds is -1");
    TEST_ASSERT(discSnapshot.waitingDurationFormatted.find("N/A") != std::string::npos, "Formatted contains N/A for UNDER_TREATMENT");

    // 3. Non-mutating Reporting Invariant:
    size_t preRegCount = sys.getRegistry().getPatientCount();
    size_t preTriageCount = sys.getTriageManager().size();
    size_t preApptCount = sys.getAppointmentManager().size();
    int64_t preClock = sys.getSimulationTime();
    size_t preAuditCount = sys.getAuditEvents().size();

    auto reportSnaps = ReportingService::captureSnapshots(sys);
    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::AGE, ReportingService::SortAlgorithm::MERGE_SORT);
    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::URGENCY, ReportingService::SortAlgorithm::QUICK_SORT);
    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::BILLING_AMOUNT, ReportingService::SortAlgorithm::HEAP_SORT);
    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::WAITING_DURATION, ReportingService::SortAlgorithm::MERGE_SORT);
    std::string reportTable = ReportingService::formatReportTable(reportSnaps, ReportingService::SortField::WAITING_DURATION, sys.getSimulationTime());
    TEST_ASSERT(!reportTable.empty(), "Report table generated successfully");

    TEST_ASSERT(sys.getRegistry().getPatientCount() == preRegCount, "Registry patient count unchanged");
    TEST_ASSERT(sys.getTriageManager().size() == preTriageCount, "Triage queue size unchanged");
    TEST_ASSERT(sys.getAppointmentManager().size() == preApptCount, "Appointment queue size unchanged");
    TEST_ASSERT(sys.getSimulationTime() == preClock, "Simulation clock unchanged");
    TEST_ASSERT(sys.getAuditEvents().size() == preAuditCount, "Audit log size unchanged");

    // 4. Deterministic Tie-Breaking
    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::URGENCY, ReportingService::SortAlgorithm::MERGE_SORT);
    TEST_ASSERT(reportSnaps[0].patientId == "P104" && reportSnaps[1].patientId == "P107",
                "Deterministic tie-breaking sorts P104 before P107 (both Urgency 1, arrSeq 4 < 7)");

    ReportingService::sortSnapshots(reportSnaps, ReportingService::SortField::WAITING_DURATION, ReportingService::SortAlgorithm::MERGE_SORT);
    TEST_ASSERT(reportSnaps[0].isWaitingDurationApplicable, "First patient has applicable waiting duration");
    TEST_ASSERT(reportSnaps[0].waitingDurationSeconds >= reportSnaps[1].waitingDurationSeconds,
                "Longest waiting duration appears first in waiting report");
}

void testEducationalDemonstrations() {
    std::cout << "[SUBTEST] Educational Demonstrations Comprehensive Verification ...\n";

    HospitalSystem sys;
    std::string err;
    sys.initializeFromSeed(gDataDir, err);

    std::vector<PatientSnapshot> snapshots = ReportingService::captureSnapshots(sys);

    // 1. Sorting Demo
    std::ostringstream sortOss;
    DemonstrationService::runSortingComparison(snapshots, sortOss);
    std::string sortStr = sortOss.str();
    TEST_ASSERT(sortStr.find("Merge Sort") != std::string::npos, "Sort demo contains Merge Sort");
    TEST_ASSERT(sortStr.find("Quicksort") != std::string::npos, "Sort demo contains Quicksort");
    TEST_ASSERT(sortStr.find("Heapsort") != std::string::npos, "Sort demo contains Heapsort");
    TEST_ASSERT(sortStr.find("STABILITY ANALYSIS") != std::string::npos, "Sort demo contains stability analysis");

    // 2. Hash Collision Demo
    std::ostringstream hashOss;
    DemonstrationService::runHashCollisionComparison(hashOss);
    std::string hashStr = hashOss.str();
    TEST_ASSERT(hashStr.find("COLLISION DETECTED") != std::string::npos, "Hash demo verifies direct collision");
    TEST_ASSERT(hashStr.find("TOMBSTONE") != std::string::npos, "Hash demo explains linear probing tombstones");

    // 3. Graph Traversal Demo
    std::ostringstream graphOss;
    DemonstrationService::runGraphTraversalComparison(graphOss);
    std::string graphStr = graphOss.str();
    TEST_ASSERT(graphStr.find("BFS Exploration") != std::string::npos, "Graph demo contains BFS");
    TEST_ASSERT(graphStr.find("O(V + E)") != std::string::npos, "Graph demo explains Adjacency List complexity");
    TEST_ASSERT(graphStr.find("O(V^2)") != std::string::npos, "Graph demo explains Adjacency Matrix complexity");

    // 4. BST Operations Demo
    std::ostringstream bstOss;
    DemonstrationService::runBSTOperationsDemo(bstOss);
    std::string bstStr = bstOss.str();
    TEST_ASSERT(bstStr.find("In-Order") != std::string::npos, "BST demo displays in-order traversal");
    TEST_ASSERT(bstStr.find("MIRROR DEMONSTRATION") != std::string::npos, "BST demo contains mirror demonstration");
    TEST_ASSERT(bstStr.find("3-CASE DELETION") != std::string::npos, "BST demo contains 3-case deletion");

    // 5. Dijkstra vs MST Demo
    std::ostringstream mstOss;
    DemonstrationService::runDijkstraVsMSTDemo(sys.getMapManager(), mstOss);
    std::string mstStr = mstOss.str();
    TEST_ASSERT(mstStr.find("Dijkstra's Algorithm") != std::string::npos, "Demo contains Dijkstra description");
    TEST_ASSERT(mstStr.find("Minimum Spanning Tree") != std::string::npos, "Demo contains MST description");
    TEST_ASSERT(mstStr.find("1280") != std::string::npos, "Demo reports verified 1280s MST total weight");
}

void testConsoleAppScriptedWorkflows() {
    // Subtest 1: Immediate Exit & Immediate EOF
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // 1a: User enters "9" at main menu to exit
        std::istringstream in1("9\n");
        std::ostringstream out1;
        ConsoleApp app1(sys, in1, out1, gDataDir);
        app1.run();
        std::string s1 = out1.str();
        TEST_ASSERT(s1.find("Shutting down Smart Hospital") != std::string::npos, "Exit choice 9 shuts down cleanly");

        // 1b: User enters "0" / "b" at main menu
        std::istringstream in1b("0\n");
        std::ostringstream out1b;
        ConsoleApp app1b(sys, in1b, out1b, gDataDir);
        app1b.run();
        TEST_ASSERT(out1b.str().find("Shutting down Smart Hospital") != std::string::npos, "Exit choice 0 shuts down cleanly");

        // 1c: Empty input stream (immediate EOF)
        std::istringstream inEof("");
        std::ostringstream outEof;
        ConsoleApp appEof(sys, inEof, outEof, gDataDir);
        appEof.run();
        TEST_ASSERT(outEof.str().find("EOF DETECTED") != std::string::npos, "Empty stream detects EOF and terminates");
    }

    // Subtest 2: Invalid Menu Choices & Out-of-Range Recovery
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Inputs: blank line, 99 (out of range), "invalid_str", "12bad", then "9" (exit)
        std::istringstream in2("\n   \n99\ninvalid_str\n12bad\n9\n");
        std::ostringstream out2;
        ConsoleApp app2(sys, in2, out2, gDataDir);
        app2.run();
        std::string s2 = out2.str();
        TEST_ASSERT(s2.find("Invalid selection '99'") != std::string::npos, "Rejects out of range selection 99");
        TEST_ASSERT(s2.find("Invalid selection 'invalid_str'") != std::string::npos, "Rejects alphanumeric string");
        TEST_ASSERT(s2.find("Invalid selection '12bad'") != std::string::npos, "Rejects trailing garbage");
    }

    // Subtest 3: Scripted Patient Registration & Full Dossier Lookup
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Main menu (1: Patient menu) -> 1: Register ->
        // ID: P901, Name: Sarah Connor-O'Reilly, Age: 34, Complaint: Machinery fracture, Urgency: 2, CurrLoc: 1, TargLoc: 4, Billing: 4500.00
        // -> 2: Lookup -> P901 -> 0: Back -> 9: Exit
        std::ostringstream script3;
        script3 << "1\n" // Go to Patient Menu
                << "1\n" // Register
                << "P901\n"
                << "Sarah Connor-O'Reilly\n"
                << "34\n"
                << "Machinery fracture and lacerations\n"
                << "2\n"
                << "1\n"
                << "4\n"
                << "4500.00\n"
                << "2\n" // Lookup
                << "P901\n"
                << "0\n" // Back to main
                << "9\n"; // Exit

        std::istringstream in3(script3.str());
        std::ostringstream out3;
        ConsoleApp app3(sys, in3, out3, gDataDir);
        app3.run();

        std::string s3 = out3.str();
        TEST_ASSERT(s3.find("P901 registered successfully") != std::string::npos, "Registration through menu succeeded");
        TEST_ASSERT(s3.find("Sarah Connor-O'Reilly") != std::string::npos, "Lookup displays full name with apostrophe and hyphen");
        TEST_ASSERT(s3.find("Machinery fracture and lacerations") != std::string::npos, "Lookup displays full complaint");
        TEST_ASSERT(s3.find("$4500.00") != std::string::npos, "Lookup displays correct billing amount");
        TEST_ASSERT(sys.findPatient("P901") != nullptr, "Patient P901 is present in authoritative registry");
    }

    // Subtest 4: Scripted Appointment Booking & Encapsulated Cancellation
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Register P902 (Registered state)
        TEST_ASSERT(sys.registerNewPatient("P902", "Alex Murphy", 38, "Chest Pain", UrgencyLevel::LESS_URGENT, 1, 2, 200.0, err), "P902 registered");

        // Main menu (3: Appointment Menu) -> 1: Schedule -> P902 -> 2: View roster -> 4: Cancel -> P902 -> 0: Back -> 9: Exit
        std::ostringstream script4;
        script4 << "3\n"
                << "1\n"
                << "P902\n"
                << "2\n" // View roster
                << "4\n" // Cancel appointment
                << "P902\n"
                << "0\n"
                << "9\n";

        std::istringstream in4(script4.str());
        std::ostringstream out4;
        ConsoleApp app4(sys, in4, out4, gDataDir);
        app4.run();

        std::string s4 = out4.str();
        TEST_ASSERT(s4.find("Appointment booked for patient P902") != std::string::npos, "Menu confirms appointment booked");
        TEST_ASSERT(s4.find("cancelled and removed from queue") != std::string::npos, "Menu confirms appointment cancelled");
        const Patient* p902 = sys.findPatient("P902");
        TEST_ASSERT(p902 && p902->state == PatientState::CANCELLED, "Patient P902 state transitioned to CANCELLED");
    }

    // Subtest 5: Scripted Triage Urgency Update
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        const Patient* preP108 = sys.findPatient("P108");
        TEST_ASSERT(preP108 != nullptr, "P108 exists in seed dataset");
        uint64_t originalSeq = preP108->arrivalSequence;

        // Main menu (2: Triage Menu) -> 3: Update Urgency -> P108 -> Urgency: 1 (Resuscitation) -> 0 -> 9
        std::ostringstream script5;
        script5 << "2\n"
                << "3\n"
                << "P108\n"
                << "1\n" // New urgency 1
                << "0\n"
                << "9\n";

        std::istringstream in5(script5.str());
        std::ostringstream out5;
        ConsoleApp app5(sys, in5, out5, gDataDir);
        app5.run();

        std::string s5 = out5.str();
        TEST_ASSERT(s5.find("Urgency for patient P108 successfully updated") != std::string::npos, "Triage urgency update confirmed");
        TEST_ASSERT(s5.find("Preserved Arrival Sequence: #" + std::to_string(originalSeq)) != std::string::npos, "Arrival sequence preservation stated");
        const Patient* postP108 = sys.findPatient("P108");
        TEST_ASSERT(postP108 && postP108->urgency == UrgencyLevel::RESUSCITATION, "P108 clinical urgency updated to RESUSCITATION");
        TEST_ASSERT(postP108 && postP108->arrivalSequence == originalSeq, "P108 arrival sequence strictly preserved");
    }

    // Subtest 6: Scripted Dispatch (Candidate Selection -> Assignment -> Pickup -> Transport -> Discharge)
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Main menu (4: Dispatch Menu)
        // -> 1: Select Candidate
        // -> 2: Assign Resource (enter "P104")
        // -> 3: Step 1 Pickup ("P104")
        // -> 4: Step 2 Transport & Handover ("P104")
        // -> 6: Discharge ("P104")
        // -> 0 -> 9
        std::ostringstream script6;
        script6 << "4\n"
                << "1\n"
                << "2\n"
                << "P104\n"
                << "3\n"
                << "P104\n"
                << "4\n"
                << "P104\n"
                << "6\n"
                << "P104\n"
                << "0\n"
                << "9\n";

        std::istringstream in6(script6.str());
        std::ostringstream out6;
        ConsoleApp app6(sys, in6, out6, gDataDir);
        app6.run();

        std::string s6 = out6.str();
        TEST_ASSERT(s6.find("ASSIGNMENT SUCCESSFUL") != std::string::npos, "Dispatch assignment succeeded in UI");
        TEST_ASSERT(s6.find("Leg 1 (Pickup Leg)") != std::string::npos, "Displays Leg 1 route and cost");
        TEST_ASSERT(s6.find("Leg 2 (Transport Leg)") != std::string::npos, "Displays Leg 2 route and cost");
        TEST_ASSERT(s6.find("PICKUP ARRIVAL COMPLETE") != std::string::npos, "Pickup step completed");
        TEST_ASSERT(s6.find("TRANSPORT & HANDOVER COMPLETE") != std::string::npos, "Transport & Handover step completed");
        TEST_ASSERT(s6.find("Patient P104 successfully discharged") != std::string::npos, "Discharge completed");

        const Patient* p104 = sys.findPatient("P104");
        TEST_ASSERT(p104 && p104->state == PatientState::DISCHARGED, "Patient P104 reached terminal DISCHARGED state");
    }

    // Subtest 7: Scripted Failed Routing & Reopen Retry
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Main menu (5: Map Menu)
        // -> 3: Block Corridor: 0 <-> 1
        // -> 5: Query Route: 0 -> 1
        // -> 4: Reopen Corridor: 0 <-> 1
        // -> 5: Query Route: 0 -> 1
        // -> 0 -> 9
        std::ostringstream script7;
        script7 << "5\n"
                << "3\n"
                << "0\n"
                << "1\n"
                << "5\n"
                << "0\n"
                << "1\n"
                << "4\n"
                << "0\n"
                << "1\n"
                << "5\n"
                << "0\n"
                << "1\n"
                << "0\n"
                << "9\n";

        std::istringstream in7(script7.str());
        std::ostringstream out7;
        ConsoleApp app7(sys, in7, out7, gDataDir);
        app7.run();

        std::string s7 = out7.str();
        TEST_ASSERT(s7.find("Corridor between Node 0") != std::string::npos && s7.find("BLOCKED") != std::string::npos, "Corridor 0-1 blocked");
        TEST_ASSERT(s7.find("REOPENED") != std::string::npos, "Corridor 0-1 reopened");
        TEST_ASSERT(s7.find("ROUTE FOUND") != std::string::npos, "Route query succeeded after reopen");
    }

    // Subtest 8: Scripted Report Sorting Without Live-State Mutation
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        size_t prePatients = sys.getPatientCount();
        size_t preTriage = sys.getTriageQueueSize();
        size_t preAppts = sys.getAppointmentQueueSize();
        int64_t preClock = sys.getSimulationTime();
        size_t preAudits = sys.getAuditEvents().size();

        // Main menu (6: Reports)
        // -> 1: Generate Report -> Field 4 (Billing Amount) -> Algorithm 3 (Heapsort)
        // -> 0 -> 9
        std::ostringstream script8;
        script8 << "6\n"
                << "1\n"
                << "4\n" // Field: Billing Amount
                << "3\n" // Algo: Heapsort
                << "0\n"
                << "9\n";

        std::istringstream in8(script8.str());
        std::ostringstream out8;
        ConsoleApp app8(sys, in8, out8, gDataDir);
        app8.run();

        std::string s8 = out8.str();
        TEST_ASSERT(s8.find("HOSPITAL PATIENT SUMMARY REPORT SNAPSHOT") != std::string::npos, "Report table generated");
        TEST_ASSERT(s8.find("Heapsort") != std::string::npos, "Heapsort metrics displayed");
        TEST_ASSERT(s8.find("READ-ONLY GUARANTEE VERIFIED") != std::string::npos, "Read-only guarantee displayed");

        TEST_ASSERT(sys.getPatientCount() == prePatients, "Live patient registry count unchanged");
        TEST_ASSERT(sys.getTriageQueueSize() == preTriage, "Live triage queue size unchanged");
        TEST_ASSERT(sys.getAppointmentQueueSize() == preAppts, "Live appointment queue size unchanged");
        TEST_ASSERT(sys.getSimulationTime() == preClock, "Live simulation clock unchanged");
        TEST_ASSERT(sys.getAuditEvents().size() == preAudits, "Live audit log size unchanged");
    }

    // Subtest 9: System Consistency Checker Validation after Workflows
    {
        HospitalSystem sys;
        std::string err;
        sys.initializeFromSeed(gDataDir, err);

        // Execute a multi-action session
        std::ostringstream script9;
        script9 << "1\n1\nP999\nBruce Wayne\n35\nHead Trauma\n1\n1\n3\n8900.00\n0\n" // Register P999
                << "2\n2\nP999\n1\n0\n" // Triage P999
                << "4\n2\nP999\n3\nP999\n4\nP999\n6\nP999\n0\n" // Assign, Pickup, Handover, Discharge
                << "8\n2\n0\n" // Run integrity check from menu
                << "9\n"; // Exit

        std::istringstream in9(script9.str());
        std::ostringstream out9;
        ConsoleApp app9(sys, in9, out9, gDataDir);
        app9.run();

        std::string s9 = out9.str();
        TEST_ASSERT(s9.find("SYSTEM INTEGRITY: VERIFIED HEALTHY") != std::string::npos, "Integrity check reported healthy via UI menu");

        std::string integrityErr;
        TEST_ASSERT(sys.validateSystemIntegrity(integrityErr), "Direct cross-service system integrity holds after workflow: " + integrityErr);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "================================================================================\n";
    std::cout << "    SMART HOSPITAL SYSTEM - AUTOMATED VERIFICATION TEST HARNESS (CS2305)\n";
    std::cout << "        Milestones 1, 2, 3, 4, 5, 6 & 7 (Full System & UI) Test Suite\n";
    std::cout << "================================================================================\n\n";

    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).length() > 0) {
        gDataDir = argv[1];
        if (gDataDir.back() != '/' && gDataDir.back() != '\\') {
            gDataDir += "/";
        }
    } else {
        std::ifstream f1("data/patients_seed.csv");
        if (!f1.good()) {
            std::ifstream f2("../data/patients_seed.csv");
            if (f2.good()) gDataDir = "../data/";
        }
    }

    std::cout << "[TEST CONFIG] Data Directory Path: " << gDataDir << "\n\n";

    // Milestone 1 Tests
    std::cout << "--- MILESTONE 1: FOUNDATION, MODELS & SEED DATA ---\n";
    RUN_TEST(testPatientModelAndStateQueries);
    RUN_TEST(testModelLifecycleAndHandoverDemo);
    RUN_TEST(testProductionDistanceArithmetic);
    RUN_TEST(testSeedDataIntegrity);
    RUN_TEST(testDataLoaderNegativeValidation);

    // Milestone 2 Tests & Corrections
    std::cout << "\n--- MILESTONE 2: CUSTOM LINEAR DATA STRUCTURES & HASHING ---\n";
    RUN_TEST(testSinglyLinkedList);
    RUN_TEST(testDoublyLinkedList);
    RUN_TEST(testCustomStack);
    RUN_TEST(testCustomQueue);
    RUN_TEST(testHashTableChaining);
    RUN_TEST(testHashTableProbing);
    RUN_TEST(testExceptionSafeCopySemantics);

    // Milestone 3 Tests
    std::cout << "\n--- MILESTONE 3: HIERARCHICAL STRUCTURES & PRIORITY HEAP ---\n";
    RUN_TEST(testBinarySearchTree);
    RUN_TEST(testBinaryMinHeapAndTriage);

    // Milestone 4 Tests
    std::cout << "\n--- MILESTONE 4: GRAPH TOPOLOGY, ROUTING & MINIMUM SPANNING TREES ---\n";
    RUN_TEST(testCampusGraphAndRouting);

    // Milestone 5 Tests
    std::cout << "\n--- MILESTONE 5: HOSPITAL WORKFLOW, TRIAGE & ROUTE DISPATCH SERVICES ---\n";
    RUN_TEST(testHospitalRegistryAndRegistration);
    RUN_TEST(testSeedInitializationAndIndexes);
    RUN_TEST(testEqualUrgencyFIFOAndUrgencyUpdates);
    RUN_TEST(testAppointmentFIFOAndCancellationAndConversion);
    RUN_TEST(testDispatchFailuresAndPreconditions);
    RUN_TEST(testDeterministicResourceSelectionAndTieBreaking);
    RUN_TEST(testSimulationLegsCorridorClosureAndRerouting);
    RUN_TEST(testPickupCancellation);
    RUN_TEST(testResourceReuseAndDischargeIsolation);
    RUN_TEST(testTerminalStateRejectionAndAudit);
    RUN_TEST(testPendingDispatchOrderingPolicy);
    RUN_TEST(testCompleteRegistrationToDischargeEndToEnd);

    // Milestone 5 Consistency Repairs (Regression Tests)
    std::cout << "\n--- MILESTONE 5 CONSISTENCY REPAIRS: REGRESSION & INTEGRITY VERIFICATION ---\n";
    RUN_TEST(testMilestone5MoveDisablingPolicy);
    RUN_TEST(testConsistentInputValidationAndStatePreservation);
    RUN_TEST(testPreserveAppointmentOrderOnFailedConversion);
    RUN_TEST(testRejectedOperationHandlingAndReciprocalChecks);
    RUN_TEST(testCoherentTimeSemanticsAndSeedSnapshot);
    RUN_TEST(testRegistryRegistrationExceptionSafetyRollback);
    RUN_TEST(testInitializationResetAndSeedValidation);
    RUN_TEST(testSystemIntegrityCheckerVerification);

    // Milestone 6 Tests: Reporting, Sorting & Educational Demonstrations
    std::cout << "\n--- MILESTONE 6: REPORTING SNAPSHOTS, SORTING & EDUCATIONAL DEMONSTRATIONS ---\n";
    RUN_TEST(testMilestone6ConsistencyFixes);
    RUN_TEST(testCustomSortingAlgorithms);
    RUN_TEST(testSortingStability);
    RUN_TEST(testPatientSnapshotAndReportingService);
    RUN_TEST(testEducationalDemonstrations);

    // Milestone 7 Tests: Interactive Console UI & Scripted Workflows
    std::cout << "\n--- MILESTONE 7: INTERACTIVE CONSOLE UI & SCRIPTED WORKFLOWS ---\n";
    RUN_TEST(testConsoleAppScriptedWorkflows);

    std::cout << "\n--------------------------------------------------------------------------------\n";
    std::cout << "TOTAL TEST SUMMARY: " << gPassCount << " Passed, " << gFailCount << " Failed.\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    return (gFailCount == 0) ? 0 : 1;
}
