#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "models/CampusLocation.hpp"
#include "models/AuditEvent.hpp"

#include "ds/DoublyLinkedList.hpp"
#include "services/HospitalRegistry.hpp"
#include "services/CampusMapManager.hpp"
#include "services/TriageManager.hpp"
#include "services/AppointmentManager.hpp"
#include "services/DispatchManager.hpp"
#include "services/DataLoader.hpp"

/**
 * @brief Central Coordinating Facade for the Smart Hospital System.
 * 
 * DESIGN RATIONALE:
 * - Coordinates HospitalRegistry, TriageManager, AppointmentManager,
 *   CampusMapManager, and DispatchManager.
 * - Enforces the comprehensive patient lifecycle state machine.
 * - Records an immutable audit log using DoublyLinkedList<AuditEvent>.
 * - Maintains a deterministic pending dispatch priority policy:
 *     - Evaluates candidates from both unassigned pending dispatch pool and triage root.
 *     - Unassigned candidates never disappear from consideration upon assignment failure.
 *     - Evaluates priority by (urgency, arrivalSequence).
 * - Synchronizes simulation clock using 64-bit seconds with checked accumulation.
 * - Move construction and move assignment are strictly deleted because member
 *   service managers hold lvalue references to sibling components that cannot
 *   be safely rebound via default move operations.
 */
class HospitalSystem {
private:
    HospitalRegistry registry_;
    CampusMapManager campusMap_;
    TriageManager triageManager_;
    AppointmentManager appointmentManager_;
    DispatchManager dispatchManager_;

    DoublyLinkedList<AuditEvent> auditLog_;
    std::vector<Patient*> unassignedDispatchPending_;

    int64_t simulationTimeSeconds_{0};
    uint64_t nextArrivalSequence_{1};
    int nextInternalHandle_{0};
    bool isInitialized_{false};

    static int64_t safeTimeAdd(int64_t base, int64_t add) noexcept {
        if (add <= 0) return base;
        if (base > std::numeric_limits<int64_t>::max() - add) {
            return std::numeric_limits<int64_t>::max();
        }
        return base + add;
    }

    void logAuditEvent(const std::string& patientId, const std::string& action,
                       PatientState priorState, PatientState newState,
                       UrgencyLevel urgency, int locationNode,
                       const std::string& resourceId, const std::string& details) {
        AuditEvent event;
        event.patientId = patientId;
        event.action = action;
        event.priorState = priorState;
        event.newState = newState;
        event.urgencyAtEvent = urgency;
        event.locationAtEvent = locationNode;
        event.resourceIdAtEvent = resourceId;
        event.simulatedTimestamp = simulationTimeSeconds_;
        event.details = details;

        auditLog_.push_back(event);
    }

    void addUnassignedPending(Patient* p) {
        if (!p) return;
        if (p->state != PatientState::DISPATCH_PENDING || !p->assignedResourceId.empty()) return;
        for (Patient* existing : unassignedDispatchPending_) {
            if (existing == p) return; // Prevent duplicate pending entries
        }
        unassignedDispatchPending_.push_back(p);
    }

    void cleanUnassignedPendingList() {
        std::vector<Patient*> cleaned;
        cleaned.reserve(unassignedDispatchPending_.size());
        for (Patient* p : unassignedDispatchPending_) {
            if (p && p->state == PatientState::DISPATCH_PENDING && p->assignedResourceId.empty()) {
                if (std::find(cleaned.begin(), cleaned.end(), p) == cleaned.end()) {
                    cleaned.push_back(p);
                }
            }
        }
        unassignedDispatchPending_ = std::move(cleaned);
    }

public:
    HospitalSystem()
        : registry_(),
          campusMap_(12),
          triageManager_(registry_),
          appointmentManager_(registry_),
          dispatchManager_(campusMap_) {}

    // Disable copy operations: HospitalRegistry owns patient memory via unique_ptr
    HospitalSystem(const HospitalSystem&) = delete;
    HospitalSystem& operator=(const HospitalSystem&) = delete;

    // Disable move operations: Internal service managers hold references to sibling members
    // (registry_, campusMap_) which default moves do not rebind.
    HospitalSystem(HospitalSystem&&) = delete;
    HospitalSystem& operator=(HospitalSystem&&) = delete;

    // ========================================================================
    // A. LIFECYCLE & RESET
    // ========================================================================

    void reset() noexcept {
        registry_.clear();
        campusMap_.clear();
        triageManager_.clear();
        appointmentManager_.clear();
        dispatchManager_.clear();
        auditLog_.clear();
        unassignedDispatchPending_.clear();
        simulationTimeSeconds_ = 0;
        nextArrivalSequence_ = 1;
        nextInternalHandle_ = 0;
        isInitialized_ = false;
    }

    bool isInitialized() const noexcept {
        return isInitialized_;
    }

    // ========================================================================
    // B. SEED INITIALIZATION & REGISTRATION
    // ========================================================================

    /**
     * @brief Initializes campus topology and dispatch fleet from seed files without loading seed patients.
     *        Pre-validates map and resources in temporary memory before modifying the live system.
     */
    bool initializeCampusAndFleet(const std::string& dataDir, std::string& outError) {
        if (isInitialized_) {
            outError = "HospitalSystem is already initialized. Call reset() before re-initializing.";
            return false;
        }

        std::vector<CampusLocation> locations;
        std::vector<CorridorEdge> corridors;
        std::vector<DispatchResource> resources;

        std::string mapPath = dataDir + "campus_map_seed.txt";
        std::string resPath = dataDir + "resources_seed.csv";

        if (!DataLoader::loadCampusMapTXT(mapPath, locations, corridors, outError)) return false;
        if (!DataLoader::loadResourcesCSV(resPath, resources, outError)) return false;

        campusMap_.initialize(locations, corridors);

        for (const auto& r : resources) {
            if (!dispatchManager_.registerResource(r, outError)) {
                reset();
                return false;
            }
        }

        isInitialized_ = true;
        return true;
    }

    /**
     * @brief Initializes the campus topology, fleet, and seed patient cohorts.
     *        Pre-validates all seed data in temporary memory before mutating the live system.
     *        Rejects repeated initialization without explicit reset().
     *        Initializes simulation clock to >= latest seed arrival time.
     */
    bool initializeFromSeed(const std::string& dataDir, std::string& outError) {
        if (isInitialized_) {
            outError = "HospitalSystem is already initialized. Call reset() before re-initializing.";
            return false;
        }

        // Phase 1: Load and pre-validate all seed data in temporary containers
        std::vector<CampusLocation> locations;
        std::vector<CorridorEdge> corridors;
        std::vector<DispatchResource> resources;
        std::vector<Patient> seedPatients;

        std::string mapPath = dataDir + "campus_map_seed.txt";
        std::string resPath = dataDir + "resources_seed.csv";
        std::string patPath = dataDir + "patients_seed.csv";

        if (!DataLoader::loadCampusMapTXT(mapPath, locations, corridors, outError)) return false;
        if (!DataLoader::loadResourcesCSV(resPath, resources, outError)) return false;
        if (!DataLoader::loadPatientsCSV(patPath, seedPatients, outError)) return false;
        if (!DataLoader::validateCampusReferences(seedPatients, resources, locations, outError)) return false;

        // Check for states requiring assignments that seed format cannot reconstruct
        for (const auto& sp : seedPatients) {
            if (sp.state == PatientState::IN_TRANSIT || 
                sp.state == PatientState::DISPATCH_PENDING || 
                sp.state == PatientState::UNDER_TREATMENT) {
                outError = "Patient " + sp.patientId + " has state " + patientStateToString(sp.state) + 
                           " which requires active assignment context that the seed format cannot reconstruct.";
                return false;
            }
        }

        // Phase 2: All seeds validated; atomically initialize live components
        campusMap_.initialize(locations, corridors);

        for (const auto& r : resources) {
            if (!dispatchManager_.registerResource(r, outError)) {
                reset();
                return false;
            }
        }

        // Initialize simulation clock to at least the latest arrival time among seeds
        int64_t maxArrivalTime = 0;
        for (const auto& sp : seedPatients) {
            if (sp.simulatedArrivalTime > maxArrivalTime) {
                maxArrivalTime = sp.simulatedArrivalTime;
            }
        }
        simulationTimeSeconds_ = maxArrivalTime;

        for (auto& sp : seedPatients) {
            auto pPtr = std::make_unique<Patient>(sp);
            Patient* rawPtr = pPtr.get();

            if (pPtr->arrivalSequence >= nextArrivalSequence_) {
                nextArrivalSequence_ = pPtr->arrivalSequence + 1;
            }
            if (pPtr->internalHandle >= nextInternalHandle_) {
                nextInternalHandle_ = pPtr->internalHandle + 1;
            }

            PatientState targetInitialState = rawPtr->state;
            rawPtr->state = PatientState::REGISTERED; // Start registered

            if (!registry_.registerPatient(std::move(pPtr), outError)) {
                reset();
                return false;
            }

            logAuditEvent(rawPtr->patientId, "REGISTERED", PatientState::REGISTERED, PatientState::REGISTERED,
                          rawPtr->urgency, rawPtr->currentLocationNode, "", "Seed registration: " + rawPtr->fullName);

            if (targetInitialState == PatientState::TRIAGED_WAITING) {
                if (!triageManager_.enqueuePatient(rawPtr, outError)) {
                    reset();
                    return false;
                }
                logAuditEvent(rawPtr->patientId, "TRIAGED", PatientState::REGISTERED, PatientState::TRIAGED_WAITING,
                              rawPtr->urgency, rawPtr->currentLocationNode, "", "Seed triage enqueue");
            } else if (targetInitialState == PatientState::APPOINTMENT_SCHEDULED) {
                if (!appointmentManager_.scheduleAppointment(rawPtr, outError)) {
                    reset();
                    return false;
                }
                logAuditEvent(rawPtr->patientId, "APPOINTMENT_SCHEDULED", PatientState::REGISTERED, PatientState::APPOINTMENT_SCHEDULED,
                              rawPtr->urgency, rawPtr->currentLocationNode, "", "Seed appointment scheduled");
            } else if (targetInitialState == PatientState::DISCHARGED || targetInitialState == PatientState::CANCELLED) {
                rawPtr->state = targetInitialState;
                logAuditEvent(rawPtr->patientId, patientStateToString(targetInitialState), PatientState::REGISTERED, targetInitialState,
                              rawPtr->urgency, rawPtr->currentLocationNode, "", "Seed terminal state: " + patientStateToString(targetInitialState));
            } else if (targetInitialState != PatientState::REGISTERED) {
                outError = "Unsupported seed state for patient " + rawPtr->patientId + ": " + patientStateToString(targetInitialState);
                reset();
                return false;
            }
        }

        isInitialized_ = true;
        return true;
    }

    /**
     * @brief Registers a new patient into the hospital registry.
     *        Applies documented validation for ID, name, age, complaint, urgency,
     *        campus location nodes, and finite/non-negative billing amount.
     *        Pre-validates everything before mutating internal handle/sequence counters.
     */
    bool registerNewPatient(const std::string& patientId, const std::string& fullName,
                            int age, const std::string& complaint, UrgencyLevel urgency,
                            int currentLocationNode, int targetLocationNode, double billingAmount,
                            std::string& outError) {
        // 1. Patient ID validation
        if (patientId.empty() || patientId.size() > kMaxPatientIdLength) {
            outError = "Patient ID must be non-empty and at most " + std::to_string(kMaxPatientIdLength) + " characters.";
            return false;
        }
        if (patientId.find_first_of(" \t\r\n,") != std::string::npos) {
            outError = "Patient ID cannot contain whitespace or comma characters.";
            return false;
        }
        if (registry_.hasPatient(patientId)) {
            outError = "Duplicate patient ID: " + patientId + " is already registered.";
            return false;
        }

        // 2. Full name validation
        if (fullName.empty() || fullName.size() > 64) {
            outError = "Full name must be non-empty and at most 64 characters.";
            return false;
        }
        for (char c : fullName) {
            if (!(std::isalpha(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '\'')) {
                outError = "Full name contains unsupported character '" + std::string(1, c) + "'.";
                return false;
            }
        }

        // 3. Age validation [0, 150]
        if (age < 0 || age > 150) {
            outError = "Age must be between 0 and 150 (received: " + std::to_string(age) + ").";
            return false;
        }

        // 4. Complaint validation
        if (complaint.empty() || complaint.size() > 128) {
            outError = "Complaint must be non-empty and at most 128 characters.";
            return false;
        }

        // 5. Urgency validation [1, 5]
        if (!isValidUrgency(urgency)) {
            outError = "Urgency level must be between 1 and 5.";
            return false;
        }

        // 6. Node locations validation
        if (!campusMap_.isValidNode(currentLocationNode)) {
            outError = "Invalid current location node ID: " + std::to_string(currentLocationNode);
            return false;
        }
        if (!campusMap_.isValidNode(targetLocationNode)) {
            outError = "Invalid target location node ID: " + std::to_string(targetLocationNode);
            return false;
        }

        // 7. Billing amount validation (finite, >= 0.0, <= 1,000,000.0)
        if (!std::isfinite(billingAmount) || billingAmount < 0.0 || billingAmount > 1000000.0) {
            outError = "Billing amount must be a finite non-negative number <= 1,000,000.0.";
            return false;
        }

        // Pre-validation succeeded: instantiate and reserve counters
        auto p = std::make_unique<Patient>();
        p->internalHandle = nextInternalHandle_++;
        p->patientId = patientId;
        p->fullName = fullName;
        p->age = age;
        p->complaint = complaint;
        p->urgency = urgency;
        p->arrivalSequence = nextArrivalSequence_++;
        p->simulatedArrivalTime = simulationTimeSeconds_;
        p->state = PatientState::REGISTERED;
        p->currentLocationNode = currentLocationNode;
        p->targetLocationNode = targetLocationNode;
        p->billingAmount = billingAmount;

        Patient* rawPtr = p.get();
        if (!registry_.registerPatient(std::move(p), outError)) {
            // Revert counters on registration failure
            nextInternalHandle_--;
            nextArrivalSequence_--;
            return false;
        }

        logAuditEvent(rawPtr->patientId, "REGISTERED", PatientState::REGISTERED, PatientState::REGISTERED,
                      rawPtr->urgency, rawPtr->currentLocationNode, "", "New patient registered: " + rawPtr->fullName);
        return true;
    }

    // ========================================================================
    // C. TRIAGE OPERATIONS & DYNAMIC URGENCY UPDATES
    // ========================================================================

    /**
     * @brief Enqueues a registered patient into emergency triage.
     *        Pre-validates urgency (1-5), handle range, and registered state
     *        before modifying patient fields.
     */
    bool triagePatient(const std::string& patientId, UrgencyLevel urgency, std::string& outError) {
        if (!isValidUrgency(urgency)) {
            outError = "Invalid urgency level: must be 1 to 5.";
            return false;
        }

        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (p->state != PatientState::REGISTERED) {
            outError = "Patient " + patientId + " is in state " + patientStateToString(p->state) + 
                       "; must be REGISTERED to enter emergency triage.";
            logAuditEvent(patientId, "REJECTED_TRIAGE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        if (p->internalHandle < 0 || p->internalHandle >= TriageMinHeap::kMaxSupportedHandle) {
            outError = "Patient handle " + std::to_string(p->internalHandle) + " is out of supported range for triage.";
            logAuditEvent(patientId, "REJECTED_TRIAGE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        if (triageManager_.isPatientActive(p->internalHandle)) {
            outError = "Patient handle " + std::to_string(p->internalHandle) + " is already active in triage.";
            logAuditEvent(patientId, "REJECTED_TRIAGE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        UrgencyLevel oldUrgency = p->urgency;
        p->urgency = urgency;

        if (!triageManager_.enqueuePatient(p, outError)) {
            p->urgency = oldUrgency; // Restore urgency on intake failure
            logAuditEvent(patientId, "REJECTED_TRIAGE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        logAuditEvent(patientId, "TRIAGED", PatientState::REGISTERED, PatientState::TRIAGED_WAITING,
                      p->urgency, p->currentLocationNode, "", "Triage prioritized: " + urgencyToString(urgency));
        return true;
    }

    /**
     * @brief Updates patient urgency with strict 1-5 validation.
     *        Preserves original arrivalSequence.
     */
    bool updateUrgency(const std::string& patientId, UrgencyLevel newUrgency, std::string& outError) {
        if (!isValidUrgency(newUrgency)) {
            outError = "Invalid target urgency level: must be 1 to 5.";
            return false;
        }

        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (p->isDischarged() || p->isCancelled()) {
            outError = "Cannot change urgency for terminal patient " + patientId + " (" + patientStateToString(p->state) + ").";
            logAuditEvent(patientId, "REJECTED_URGENCY_UPDATE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        UrgencyLevel oldUrgency = p->urgency;

        if (p->state == PatientState::TRIAGED_WAITING) {
            if (!triageManager_.updateUrgency(p, newUrgency, outError)) {
                logAuditEvent(patientId, "REJECTED_URGENCY_UPDATE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
                return false;
            }
        } else if (p->isDispatchUnassigned()) {
            p->urgency = newUrgency; // arrivalSequence preserved
        } else {
            p->urgency = newUrgency;
        }

        logAuditEvent(patientId, "URGENCY_UPDATED", p->state, p->state, p->urgency, p->currentLocationNode, p->assignedResourceId,
                      "Urgency changed from " + urgencyToString(oldUrgency) + " to " + urgencyToString(newUrgency) + 
                      "; arrivalSequence preserved (" + std::to_string(p->arrivalSequence) + ")");
        return true;
    }

    // ========================================================================
    // D. APPOINTMENT OPERATIONS & FIFO PROCESSING
    // ========================================================================

    bool scheduleAppointment(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (p->state != PatientState::REGISTERED) {
            outError = "Patient " + patientId + " is in state " + patientStateToString(p->state) + 
                       "; must be REGISTERED to schedule appointment.";
            logAuditEvent(patientId, "REJECTED_APPOINTMENT", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        if (!appointmentManager_.scheduleAppointment(p, outError)) {
            logAuditEvent(patientId, "REJECTED_APPOINTMENT", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        logAuditEvent(patientId, "APPOINTMENT_SCHEDULED", PatientState::REGISTERED, PatientState::APPOINTMENT_SCHEDULED,
                      p->urgency, p->currentLocationNode, "", "Outpatient clinic appointment booked");
        return true;
    }

    bool cancelAppointment(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (!appointmentManager_.cancelAppointment(p, outError)) {
            logAuditEvent(patientId, "REJECTED_APPOINTMENT_CANCEL", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        logAuditEvent(patientId, "APPOINTMENT_CANCELLED", PatientState::APPOINTMENT_SCHEDULED, PatientState::CANCELLED,
                      p->urgency, p->currentLocationNode, "", "Appointment cancelled via encapsulated queue removal");
        return true;
    }

    bool callNextAppointment(Patient*& outPatient, std::string& outError) {
        if (!appointmentManager_.processNextAppointment(outPatient, outError)) {
            return false;
        }

        logAuditEvent(outPatient->patientId, "APPOINTMENT_CALLED", PatientState::APPOINTMENT_SCHEDULED, PatientState::UNDER_TREATMENT,
                      outPatient->urgency, outPatient->currentLocationNode, "", "Consultation turn called in FIFO order; UNDER_TREATMENT");
        return true;
    }

    bool convertAppointmentToEmergency(const std::string& patientId, UrgencyLevel newUrgency, std::string& outError) {
        if (!isValidUrgency(newUrgency)) {
            outError = "Invalid target urgency: must be 1 to 5.";
            return false;
        }

        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (!appointmentManager_.convertToEmergency(p, newUrgency, triageManager_, outError)) {
            logAuditEvent(patientId, "REJECTED_EMERGENCY_CONVERSION", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        logAuditEvent(patientId, "APPOINTMENT_CONVERTED_TO_EMERGENCY", PatientState::APPOINTMENT_SCHEDULED, PatientState::TRIAGED_WAITING,
                      p->urgency, p->currentLocationNode, "", "Condition deteriorated; moved from appointment queue to emergency triage");
        return true;
    }

    // ========================================================================
    // E. DETERMINISTIC PENDING DISPATCH SELECTION & ASSIGNMENT
    // ========================================================================

    Patient* selectNextPatientForDispatch(std::string& outError) {
        cleanUnassignedPendingList();

        // 1. Evaluate best unassigned pending candidate
        Patient* bestPending = nullptr;
        for (Patient* p : unassignedDispatchPending_) {
            if (!p || p->state != PatientState::DISPATCH_PENDING || !p->assignedResourceId.empty()) continue;
            if (bestPending == nullptr) {
                bestPending = p;
            } else {
                int urgP = urgencyToInt(p->urgency);
                int urgBest = urgencyToInt(bestPending->urgency);
                if (urgP < urgBest || (urgP == urgBest && p->arrivalSequence < bestPending->arrivalSequence)) {
                    bestPending = p;
                }
            }
        }

        // 2. Peek at triage heap top
        Patient* triageCandidate = nullptr;
        triageManager_.peekHighestPriority(triageCandidate);

        // 3. Selection arbitration
        if (!bestPending && !triageCandidate) {
            outError = "No eligible emergency patients waiting for dispatch.";
            return nullptr;
        }

        if (bestPending && !triageCandidate) {
            return bestPending;
        }

        if (!bestPending && triageCandidate) {
            Patient* extracted = nullptr;
            if (triageManager_.extractHighestPriority(extracted, outError)) {
                addUnassignedPending(extracted);
                logAuditEvent(extracted->patientId, "DISPATCH_SELECTED", PatientState::TRIAGED_WAITING, PatientState::DISPATCH_PENDING,
                              extracted->urgency, extracted->currentLocationNode, "", "Extracted from triage for dispatch consideration");
                return extracted;
            }
            return nullptr;
        }

        int urgTriage = urgencyToInt(triageCandidate->urgency);
        int urgPending = urgencyToInt(bestPending->urgency);

        bool triageIsHigher = (urgTriage < urgPending) || 
                              (urgTriage == urgPending && triageCandidate->arrivalSequence < bestPending->arrivalSequence);

        if (triageIsHigher) {
            Patient* extracted = nullptr;
            if (triageManager_.extractHighestPriority(extracted, outError)) {
                addUnassignedPending(extracted);
                logAuditEvent(extracted->patientId, "DISPATCH_SELECTED", PatientState::TRIAGED_WAITING, PatientState::DISPATCH_PENDING,
                              extracted->urgency, extracted->currentLocationNode, "", "Extracted higher-urgency triage patient for dispatch consideration");
                return extracted;
            }
            return bestPending;
        } else {
            return bestPending;
        }
    }

    bool assignDispatch(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        PatientState priorState = p->state;
        std::string priorRes = p->assignedResourceId;
        int priorLoc = p->currentLocationNode;
        UrgencyLevel priorUrg = p->urgency;

        if (priorState != PatientState::DISPATCH_PENDING) {
            outError = "Patient " + patientId + " is in state " + patientStateToString(priorState) + 
                       "; must be DISPATCH_PENDING.";
            logAuditEvent(patientId, "REJECTED_INVALID_STATE", priorState, priorState, priorUrg, priorLoc, priorRes, outError);
            return false;
        }

        if (!priorRes.empty()) {
            outError = "Patient " + patientId + " is already assigned to resource " + priorRes + 
                       ". Duplicate assignment rejected.";
            logAuditEvent(patientId, "REJECTED_ALREADY_ASSIGNED", priorState, priorState, priorUrg, priorLoc, priorRes, outError);
            return false;
        }

        DispatchResource* res = nullptr;
        DijkstraResult leg1Route;
        DijkstraResult leg2Route;

        if (!dispatchManager_.assignResource(p, res, leg1Route, leg2Route, outError)) {
            // Keep in unassigned pending pool
            addUnassignedPending(p);

            std::string action = "REJECTED_ASSIGNMENT";
            if (outError.find("unreachable") != std::string::npos) {
                action = "REJECTED_UNREACHABLE_DESTINATION";
            } else if (outError.find("No available") != std::string::npos) {
                action = "REJECTED_NO_AVAILABLE_RESOURCE";
            }

            logAuditEvent(patientId, action, priorState, p->state, priorUrg, priorLoc, p->assignedResourceId, outError);
            return false;
        }

        cleanUnassignedPendingList();

        logAuditEvent(patientId, "DISPATCH_ASSIGNED", PatientState::DISPATCH_PENDING, PatientState::DISPATCH_PENDING,
                      p->urgency, p->currentLocationNode, res->resourceId,
                      "Resource " + res->resourceId + " assigned (Leg 1 pickup: " + std::to_string(leg1Route.totalCost) + 
                      "s, Leg 2 transport: " + std::to_string(leg2Route.totalCost) + "s)");
        return true;
    }

    // ========================================================================
    // F. SIMULATION TRANSIT STEPS & HANDOVER (WHOLE-LEG STEPS)
    // ========================================================================

    /**
     * @brief Executes Leg 1 movement: Resource travels to patient pickup point.
     *        Movement is modeled as a discrete whole-leg step.
     */
    bool simulationStepPickup(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        PatientState priorState = p->state;
        std::string priorRes = p->assignedResourceId;
        int priorLoc = p->currentLocationNode;
        UrgencyLevel priorUrg = p->urgency;

        int64_t elapsedSeconds = 0;
        if (!dispatchManager_.stepPickup(p, elapsedSeconds, outError)) {
            std::string action = "REJECTED_PICKUP";
            if (priorState != PatientState::DISPATCH_PENDING) {
                action = "REJECTED_INVALID_STATE";
            } else if (priorRes.empty()) {
                action = "REJECTED_MISSING_RESOURCE";
            } else if (outError.find("Corridor closure") != std::string::npos) {
                action = "REJECTED_PICKUP_CORRIDOR_BLOCKED";
                addUnassignedPending(p); // Reverted to unassigned pending
            } else if (outError.find("mismatch") != std::string::npos) {
                action = "REJECTED_ASSIGNMENT_MISMATCH";
            }

            logAuditEvent(patientId, action, priorState, p->state, priorUrg, priorLoc, p->assignedResourceId, outError);
            return false;
        }

        simulationTimeSeconds_ = safeTimeAdd(simulationTimeSeconds_, elapsedSeconds);
        logAuditEvent(patientId, "PICKUP_COMPLETE", PatientState::DISPATCH_PENDING, PatientState::IN_TRANSIT,
                      p->urgency, p->currentLocationNode, p->assignedResourceId,
                      "Resource reached pickup point in " + std::to_string(elapsedSeconds) + "s; patient IN_TRANSIT");
        return true;
    }

    /**
     * @brief Executes Leg 2 movement: Resource transports patient to destination and handsoff.
     *        Movement is modeled as a discrete whole-leg step.
     */
    bool simulationStepTransportAndHandover(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        PatientState priorState = p->state;
        std::string priorRes = p->assignedResourceId;
        int priorLoc = p->currentLocationNode;
        UrgencyLevel priorUrg = p->urgency;

        int64_t elapsedSeconds = 0;
        if (!dispatchManager_.stepTransportAndHandover(p, elapsedSeconds, outError)) {
            std::string action = "REJECTED_TRANSPORT";
            if (priorState != PatientState::IN_TRANSIT) {
                action = "REJECTED_INVALID_STATE";
            } else if (priorRes.empty()) {
                action = "REJECTED_MISSING_RESOURCE";
            } else if (outError.find("Corridor closure") != std::string::npos) {
                action = "REJECTED_TRANSPORT_CORRIDOR_BLOCKED";
            } else if (outError.find("mismatch") != std::string::npos) {
                action = "REJECTED_ASSIGNMENT_MISMATCH";
            }

            logAuditEvent(patientId, action, priorState, p->state, priorUrg, priorLoc, priorRes, outError);
            return false;
        }

        simulationTimeSeconds_ = safeTimeAdd(simulationTimeSeconds_, elapsedSeconds);
        logAuditEvent(patientId, "HANDOVER_COMPLETE", PatientState::IN_TRANSIT, PatientState::UNDER_TREATMENT,
                      p->urgency, p->currentLocationNode, "",
                      "Handover complete at node " + std::to_string(p->currentLocationNode) + 
                      " in " + std::to_string(elapsedSeconds) + "s; resource released to AVAILABLE; patient UNDER_TREATMENT");
        return true;
    }

    bool cancelPickup(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        std::string prevRes = p->assignedResourceId;
        PatientState prior = p->state;

        if (!dispatchManager_.cancelPickup(p, outError)) {
            logAuditEvent(patientId, "REJECTED_CANCELLATION", prior, prior, p->urgency, p->currentLocationNode, prevRes, outError);
            return false;
        }

        cleanUnassignedPendingList();
        logAuditEvent(patientId, "DISPATCH_CANCELLED", prior, PatientState::CANCELLED, p->urgency, p->currentLocationNode, "",
                      "Dispatch cancelled during pickup; resource released immediately");
        return true;
    }

    // ========================================================================
    // G. PATIENT DISCHARGE & ISOLATION
    // ========================================================================

    bool dischargePatient(const std::string& patientId, std::string& outError) {
        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' not found in registry.";
            return false;
        }

        if (p->state != PatientState::UNDER_TREATMENT) {
            outError = "Cannot discharge patient " + patientId + ": current state is " + 
                       patientStateToString(p->state) + " (must be UNDER_TREATMENT).";
            logAuditEvent(patientId, "REJECTED_DISCHARGE", p->state, p->state, p->urgency, p->currentLocationNode, "", outError);
            return false;
        }

        p->state = PatientState::DISCHARGED;
        logAuditEvent(patientId, "DISCHARGED", PatientState::UNDER_TREATMENT, PatientState::DISCHARGED,
                      p->urgency, p->currentLocationNode, "", "Clinical treatment finished; patient successfully discharged");
        return true;
    }

    // ========================================================================
    // H. SYSTEM CONSISTENCY CHECKER FOR TESTS
    // ========================================================================

    /**
     * @brief Comprehensive cross-service consistency checker.
     * 
     * Verifies:
     * 1. Waiting patients correspond to triage entries.
     * 2. Scheduled appointments correspond to queue entries.
     * 3. Pending patients occur once in the appropriate pool.
     * 4. Assigned patients and resources refer to each other with compatible states.
     * 5. Terminal patients have no active queue/heap membership or assignment.
     */
    bool validateSystemIntegrity(std::string& outError) const {
        // 1. Triage heap internal integrity
        if (!triageManager_.getHeap().validateInvariants()) {
            outError = "Triage min-heap invariants violated.";
            return false;
        }

        // 2. Validate all registered patients
        std::vector<Patient*> allPatients = registry_.getAllPatients();
        for (Patient* p : allPatients) {
            if (!p) {
                outError = "Null pointer encountered in patient registry.";
                return false;
            }

            switch (p->state) {
                case PatientState::REGISTERED:
                    if (triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " is in REGISTERED state but active in triage heap.";
                        return false;
                    }
                    if (appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " is in REGISTERED state but present in appointment queue.";
                        return false;
                    }
                    if (!p->assignedResourceId.empty()) {
                        outError = "Patient " + p->patientId + " is in REGISTERED state but has assigned resource " + p->assignedResourceId;
                        return false;
                    }
                    break;

                case PatientState::TRIAGED_WAITING:
                    if (!triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " is in TRIAGED_WAITING state but missing from triage heap.";
                        return false;
                    }
                    if (appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " is in TRIAGED_WAITING state but present in appointment queue.";
                        return false;
                    }
                    if (!p->assignedResourceId.empty()) {
                        outError = "Patient " + p->patientId + " is in TRIAGED_WAITING state but has assigned resource " + p->assignedResourceId;
                        return false;
                    }
                    break;

                case PatientState::APPOINTMENT_SCHEDULED:
                    if (triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " is in APPOINTMENT_SCHEDULED state but active in triage heap.";
                        return false;
                    }
                    if (!appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " is in APPOINTMENT_SCHEDULED state but missing from appointment queue.";
                        return false;
                    }
                    if (!p->assignedResourceId.empty()) {
                        outError = "Patient " + p->patientId + " is in APPOINTMENT_SCHEDULED state but has assigned resource " + p->assignedResourceId;
                        return false;
                    }
                    break;

                case PatientState::DISPATCH_PENDING:
                    if (triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " is in DISPATCH_PENDING state but active in triage heap.";
                        return false;
                    }
                    if (appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " is in DISPATCH_PENDING state but present in appointment queue.";
                        return false;
                    }
                    if (p->assignedResourceId.empty()) {
                        // Unassigned pending: must appear exactly once in unassignedDispatchPending_
                        int count = 0;
                        for (Patient* pend : unassignedDispatchPending_) {
                            if (pend == p) count++;
                        }
                        if (count != 1) {
                            outError = "Unassigned pending patient " + p->patientId + " occurs " + 
                                       std::to_string(count) + " times in unassignedDispatchPending_ pool (expected 1).";
                            return false;
                        }
                    } else {
                        // Assigned pending: must NOT appear in unassignedDispatchPending_
                        for (Patient* pend : unassignedDispatchPending_) {
                            if (pend == p) {
                                outError = "Assigned pending patient " + p->patientId + " found in unassignedDispatchPending_ pool.";
                                return false;
                            }
                        }
                        const DispatchResource* res = dispatchManager_.findResource(p->assignedResourceId);
                        if (!res) {
                            outError = "Patient " + p->patientId + " assigned resource " + p->assignedResourceId + " which does not exist.";
                            return false;
                        }
                        if (res->assignedPatientId != p->patientId) {
                            outError = "Reciprocal mismatch: patient " + p->patientId + " assigned to " + res->resourceId + 
                                       ", but resource is assigned to '" + res->assignedPatientId + "'.";
                            return false;
                        }
                        if (res->state != ResourceState::EN_ROUTE_PICKUP) {
                            outError = "State mismatch: patient " + p->patientId + " is DISPATCH_PENDING, but resource " + 
                                       res->resourceId + " is " + resourceStateToString(res->state) + " (expected EN_ROUTE_PICKUP).";
                            return false;
                        }
                    }
                    break;

                case PatientState::IN_TRANSIT:
                    if (triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " is IN_TRANSIT but active in triage heap.";
                        return false;
                    }
                    if (appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " is IN_TRANSIT but present in appointment queue.";
                        return false;
                    }
                    for (Patient* pend : unassignedDispatchPending_) {
                        if (pend == p) {
                            outError = "In-transit patient " + p->patientId + " found in unassignedDispatchPending_ pool.";
                            return false;
                        }
                    }
                    if (p->assignedResourceId.empty()) {
                        outError = "Patient " + p->patientId + " is IN_TRANSIT but has no assigned resource.";
                        return false;
                    } else {
                        const DispatchResource* res = dispatchManager_.findResource(p->assignedResourceId);
                        if (!res) {
                            outError = "Patient " + p->patientId + " assigned resource " + p->assignedResourceId + " which does not exist.";
                            return false;
                        }
                        if (res->assignedPatientId != p->patientId) {
                            outError = "Reciprocal mismatch: patient " + p->patientId + " assigned to " + res->resourceId + 
                                       ", but resource is assigned to '" + res->assignedPatientId + "'.";
                            return false;
                        }
                        if (res->state != ResourceState::TRANSPORTING_PATIENT) {
                            outError = "State mismatch: patient " + p->patientId + " is IN_TRANSIT, but resource " + 
                                       res->resourceId + " is " + resourceStateToString(res->state) + " (expected TRANSPORTING_PATIENT).";
                            return false;
                        }
                    }
                    break;

                case PatientState::UNDER_TREATMENT:
                case PatientState::DISCHARGED:
                case PatientState::CANCELLED:
                    if (triageManager_.isPatientActive(p->internalHandle)) {
                        outError = "Patient " + p->patientId + " in terminal/treatment state (" + 
                                   patientStateToString(p->state) + ") but active in triage heap.";
                        return false;
                    }
                    if (appointmentManager_.contains(p->patientId)) {
                        outError = "Patient " + p->patientId + " in terminal/treatment state (" + 
                                   patientStateToString(p->state) + ") but in appointment queue.";
                        return false;
                    }
                    for (Patient* pend : unassignedDispatchPending_) {
                        if (pend == p) {
                            outError = "Patient " + p->patientId + " in state " + patientStateToString(p->state) + 
                                       " found in unassignedDispatchPending_ pool.";
                            return false;
                        }
                    }
                    if (!p->assignedResourceId.empty()) {
                        outError = "Patient " + p->patientId + " in state " + patientStateToString(p->state) + 
                                   " has non-empty assigned resource " + p->assignedResourceId;
                        return false;
                    }
                    break;
            }
        }

        // 3. Reverse check triage heap entries
        for (const auto& rec : triageManager_.getHeap().getRawRecords()) {
            Patient* p = registry_.findPatientByHandle(rec.internalHandle);
            if (!p) {
                outError = "Triage heap contains handle " + std::to_string(rec.internalHandle) + " not in registry.";
                return false;
            }
            if (p->state != PatientState::TRIAGED_WAITING) {
                outError = "Triage heap contains patient " + p->patientId + " whose state is " + 
                           patientStateToString(p->state) + " (expected TRIAGED_WAITING).";
                return false;
            }
            if (p->urgency != rec.urgency) {
                outError = "Triage record urgency mismatch for patient " + p->patientId;
                return false;
            }
        }

        // 4. Reverse check appointment queue entries
        std::vector<std::string> apptIds = appointmentManager_.getScheduledPatientIds();
        for (const auto& pid : apptIds) {
            Patient* p = registry_.findPatient(pid);
            if (!p) {
                outError = "Appointment queue contains ID " + pid + " not in registry.";
                return false;
            }
            if (p->state != PatientState::APPOINTMENT_SCHEDULED) {
                outError = "Appointment queue contains patient " + pid + " whose state is " + 
                           patientStateToString(p->state) + " (expected APPOINTMENT_SCHEDULED).";
                return false;
            }
        }

        // 5. Check all resources
        for (const auto& r : dispatchManager_.getAllResources()) {
            switch (r.state) {
                case ResourceState::AVAILABLE:
                    if (!r.assignedPatientId.empty()) {
                        outError = "Resource " + r.resourceId + " is AVAILABLE but has assignedPatientId '" + r.assignedPatientId + "'.";
                        return false;
                    }
                    break;
                case ResourceState::EN_ROUTE_PICKUP: {
                    if (r.assignedPatientId.empty()) {
                        outError = "Resource " + r.resourceId + " is EN_ROUTE_PICKUP but assignedPatientId is empty.";
                        return false;
                    }
                    Patient* p = registry_.findPatient(r.assignedPatientId);
                    if (!p) {
                        outError = "Resource " + r.resourceId + " assigned to unknown patient " + r.assignedPatientId;
                        return false;
                    }
                    if (p->state != PatientState::DISPATCH_PENDING || p->assignedResourceId != r.resourceId) {
                        outError = "Resource " + r.resourceId + " assignment mismatch with patient " + r.assignedPatientId;
                        return false;
                    }
                    break;
                }
                case ResourceState::TRANSPORTING_PATIENT: {
                    if (r.assignedPatientId.empty()) {
                        outError = "Resource " + r.resourceId + " is TRANSPORTING_PATIENT but assignedPatientId is empty.";
                        return false;
                    }
                    Patient* p = registry_.findPatient(r.assignedPatientId);
                    if (!p) {
                        outError = "Resource " + r.resourceId + " assigned to unknown patient " + r.assignedPatientId;
                        return false;
                    }
                    if (p->state != PatientState::IN_TRANSIT || p->assignedResourceId != r.resourceId) {
                        outError = "Resource " + r.resourceId + " transport mismatch with patient " + r.assignedPatientId;
                        return false;
                    }
                    break;
                }
            }
        }

        return true;
    }

    // ========================================================================
    // I. COMPREHENSIVE FACADE METHODS FOR UI & SYSTEM COORDINATION
    // ========================================================================

    // --- Campus Map & Routing Facades ---
    bool blockCorridor(int u, int v, std::string& outError) {
        if (!campusMap_.isValidNode(u) || !campusMap_.isValidNode(v)) {
            outError = "Invalid node ID(s) for corridor: " + std::to_string(u) + ", " + std::to_string(v);
            return false;
        }
        if (campusMap_.getGraph().getOriginalCorridorWeight(u, v) == CampusGraph::NO_EDGE) {
            outError = "No corridor exists between node " + std::to_string(u) + " and node " + std::to_string(v);
            return false;
        }
        if (!campusMap_.blockCorridor(u, v)) {
            outError = "Failed to block corridor between " + std::to_string(u) + " and " + std::to_string(v);
            logAuditEvent("", "REJECTED_CORRIDOR_BLOCK", PatientState::REGISTERED, PatientState::REGISTERED,
                          UrgencyLevel::NON_URGENT, u, "", outError);
            return false;
        }
        logAuditEvent("", "CORRIDOR_BLOCKED", PatientState::REGISTERED, PatientState::REGISTERED,
                      UrgencyLevel::NON_URGENT, u, "", "Corridor (" + std::to_string(u) + " <-> " + std::to_string(v) + ") blocked");
        return true;
    }

    bool reopenCorridor(int u, int v, std::string& outError) {
        if (!campusMap_.isValidNode(u) || !campusMap_.isValidNode(v)) {
            outError = "Invalid node ID(s) for corridor: " + std::to_string(u) + ", " + std::to_string(v);
            return false;
        }
        if (campusMap_.getGraph().getOriginalCorridorWeight(u, v) == CampusGraph::NO_EDGE) {
            outError = "No corridor exists between node " + std::to_string(u) + " and node " + std::to_string(v);
            return false;
        }
        if (!campusMap_.reopenCorridor(u, v)) {
            outError = "Failed to reopen corridor between " + std::to_string(u) + " and " + std::to_string(v);
            logAuditEvent("", "REJECTED_CORRIDOR_REOPEN", PatientState::REGISTERED, PatientState::REGISTERED,
                          UrgencyLevel::NON_URGENT, u, "", outError);
            return false;
        }
        logAuditEvent("", "CORRIDOR_REOPENED", PatientState::REGISTERED, PatientState::REGISTERED,
                      UrgencyLevel::NON_URGENT, u, "", "Corridor (" + std::to_string(u) + " <-> " + std::to_string(v) + ") reopened");
        return true;
    }

    DijkstraResult findRoute(int source, int destination) const {
        return campusMap_.findShortestRoute(source, destination);
    }

    MSTResult computeKruskalMST() const {
        return campusMap_.computeKruskalMST();
    }

    MSTResult computePrimMST() const {
        return campusMap_.computePrimMST();
    }

    std::vector<int> getAccessibleNodes(int startNode) const {
        return campusMap_.bfsAccessibleNodes(startNode);
    }

    std::vector<int> getBFSOrder(int startNode) const {
        return campusMap_.getGraph().bfs(startNode);
    }

    std::vector<int> getDFSOrder(int startNode) const {
        return campusMap_.getGraph().dfsList(startNode);
    }

    bool isValidNode(int nodeId) const {
        return campusMap_.isValidNode(nodeId);
    }

    std::string getNodeName(int nodeId) const {
        return campusMap_.getNodeName(nodeId);
    }

    const std::vector<CampusLocation>& getAllLocations() const {
        return campusMap_.getAllLocations();
    }

    std::vector<CorridorEdge> getAllCorridors() const {
        return campusMap_.getAllCorridors();
    }

    bool isCorridorOpen(int u, int v) const {
        return campusMap_.isCorridorOpen(u, v);
    }

    // --- Registry Facades ---
    const Patient* findPatient(const std::string& patientId) const {
        return registry_.findPatient(patientId);
    }

    size_t getPatientCount() const noexcept {
        return registry_.getPatientCount();
    }

    std::vector<Patient*> getAllPatients() const {
        return registry_.getAllPatients();
    }

    // --- Triage Facades ---
    size_t getTriageQueueSize() const noexcept {
        return triageManager_.size();
    }

    bool peekTriageHighestPriority(Patient*& outPatient) const {
        return triageManager_.peekHighestPriority(outPatient);
    }

    // --- Appointment Facades ---
    size_t getAppointmentQueueSize() const noexcept {
        return appointmentManager_.size();
    }

    std::vector<std::string> getScheduledAppointmentPatientIds() const {
        return appointmentManager_.getScheduledPatientIds();
    }

    // --- Fleet Facades ---
    std::vector<DispatchResource> getAllResources() const {
        return dispatchManager_.getAllResources();
    }

    const DispatchResource* findResource(const std::string& resourceId) const {
        return dispatchManager_.findResource(resourceId);
    }

    // ========================================================================
    // J. SERVICE ACCESSORS & AUDIT INSPECTIONS
    // ========================================================================

    const HospitalRegistry& getRegistry() const noexcept { return registry_; }
    HospitalRegistry& getRegistry() noexcept { return registry_; }

    const CampusMapManager& getMapManager() const noexcept { return campusMap_; }
    CampusMapManager& getMapManager() noexcept { return campusMap_; }

    const TriageManager& getTriageManager() const noexcept { return triageManager_; }
    TriageManager& getTriageManager() noexcept { return triageManager_; }

    const AppointmentManager& getAppointmentManager() const noexcept { return appointmentManager_; }
    AppointmentManager& getAppointmentManager() noexcept { return appointmentManager_; }

    const DispatchManager& getDispatchManager() const noexcept { return dispatchManager_; }
    DispatchManager& getDispatchManager() noexcept { return dispatchManager_; }

    const DoublyLinkedList<AuditEvent>& getAuditLog() const noexcept { return auditLog_; }

    std::vector<AuditEvent> getAuditEvents() const {
        return auditLog_.to_vector_forward();
    }

    int64_t getSimulationTime() const noexcept { return simulationTimeSeconds_; }
    void advanceSimulationTime(int64_t seconds) noexcept { simulationTimeSeconds_ = safeTimeAdd(simulationTimeSeconds_, seconds); }

    const std::vector<Patient*>& getUnassignedPendingPatients() const noexcept {
        return unassignedDispatchPending_;
    }
};

// Compile-time checks for move/copy policy
static_assert(!std::is_copy_constructible<HospitalSystem>::value, "HospitalSystem must not be copy constructible");
static_assert(!std::is_copy_assignable<HospitalSystem>::value, "HospitalSystem must not be copy assignable");
static_assert(!std::is_move_constructible<HospitalSystem>::value, "HospitalSystem must not be move constructible");
static_assert(!std::is_move_assignable<HospitalSystem>::value, "HospitalSystem must not be move assignable");
