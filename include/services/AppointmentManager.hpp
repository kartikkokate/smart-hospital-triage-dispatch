#pragma once

#include <string>
#include <vector>
#include "models/Patient.hpp"
#include "ds/CustomQueue.hpp"
#include "services/HospitalRegistry.hpp"
#include "services/TriageManager.hpp"

/**
 * @brief Outpatient Appointment Manager.
 * 
 * DESIGN RATIONALE:
 * - Manages outpatient clinic consultations scheduled in predictable FIFO order.
 * - Underpinned by CustomQueue<std::string> storing alphanumeric patient IDs.
 * - Supports cancellation through an encapsulated queue operation without exposing private nodes.
 * - Supports emergency conversion if a waiting outpatient clinic patient's condition deteriorates.
 */
class AppointmentManager {
private:
    CustomQueue<std::string> queue_;
    HospitalRegistry& registry_;

public:
    explicit AppointmentManager(HospitalRegistry& registry) noexcept
        : registry_(registry) {}

    /**
     * @brief Books an outpatient appointment for a registered patient.
     */
    bool scheduleAppointment(Patient* patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot schedule appointment for null patient.";
            return false;
        }

        if (patient->state != PatientState::REGISTERED) {
            outError = "Patient " + patient->patientId + " is in state " + 
                       patientStateToString(patient->state) + "; must be REGISTERED to schedule appointment.";
            return false;
        }

        if (queue_.contains(patient->patientId)) {
            outError = "Patient " + patient->patientId + " is already in the appointment queue.";
            return false;
        }

        queue_.enqueue(patient->patientId);
        patient->state = PatientState::APPOINTMENT_SCHEDULED;
        return true;
    }

    /**
     * @brief Cancels an outpatient appointment via encapsulated queue removal.
     */
    bool cancelAppointment(Patient* patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot cancel appointment for null patient.";
            return false;
        }

        if (patient->state != PatientState::APPOINTMENT_SCHEDULED) {
            outError = "Patient " + patient->patientId + " is not in APPOINTMENT_SCHEDULED state.";
            return false;
        }

        if (!queue_.remove(patient->patientId)) {
            outError = "Patient " + patient->patientId + " was not found in appointment queue.";
            return false;
        }

        patient->state = PatientState::CANCELLED;
        return true;
    }

    /**
     * @brief Calls next appointment in FIFO order for clinical consultation.
     *        Transitions patient to UNDER_TREATMENT.
     */
    bool processNextAppointment(Patient*& outPatient, std::string& outError) {
        if (queue_.isEmpty()) {
            outError = "Appointment queue is empty.";
            outPatient = nullptr;
            return false;
        }

        std::string patientId;
        if (!queue_.dequeue(patientId)) {
            outError = "Failed to dequeue appointment from queue.";
            outPatient = nullptr;
            return false;
        }

        Patient* p = registry_.findPatient(patientId);
        if (!p) {
            outError = "Patient ID '" + patientId + "' in appointment queue not found in registry.";
            outPatient = nullptr;
            return false;
        }

        p->state = PatientState::UNDER_TREATMENT;
        outPatient = p;
        return true;
    }

    /**
     * @brief Moves an outpatient appointment patient to emergency triage if condition deteriorates.
     *        Pre-validates triage admission criteria before touching queue, and on any downstream
     *        intake failure restores the patient to the exact original FIFO position in the queue.
     */
    bool convertToEmergency(Patient* patient, UrgencyLevel newUrgency, TriageManager& triage, std::string& outError) {
        if (!patient) {
            outError = "Cannot convert null patient to emergency triage.";
            return false;
        }

        if (patient->state != PatientState::APPOINTMENT_SCHEDULED) {
            outError = "Patient " + patient->patientId + " is not in APPOINTMENT_SCHEDULED state (current: " +
                       patientStateToString(patient->state) + ").";
            return false;
        }

        int urgVal = urgencyToInt(newUrgency);
        if (urgVal < 1 || urgVal > 5) {
            outError = "Invalid target urgency " + std::to_string(urgVal) + " (must be 1 to 5).";
            return false;
        }

        // Detach node from appointment queue without releasing its memory, capturing exact FIFO index
        int origIdx = -1;
        auto* detachedNode = queue_.detachNode(patient->patientId, origIdx);
        if (!detachedNode || origIdx < 0) {
            outError = "Patient " + patient->patientId + " was not found in appointment queue.";
            return false;
        }

        // Update urgency and transition to REGISTERED for triage intake
        UrgencyLevel oldUrgency = patient->urgency;
        PatientState oldState = patient->state;

        patient->urgency = newUrgency;
        patient->state = PatientState::REGISTERED;

        try {
            if (!triage.enqueuePatient(patient, outError)) {
                // Strictly rollback patient fields and reinsert original node at exact index without allocation
                patient->urgency = oldUrgency;
                patient->state = oldState;
                queue_.reinsertNodeAt(static_cast<size_t>(origIdx), detachedNode);
                return false;
            }
        } catch (...) {
            // Strictly rollback patient fields and reinsert original node on exception without allocation
            patient->urgency = oldUrgency;
            patient->state = oldState;
            queue_.reinsertNodeAt(static_cast<size_t>(origIdx), detachedNode);
            throw;
        }

        // Triage intake succeeded: now safely free the detached appointment node
        CustomQueue<std::string>::freeNode(detachedNode);
        return true;
    }

    /**
     * @brief Returns snapshot of all currently scheduled patient IDs in FIFO order.
     */
    std::vector<std::string> getScheduledPatientIds() const {
        std::vector<std::string> result;
        queue_.forEach([&result](const std::string& id) {
            result.push_back(id);
        });
        return result;
    }

    bool contains(const std::string& patientId) const noexcept {
        return queue_.contains(patientId);
    }

    bool peekNext(Patient*& outPatient) const noexcept {
        if (queue_.isEmpty()) {
            outPatient = nullptr;
            return false;
        }
        const std::string* pid = queue_.front();
        if (!pid) {
            outPatient = nullptr;
            return false;
        }
        outPatient = registry_.findPatient(*pid);
        return (outPatient != nullptr);
    }

    size_t size() const noexcept { return queue_.size(); }
    bool empty() const noexcept { return queue_.isEmpty(); }
    void clear() noexcept { queue_.clear(); }
};
