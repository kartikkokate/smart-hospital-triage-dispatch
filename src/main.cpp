#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <string>

#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "models/CampusLocation.hpp"
#include "services/DataLoader.hpp"

// Custom Data Structures
#include "ds/SinglyLinkedList.hpp"
#include "ds/DoublyLinkedList.hpp"
#include "ds/CustomStack.hpp"
#include "ds/CustomQueue.hpp"
#include "ds/HashTableChaining.hpp"
#include "ds/HashTableProbing.hpp"
#include "ds/BinarySearchTree.hpp"
#include "ds/BinaryMinHeap.hpp"
#include "algorithms/CustomSorting.hpp"
#include "ds/DisjointSetUnion.hpp"
#include "ds/CampusGraph.hpp"

// Hospital Workflow & Dispatch Services
#include "services/HospitalRegistry.hpp"
#include "services/CampusMapManager.hpp"
#include "services/TriageManager.hpp"
#include "services/AppointmentManager.hpp"
#include "services/DispatchManager.hpp"
#include "services/HospitalSystem.hpp"

// Reporting, Sorting & Educational Demonstrations
#include "models/PatientSnapshot.hpp"
#include "services/ReportingService.hpp"
#include "services/DemonstrationService.hpp"

// Milestone 7 Interactive Console Application
#include "ui/ConsoleApp.hpp"

namespace {
    std::string resolveDataDir() {
        std::ifstream test1("data/patients_seed.csv");
        if (test1.good()) return "data/";
        std::ifstream test2("../data/patients_seed.csv");
        if (test2.good()) return "../data/";
        std::ifstream test3("../../data/patients_seed.csv");
        if (test3.good()) return "../../data/";
        return "data/";
    }

    /**
     * @brief Deterministic, non-interactive demonstration routine for automated verification.
     *        Executes Milestones 1 to 6 features end-to-end, validates every operation result,
     *        verifies expected states and system integrity at each step, and terminates with code 0 on success.
     */
    int runNonInteractiveDemo(const std::string& dataDir) {
        std::cout << "================================================================================\n";
        std::cout << "    SMART HOSPITAL EMERGENCY TRIAGE & ROUTE DISPATCH SYSTEM (CS2305)\n";
        std::cout << "       Academic Simulation - AY 2026-27 | Data Structures-I Project\n";
        std::cout << "        [--demo MODE: Deterministic Noninteractive Verification Run]\n";
        std::cout << "================================================================================\n\n";

        std::cout << "[SYSTEM INIT] Resolved synthetic seed datasets directory: " << dataDir << "\n";

        std::vector<Patient> patients;
        std::vector<DispatchResource> resources;
        std::vector<CampusLocation> locations;
        std::vector<CorridorEdge> edges;
        std::string error;

        // 1. Load patients
        if (!DataLoader::loadPatientsCSV(dataDir + "patients_seed.csv", patients, error)) {
            std::cerr << "[ERROR] Patient loading failed: " << error << "\n";
            return 1;
        }
        if (patients.size() != 12) {
            std::cerr << "[ERROR] Expected 12 seed patients, loaded " << patients.size() << "\n";
            return 1;
        }
        std::cout << "  -> Loaded " << patients.size() << " synthetic patient records successfully.\n";

        // 2. Load resources
        if (!DataLoader::loadResourcesCSV(dataDir + "resources_seed.csv", resources, error)) {
            std::cerr << "[ERROR] Resource loading failed: " << error << "\n";
            return 1;
        }
        if (resources.size() != 3) {
            std::cerr << "[ERROR] Expected 3 seed resources, loaded " << resources.size() << "\n";
            return 1;
        }
        std::cout << "  -> Loaded " << resources.size() << " dispatch resources successfully.\n";

        // 3. Load campus map
        if (!DataLoader::loadCampusMapTXT(dataDir + "campus_map_seed.txt", locations, edges, error)) {
            std::cerr << "[ERROR] Campus map loading failed: " << error << "\n";
            return 1;
        }
        if (locations.size() != 12 || edges.size() != 15) {
            std::cerr << "[ERROR] Expected 12 locations and 15 corridors, loaded " 
                      << locations.size() << " locations and " << edges.size() << " corridors.\n";
            return 1;
        }
        std::cout << "  -> Loaded " << locations.size() << " campus locations and " 
                  << edges.size() << " corridors successfully.\n";

        // 4. Validate campus references against actual loaded nodes
        if (!DataLoader::validateCampusReferences(patients, resources, locations, error)) {
            std::cerr << "[ERROR] Campus reference validation failed: " << error << "\n";
            return 1;
        }
        std::cout << "  -> Cross-validated all patient and resource location references against campus topology.\n";

        auto getLocationName = [&](int nodeId) -> std::string {
            for (const auto& loc : locations) {
                if (loc.nodeId == nodeId) return loc.name;
            }
            return "Node " + std::to_string(nodeId);
        };

        auto getPatientLabel = [&](const std::string& id) -> std::string {
            for (const auto& p : patients) {
                if (p.patientId == id) return p.patientId + " (" + p.fullName + ")";
            }
            return id;
        };

        // Milestone 2 & 3 Demonstrations: Linear & Hierarchical Structures
        std::cout << "\n================================================================================\n";
        std::cout << "    DEMONSTRATION: CUSTOM LINEAR & HIERARCHICAL STRUCTURES (CO2, CO3, CO4, CO5)\n";
        std::cout << "================================================================================\n";

        // Demo 1: CustomStack (CO3)
        std::cout << "\n[1] CustomStack<int> (LIFO Route Backtracking Demo - CO3):\n";
        CustomStack<int> routeStack;
        routeStack.push(0);
        routeStack.push(1);
        routeStack.push(5);
        if (routeStack.size() != 3 || *routeStack.top() != 5) {
            std::cerr << "[ERROR] CustomStack push/top validation failed.\n";
            return 1;
        }
        std::cout << "  Stack size: " << routeStack.size() << ", Current Top: " << *routeStack.top() 
                  << " (" << getLocationName(*routeStack.top()) << ")\n";
        std::cout << "  Popping stack (LIFO order): ";
        int expectedPopped[] = {5, 1, 0};
        int popIdx = 0;
        while (!routeStack.isEmpty()) {
            int node = 0;
            if (!routeStack.pop(node) || node != expectedPopped[popIdx++]) {
                std::cerr << "\n[ERROR] CustomStack LIFO order mismatch.\n";
                return 1;
            }
            std::cout << node << " (" << getLocationName(node) << ") ";
        }
        if (!routeStack.isEmpty() || routeStack.pop()) {
            std::cerr << "\n[ERROR] CustomStack underflow protection failed.\n";
            return 1;
        }
        std::cout << "\n  Safe underflow check on empty stack: false (Safe underflow verified)\n";

        // Demo 2: CustomQueue (CO3)
        std::cout << "\n[2] CustomQueue<std::string> (FIFO Outpatient Roster Demo - CO3):\n";
        CustomQueue<std::string> appointmentQueue;
        appointmentQueue.enqueue(getPatientLabel("P102"));
        appointmentQueue.enqueue(getPatientLabel("P106"));
        appointmentQueue.enqueue(getPatientLabel("P111"));
        if (appointmentQueue.size() != 3) {
            std::cerr << "[ERROR] CustomQueue enqueue count mismatch.\n";
            return 1;
        }
        std::cout << "  Queue size: " << appointmentQueue.size() << ", Current Front: " << *appointmentQueue.front() << "\n";
        std::cout << "  Dequeuing consultations in FIFO order:\n";
        std::string expectedAppts[] = {getPatientLabel("P102"), getPatientLabel("P106"), getPatientLabel("P111")};
        int apptIdx = 0;
        while (!appointmentQueue.isEmpty()) {
            std::string label;
            if (!appointmentQueue.dequeue(label) || label != expectedAppts[apptIdx++]) {
                std::cerr << "[ERROR] CustomQueue FIFO ordering mismatch.\n";
                return 1;
            }
            std::cout << "    - Called: " << label << "\n";
        }
        if (!appointmentQueue.isEmpty() || appointmentQueue.dequeue()) {
            std::cerr << "[ERROR] CustomQueue safe underflow failed.\n";
            return 1;
        }
        std::cout << "  Safe dequeue on empty queue: false (Safe underflow verified)\n";

        // Demo 3: DoublyLinkedList (CO2)
        std::cout << "\n[3] DoublyLinkedList<std::string> (Audit Trail / Dispatch Log Demo - CO2):\n";
        DoublyLinkedList<std::string> auditLog;
        auditLog.push_back("[t=10s] Patient P101 registered at ER Triage");
        auditLog.push_back("[t=45s] Resource GURNEY-01 dispatched to North Gate");
        auditLog.push_back("[t=120s] Patient P101 handover complete at Trauma ICU");
        if (auditLog.size() != 3 || !auditLog.validateIntegrity()) {
            std::cerr << "[ERROR] DoublyLinkedList integrity validation failed.\n";
            return 1;
        }
        std::cout << "  Forward Traversal (Chronological Log):\n";
        for (const auto& entry : auditLog.to_vector_forward()) {
            std::cout << "    " << entry << "\n";
        }
        std::cout << "  Reverse Traversal (Most Recent First):\n";
        for (const auto& entry : auditLog.to_vector_backward()) {
            std::cout << "    " << entry << "\n";
        }

        // Demo 4: SinglyLinkedList (CO2)
        std::cout << "\n[4] SinglyLinkedList<int> (Dynamic Free List & Route Segments Demo - CO2):\n";
        SinglyLinkedList<int> segmentList;
        segmentList.push_front(5);
        segmentList.push_front(1);
        segmentList.push_front(0);
        if (segmentList.size() != 3) {
            std::cerr << "[ERROR] SinglyLinkedList size mismatch.\n";
            return 1;
        }
        std::cout << "  Path segments (push_front): ";
        for (int node : segmentList) {
            std::cout << node << " (" << getLocationName(node) << ") -> ";
        }
        std::cout << "END\n";

        // Demo 5: HashTableChaining (CO2)
        std::cout << "\n[5] HashTableChaining<std::string, Patient> (Primary Expected O(1) Patient Registry - CO2):\n";
        HashTableChaining<std::string, Patient> registryTable(17, 0.75);
        for (const auto& p : patients) {
            registryTable.insert(p.patientId, p);
        }
        std::cout << "  Inserted " << registryTable.size() << " patients. Current load factor: " 
                  << std::fixed << std::setprecision(2) << registryTable.loadFactor() << "\n";
        std::string testId = "P104";
        Patient* found = registryTable.search(testId);
        if (!found || found->fullName != "Marcus Brody" || urgencyToInt(found->urgency) != 1) {
            std::cerr << "[ERROR] HashTableChaining lookup failed for " << testId << "\n";
            return 1;
        }
        std::cout << "  Lookup " << testId << ": SUCCESS -> Name: " << found->fullName 
                  << ", Urgency: " << urgencyToString(found->urgency) 
                  << " (" << urgencyToInt(found->urgency) << ")"
                  << ", Location: " << found->currentLocationNode 
                  << " (" << getLocationName(found->currentLocationNode) << ")\n";

        // Demo 6: HashTableProbing (CO2)
        std::cout << "\n[6] HashTableProbing<std::string, DispatchResource> (Transfer Fleet Index - CO2):\n";
        HashTableProbing<std::string, DispatchResource> fleetTable(17, 0.50);
        for (const auto& r : resources) {
            fleetTable.insert(r.resourceId, r);
        }
        std::cout << "  Inserted " << fleetTable.size() << " resources. Current load factor: " 
                  << std::fixed << std::setprecision(2) << fleetTable.loadFactor() << "\n";
        std::string testRes = "GURNEY-02";
        DispatchResource* resFound = fleetTable.search(testRes);
        if (!resFound || resFound->name != "Critical Care Mobile Unit" || resFound->state != ResourceState::AVAILABLE) {
            std::cerr << "[ERROR] HashTableProbing lookup failed for valid fixture " << testRes << "\n";
            return 1;
        }
        std::cout << "  Lookup " << testRes << ": SUCCESS -> Name: " << resFound->name 
                  << ", State: " << resourceStateToString(resFound->state) 
                  << ", Location: " << resFound->currentNodeId 
                  << " (" << getLocationName(resFound->currentNodeId) << ")\n";

        // Demo 7: BinarySearchTree (CO4)
        std::cout << "\n[7] BinarySearchTree<std::string, int64_t> (Patient Arrival Index - CO4):\n";
        BinarySearchTree<std::string, int64_t> arrivalBst;
        for (const auto& p : patients) {
            arrivalBst.insert(p.patientId, p.simulatedArrivalTime);
        }
        if (arrivalBst.size() != 12 || arrivalBst.search("P101") == nullptr) {
            std::cerr << "[ERROR] BinarySearchTree validation failed.\n";
            return 1;
        }
        std::cout << "  BST Node Count: " << arrivalBst.size() << ", Tree Height: " << arrivalBst.height() << "\n";

        // Demo 8: BinaryMinHeap (CO5)
        std::cout << "\n[8] BinaryMinHeap<TriageRecord, ...> (Emergency Clinical Triage Min-Heap - CO5):\n";
        BinaryMinHeap<TriageRecord, TriageRecordComparator> triageHeap;
        for (const auto& p : patients) {
            if (p.state == PatientState::TRIAGED_WAITING) {
                TriageRecord rec;
                rec.internalHandle = p.internalHandle;
                rec.urgency = p.urgency;
                rec.arrivalSequence = p.arrivalSequence;
                triageHeap.insert(rec);
            }
        }
        if (triageHeap.size() != 9) {
            std::cerr << "[ERROR] Triage min-heap size mismatch. Expected 9, got " << triageHeap.size() << "\n";
            return 1;
        }
        std::cout << "  Triage Min-Heap entries: " << triageHeap.size() << "\n";
        const TriageRecord* root = triageHeap.peek();
        if (!root || urgencyToInt(root->urgency) != 1) {
            std::cerr << "[ERROR] Triage min-heap root priority validation failed.\n";
            return 1;
        }
        std::cout << "  Root Patient: Handle #" << root->internalHandle
                  << " | Urgency: " << urgencyToString(root->urgency) 
                  << " (" << urgencyToInt(root->urgency) << ") | ArrSeq: #" << root->arrivalSequence << "\n";

        // Demo 9 & 10: Dijkstra & MST (CO6)
        std::cout << "\n================================================================================\n";
        std::cout << "    DEMONSTRATION: GRAPH TOPOLOGY, ROUTING & SPANNING TREES (CO6)\n";
        std::cout << "================================================================================\n";

        CampusGraph graph(locations.size());
        for (const auto& e : edges) {
            graph.addCorridor(e.u, e.v, e.transitTimeSeconds);
        }

        std::cout << "\n[9] Dijkstra's Algorithm (Point-to-Point Dispatch Route Optimization - CO6):\n";
        int demoSource = 0;
        int demoDest = 3;
        DijkstraResult route = graph.findShortestRoute(demoSource, demoDest);
        if (!route.isReachable || route.totalCost != 270 || route.path.size() != 4) {
            std::cerr << "[ERROR] Dijkstra route optimization failed for 0 -> 3. Expected cost 270s.\n";
            return 1;
        }
        std::cout << "  Optimal Emergency Transit Route (" << demoSource << " [" << getLocationName(demoSource) << "] -> "
                  << demoDest << " [" << getLocationName(demoDest) << "]):\n";
        std::cout << "    Total Travel Duration: " << route.totalCost << " seconds\n";
        std::cout << "    Turn-by-turn Navigation Path: ";
        for (size_t i = 0; i < route.path.size(); ++i) {
            std::cout << route.path[i] << " (" << getLocationName(route.path[i]) << ")"
                      << (i + 1 < route.path.size() ? " -> " : "");
        }
        std::cout << "\n";

        std::cout << "\n[10] Minimum Spanning Trees (Campus Evacuation & Telemetry Backbone - CO6):\n";
        MSTResult kruskalMST = graph.computeKruskalMST();
        MSTResult primMST = graph.computePrimMST();
        if (kruskalMST.totalTransitWeight != 1280 || primMST.totalTransitWeight != 1280 ||
            !kruskalMST.isConnectedSpanningTree || !primMST.isConnectedSpanningTree ||
            kruskalMST.edges.size() != 11 || primMST.edges.size() != 11) {
            std::cerr << "[ERROR] MST validation failed. Expected 1280s total weight across 11 edges.\n";
            return 1;
        }
        std::cout << "  Kruskal's Algorithm Total Spanning Cost: " << kruskalMST.totalTransitWeight << "s (" 
                  << kruskalMST.edges.size() << " corridors, " 
                  << (kruskalMST.isConnectedSpanningTree ? "Fully Connected MST" : "Spanning Forest") << ")\n";
        std::cout << "  Prim's Algorithm Total Spanning Cost:    " << primMST.totalTransitWeight << "s (" 
                  << primMST.edges.size() << " corridors, " 
                  << (primMST.isConnectedSpanningTree ? "Fully Connected MST" : "Spanning Forest") << ")\n";

        // Milestone 5 Integrated Workflow Demo
        std::cout << "\n================================================================================\n";
        std::cout << "  MILESTONE 5: INTEGRATED HOSPITAL OPERATIONS & DISPATCH WORKFLOW DEMO\n";
        std::cout << "================================================================================\n";

        HospitalSystem hospital;
        std::string sysErr;
        if (!hospital.initializeFromSeed(dataDir, sysErr)) {
            std::cerr << "  [ERROR] Failed to initialize HospitalSystem from seed: " << sysErr << "\n";
            return 1;
        }

        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Post-seed system integrity validation failed: " << sysErr << "\n";
            return 1;
        }
        if (hospital.getRegistry().getPatientCount() != 12 ||
            hospital.getTriageManager().size() != 9 ||
            hospital.getAppointmentManager().size() != 3 ||
            hospital.getDispatchManager().getAllResources().size() != 3) {
            std::cerr << "  [ERROR] HospitalSystem post-seed counts mismatch.\n";
            return 1;
        }
        std::cout << "  [1] Seed Initialization & System Consistency Verified:\n";
        std::cout << "      Authoritative Patients in Registry: " << hospital.getRegistry().getPatientCount() << "\n";
        std::cout << "      Active Emergency Patients in Triage: " << hospital.getTriageManager().size() << "\n";
        std::cout << "      Scheduled Outpatient Appointments:  " << hospital.getAppointmentManager().size() << "\n";
        std::cout << "      Active Transfer Resources:          " << hospital.getDispatchManager().getAllResources().size() << "\n";

        // 2. Patient Registration
        std::cout << "\n  [2] Patient Registration & Duplicate ID Rejection:\n";
        if (!hospital.registerNewPatient("P501", "Jonathan Wick", 42, "Lacerations & Shock",
                                        UrgencyLevel::RESUSCITATION, 1, 2, 2800.0, sysErr)) {
            std::cerr << "  [ERROR] Register P501 failed: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Register P501 (Jonathan Wick): SUCCESS\n";
        if (!hospital.triagePatient("P501", UrgencyLevel::RESUSCITATION, sysErr)) {
            std::cerr << "  [ERROR] Triage P501 failed: " << sysErr << "\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after registering P501: " << sysErr << "\n";
            return 1;
        }

        // 3. Dynamic Urgency Update
        std::cout << "\n  [3] Dynamic Urgency Update (Condition Deterioration with Sequence Preservation):\n";
        Patient* p108 = hospital.getRegistry().findPatient("P108");
        if (!p108) {
            std::cerr << "  [ERROR] Patient P108 not found in registry.\n";
            return 1;
        }
        uint32_t origSeq108 = p108->arrivalSequence;
        if (!hospital.updateUrgency("P108", UrgencyLevel::RESUSCITATION, sysErr)) {
            std::cerr << "  [ERROR] Urgency update for P108 failed: " << sysErr << "\n";
            return 1;
        }
        if (p108->urgency != UrgencyLevel::RESUSCITATION || p108->arrivalSequence != origSeq108) {
            std::cerr << "  [ERROR] Urgency update corrupted urgency level or arrival sequence.\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after P108 urgency update: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Updated P108 to Urgency " << urgencyToString(p108->urgency) 
                  << "; arrivalSequence #" << p108->arrivalSequence << " strictly preserved.\n";

        // 4. Outpatient Appointment Handling
        std::cout << "\n  [4] Outpatient Appointment Processing (FIFO Call & Consultation Intake):\n";
        Patient* apptPatient = nullptr;
        if (!hospital.callNextAppointment(apptPatient, sysErr) || !apptPatient) {
            std::cerr << "  [ERROR] Failed to call next appointment: " << sysErr << "\n";
            return 1;
        }
        if (apptPatient->state != PatientState::UNDER_TREATMENT) {
            std::cerr << "  [ERROR] Called appointment patient state is not UNDER_TREATMENT.\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after calling appointment: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      FIFO Appointment Called: " << apptPatient->patientId 
                  << " (" << apptPatient->fullName << ") -> State: " 
                  << patientStateToString(apptPatient->state) << "\n";

        // 5. Dispatch Candidate Selection
        std::cout << "\n  [5] Deterministic Dispatch Candidate Selection:\n";
        Patient* dispatchCandidate = hospital.selectNextPatientForDispatch(sysErr);
        if (!dispatchCandidate || dispatchCandidate->state != PatientState::DISPATCH_PENDING) {
            std::cerr << "  [ERROR] Dispatch candidate selection failed: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Selected Highest-Priority Candidate: " << dispatchCandidate->patientId 
                  << " (" << dispatchCandidate->fullName << ")\n";
        std::cout << "      Urgency: " << urgencyToString(dispatchCandidate->urgency) 
                  << " | State: " << patientStateToString(dispatchCandidate->state) << "\n";

        // 6. Resource Assignment & Two-Leg Routing
        std::cout << "\n  [6] Dispatch Assignment & Whole-Leg Simulation Steps:\n";
        std::string targetPid = dispatchCandidate->patientId;
        if (!hospital.assignDispatch(targetPid, sysErr)) {
            std::cerr << "  [ERROR] Dispatch assignment failed for " << targetPid << ": " << sysErr << "\n";
            return 1;
        }
        std::string resId = dispatchCandidate->assignedResourceId;
        if (resId.empty()) {
            std::cerr << "  [ERROR] Assigned resource ID is empty for " << targetPid << "\n";
            return 1;
        }
        DispatchResource* resPtr = hospital.getDispatchManager().findResource(resId);
        if (!resPtr || resPtr->state != ResourceState::EN_ROUTE_PICKUP || resPtr->assignedPatientId != targetPid) {
            std::cerr << "  [ERROR] Assigned resource state mismatch for " << resId << "\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after assignment: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Assigned Resource: " << resId << " to patient " << targetPid << "\n";

        // Simulation Leg 1: Pickup
        if (!hospital.simulationStepPickup(targetPid, sysErr)) {
            std::cerr << "  [ERROR] Simulation step pickup failed: " << sysErr << "\n";
            return 1;
        }
        if (dispatchCandidate->state != PatientState::IN_TRANSIT ||
            resPtr->state != ResourceState::TRANSPORTING_PATIENT) {
            std::cerr << "  [ERROR] State mismatch after pickup transit.\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after pickup: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Leg 1 Pickup Complete! Patient State: " << patientStateToString(dispatchCandidate->state) << "\n";

        // Simulation Leg 2: Transport & Handover
        if (!hospital.simulationStepTransportAndHandover(targetPid, sysErr)) {
            std::cerr << "  [ERROR] Transport & Handover failed: " << sysErr << "\n";
            return 1;
        }
        if (dispatchCandidate->state != PatientState::UNDER_TREATMENT ||
            resPtr->state != ResourceState::AVAILABLE ||
            !resPtr->assignedPatientId.empty()) {
            std::cerr << "  [ERROR] State mismatch after handover and resource release.\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after handover: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Leg 2 Transport & Handover Complete! Patient State: " << patientStateToString(dispatchCandidate->state) << "\n";

        // 7. Clinical Discharge
        std::cout << "\n  [7] Clinical Patient Discharge:\n";
        if (!hospital.dischargePatient(targetPid, sysErr)) {
            std::cerr << "  [ERROR] Discharge failed for " << targetPid << ": " << sysErr << "\n";
            return 1;
        }
        if (dispatchCandidate->state != PatientState::DISCHARGED) {
            std::cerr << "  [ERROR] Patient state is not DISCHARGED after discharge.\n";
            return 1;
        }
        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "  [ERROR] Integrity validation failed after discharge: " << sysErr << "\n";
            return 1;
        }
        std::cout << "      Discharged Patient " << targetPid << ". Final State: " 
                  << patientStateToString(dispatchCandidate->state) << "\n";

        // Milestone 6: Report Snapshots & Demonstrations (CO1 - CO6)
        std::cout << "\n================================================================================\n";
        std::cout << "  MILESTONE 6: REPORT SNAPSHOTS & EDUCATIONAL DEMONSTRATIONS (CO1 - CO6)\n";
        std::cout << "================================================================================\n\n";

        std::vector<PatientSnapshot> snapshots = ReportingService::captureSnapshots(hospital);
        if (snapshots.size() != hospital.getRegistry().getPatientCount()) {
            std::cerr << "[ERROR] Captured snapshot count mismatch with registry.\n";
            return 1;
        }
        std::cout << "  [1] Generated " << snapshots.size() << " independent value-copy PatientSnapshot records.\n";
        std::cout << "      Simulation report timestamp: " << hospital.getSimulationTime() << "s.\n\n";

        ReportingService::sortSnapshots(snapshots, ReportingService::SortField::URGENCY, ReportingService::SortAlgorithm::MERGE_SORT);
        std::cout << ReportingService::formatReportTable(snapshots, ReportingService::SortField::URGENCY);

        DemonstrationService::runSortingComparison(snapshots, std::cout);
        DemonstrationService::runHashCollisionComparison(std::cout);
        DemonstrationService::runGraphTraversalComparison(std::cout);
        DemonstrationService::runBSTOperationsDemo(std::cout);
        DemonstrationService::runDijkstraVsMSTDemo(hospital.getMapManager(), std::cout);

        if (!hospital.validateSystemIntegrity(sysErr)) {
            std::cerr << "[ERROR] Final system integrity check failed: " << sysErr << "\n";
            return 1;
        }

        std::cout << "================================================================================\n";
        std::cout << "[DEMO COMPLETE] Noninteractive demonstration completed and self-validated successfully.\n";
        std::cout << "To launch the interactive menu, run without --demo: bin\\SmartHospitalSystem.exe\n";
        std::cout << "To execute the full automated test suite, run: bin\\TestRunner.exe\n";
        std::cout << "================================================================================\n";

        return 0;
    }

    /**
     * @brief Normal interactive launch mode. Launches ConsoleApp menu loop.
     */
    int runInteractiveMode(const std::string& dataDir) {
        HospitalSystem hospital;
        std::string err;
        if (!hospital.initializeFromSeed(dataDir, err)) {
            std::cerr << "[ERROR] Failed to initialize HospitalSystem from seed datasets: " << err << "\n";
            return 1;
        }

        if (!hospital.validateSystemIntegrity(err)) {
            std::cerr << "[ERROR] Pre-launch system integrity validation failed: " << err << "\n";
            return 1;
        }

        ConsoleApp app(hospital, std::cin, std::cout, dataDir, false);
        app.run();
        return 0;
    }
} // namespace

int main(int argc, char* argv[]) {
    bool isDemo = false;
    std::string dataDir = "";
    bool dataDirSpecified = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--demo") {
            isDemo = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "SMART HOSPITAL EMERGENCY TRIAGE & ROUTE DISPATCH SYSTEM (CS2305)\n\n"
                      << "Usage:\n"
                      << "  SmartHospitalSystem [options] [data_directory]\n\n"
                      << "Options:\n"
                      << "  --demo              Run deterministic noninteractive self-validating demonstration and exit 0.\n"
                      << "  --data-dir <path>   Specify path to synthetic seed data directory (default: data/).\n"
                      << "  --help, -h          Display this help documentation and exit 0.\n\n"
                      << "Positional Arguments:\n"
                      << "  data_directory      Optional path to synthetic seed data directory (e.g. \"data/\" or \"C:/path with spaces/data/\").\n";
            return 0;
        } else if (arg == "--data-dir") {
            if (dataDirSpecified) {
                std::cerr << "[ERROR] Conflicting or repeated data directory argument: '--data-dir'.\n";
                return 1;
            }
            if (i + 1 < argc) {
                dataDir = argv[++i];
                dataDirSpecified = true;
            } else {
                std::cerr << "[ERROR] --data-dir requires a directory path argument.\n";
                return 1;
            }
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "[ERROR] Unsupported command-line option: '" << arg << "'.\n";
            std::cerr << "Run 'SmartHospitalSystem --help' for available options.\n";
            return 1;
        } else {
            if (dataDirSpecified) {
                std::cerr << "[ERROR] Conflicting or repeated data directory argument: '" << arg << "'.\n";
                return 1;
            }
            dataDir = arg;
            dataDirSpecified = true;
        }
    }

    if (dataDir.empty()) {
        dataDir = resolveDataDir();
    } else {
        if (dataDir.back() != '/' && dataDir.back() != '\\') {
            dataDir += "/";
        }
    }

    if (isDemo) {
        return runNonInteractiveDemo(dataDir);
    } else {
        return runInteractiveMode(dataDir);
    }
}
