#pragma once

#include <string>
#include "models/Patient.hpp"
#include "ds/BinaryMinHeap.hpp"
#include "services/HospitalRegistry.hpp"

/**
 * @brief Triage Queue Manager.
 * 
 * DESIGN RATIONALE:
 * - Manages patients waiting for emergency treatment/dispatch using TriageMinHeap.
 * - Primary key: UrgencyLevel (1 = Resuscitation down to 5 = Non-Urgent).
 * - Secondary key: Monotonic arrivalSequence for strictly deterministic FIFO tie-breaking.
 * - Supports dynamic urgency changes with sift-up/down in O(log n) time while
 *   strictly preserving the original arrivalSequence.
 * - Directly accesses authoritative Patient records in HospitalRegistry via stable dense handles.
 */
class TriageManager {
private:
    TriageMinHeap heap_;
    HospitalRegistry& registry_;

public:
    explicit TriageManager(HospitalRegistry& registry) noexcept
        : registry_(registry) {}

    /**
     * @brief Enqueues a registered patient into the emergency triage min-heap.
     */
    bool enqueuePatient(Patient* patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot enqueue null patient pointer into triage.";
            return false;
        }

        if (patient->state != PatientState::REGISTERED) {
            outError = "Patient " + patient->patientId + " is in state " + 
                       patientStateToString(patient->state) + "; must be REGISTERED to enter triage.";
            return false;
        }

        int urgVal = urgencyToInt(patient->urgency);
        if (urgVal < 1 || urgVal > 5) {
            outError = "Invalid urgency level " + std::to_string(urgVal) + " (must be 1 to 5).";
            return false;
        }

        TriageRecord rec(patient->internalHandle, patient->patientId, patient->urgency, patient->arrivalSequence);
        if (!heap_.insert(rec)) {
            outError = "Failed to insert patient " + patient->patientId + " into triage heap (handle: " + 
                       std::to_string(patient->internalHandle) + ").";
            return false;
        }

        patient->state = PatientState::TRIAGED_WAITING;
        return true;
    }

    /**
     * @brief Updates urgency of a waiting patient, adjusting heap position in O(log n) time
     *        while strictly preserving original arrivalSequence.
     */
    bool updateUrgency(Patient* patient, UrgencyLevel newUrgency, std::string& outError) {
        if (!patient) {
            outError = "Cannot update urgency on null patient.";
            return false;
        }

        if (patient->state != PatientState::TRIAGED_WAITING) {
            outError = "Patient " + patient->patientId + " is not in TRIAGED_WAITING state (current: " + 
                       patientStateToString(patient->state) + ").";
            return false;
        }

        int urgVal = urgencyToInt(newUrgency);
        if (urgVal < 1 || urgVal > 5) {
            outError = "Invalid target urgency " + std::to_string(urgVal) + " (must be 1 to 5).";
            return false;
        }

        if (!heap_.changeUrgency(patient->internalHandle, newUrgency)) {
            outError = "Failed to update urgency for handle " + std::to_string(patient->internalHandle) + " in heap.";
            return false;
        }

        patient->urgency = newUrgency; // arrivalSequence is kept unchanged!
        return true;
    }

    /**
     * @brief Peeks at the highest-priority waiting patient without removing them from heap.
     */
    bool peekHighestPriority(Patient*& outPatient) const noexcept {
        const TriageRecord* rec = heap_.peek();
        if (!rec) {
            outPatient = nullptr;
            return false;
        }
        outPatient = registry_.findPatientByHandle(rec->internalHandle);
        return (outPatient != nullptr);
    }

    /**
     * @brief Extracts highest-priority emergency patient from triage.
     *        Transitions patient state to DISPATCH_PENDING (Unassigned).
     */
    bool extractHighestPriority(Patient*& outPatient, std::string& outError) {
        if (heap_.empty()) {
            outError = "Emergency triage queue is empty.";
            outPatient = nullptr;
            return false;
        }

        TriageRecord rec;
        if (!heap_.extractMin(rec)) {
            outError = "Failed to extract root from triage min-heap.";
            outPatient = nullptr;
            return false;
        }

        Patient* p = registry_.findPatientByHandle(rec.internalHandle);
        if (!p) {
            outError = "Extracted handle " + std::to_string(rec.internalHandle) + " not found in hospital registry.";
            outPatient = nullptr;
            return false;
        }

        p->state = PatientState::DISPATCH_PENDING;
        p->assignedResourceId = ""; // Unassigned pending dispatch
        outPatient = p;
        return true;
    }

    /**
     * @brief Cancels a waiting patient from triage, removing them from heap in O(log n) time.
     */
    bool cancelWaitingPatient(Patient* patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot cancel null patient.";
            return false;
        }

        if (patient->state != PatientState::TRIAGED_WAITING) {
            outError = "Patient " + patient->patientId + " is not in TRIAGED_WAITING state.";
            return false;
        }

        if (!heap_.removePatient(patient->internalHandle)) {
            outError = "Failed to remove patient handle " + std::to_string(patient->internalHandle) + " from triage heap.";
            return false;
        }

        patient->state = PatientState::CANCELLED;
        return true;
    }

    size_t size() const noexcept { return heap_.size(); }
    bool empty() const noexcept { return heap_.empty(); }
    void clear() noexcept { heap_.clear(); }

    bool isPatientActive(int handle) const noexcept {
        return heap_.isPatientActive(handle);
    }

    const TriageMinHeap& getHeap() const noexcept {
        return heap_;
    }
};
