#pragma once

#include <string>
#include <cstdint>
#include "models/Patient.hpp"

/**
 * @brief Independent value copy representing a patient at an exact simulation timestamp.
 * 
 * DESIGN RATIONALE:
 * - Independent value copy completely decoupled from internal mutable Patient pointers.
 * - Provides read-only isolation across hospital registry, priority queues, and live dispatch state
 *   without requiring language-enforced immutable object wrappers.
 * - Report generation operates strictly on PatientSnapshot vectors, ensuring read-only safety
 *   across hospital registry, priority queues, and live dispatch state.
 * 
 * WAITING DURATION SPECIFICATION:
 * - Actively Waiting: Elapsed seconds (reportTimestamp - simulatedArrivalTime) is calculated
 *   EXCLUSIVELY for patients who are actively waiting in queues or unassigned dispatch:
 *     * REGISTERED (awaiting clinical intake / triage)
 *     * TRIAGED_WAITING (awaiting bed assignment / transit dispatch)
 *     * APPOINTMENT_SCHEDULED (awaiting outpatient clinic call)
 *     * DISPATCH_PENDING without assigned resource (awaiting fleet assignment)
 * - Historical / Under Treatment: For patients in clinical treatment (UNDER_TREATMENT),
 *   in transit (IN_TRANSIT), or terminal states (DISCHARGED, CANCELLED), current time minus
 *   arrival time does NOT represent waiting time. Since historical treatment-start timestamps
 *   are not recorded in the seed/runtime records, waiting duration is explicitly marked
 *   as unavailable ("N/A - Under Treatment", "N/A - Discharged", etc.).
 */
struct PatientSnapshot {
    std::string patientId;
    std::string fullName;
    int age = 0;
    std::string complaint;
    UrgencyLevel urgency = UrgencyLevel::NON_URGENT;
    uint64_t arrivalSequence = 0;
    int64_t simulatedArrivalTime = 0;
    PatientState state = PatientState::REGISTERED;
    std::string assignedResourceId;
    int currentLocationNode = 1;
    int targetLocationNode = 1;
    double billingAmount = 0.0;

    // Report-level metadata
    int64_t reportTimestamp = 0;
    bool isWaitingDurationApplicable = false;
    int64_t waitingDurationSeconds = -1;
    std::string waitingDurationFormatted;

    PatientSnapshot() = default;

    PatientSnapshot(const Patient& p, int64_t timestamp)
        : patientId(p.patientId),
          fullName(p.fullName),
          age(p.age),
          complaint(p.complaint),
          urgency(p.urgency),
          arrivalSequence(p.arrivalSequence),
          simulatedArrivalTime(p.simulatedArrivalTime),
          state(p.state),
          assignedResourceId(p.assignedResourceId),
          currentLocationNode(p.currentLocationNode),
          targetLocationNode(p.targetLocationNode),
          billingAmount(p.billingAmount),
          reportTimestamp(timestamp) {
        
        // Waiting duration applies exclusively to patients actively waiting
        if (state == PatientState::REGISTERED ||
            state == PatientState::TRIAGED_WAITING ||
            state == PatientState::APPOINTMENT_SCHEDULED ||
            (state == PatientState::DISPATCH_PENDING && assignedResourceId.empty())) {
            
            isWaitingDurationApplicable = true;
            waitingDurationSeconds = (timestamp >= simulatedArrivalTime) ? (timestamp - simulatedArrivalTime) : 0;
            waitingDurationFormatted = std::to_string(waitingDurationSeconds) + "s";
        } else {
            isWaitingDurationApplicable = false;
            waitingDurationSeconds = -1;
            waitingDurationFormatted = "N/A (" + patientStateToString(state) + ")";
        }
    }
};
