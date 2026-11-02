#pragma once

#include <string>
#include <cstdint>

enum class UrgencyLevel : int {
    RESUSCITATION = 1, // Highest urgency / Immediate
    EMERGENT      = 2, // High acuity
    URGENT        = 3, // Moderate acuity
    LESS_URGENT   = 4, // Low acuity
    NON_URGENT    = 5  // Routine / Outpatient clinic
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

constexpr size_t kMaxPatientIdLength = 16;
constexpr size_t kMaxPatientNameLength = 64;
constexpr size_t kMaxComplaintLength = 128;

struct Patient {
    int internalHandle{-1};                 // Stable dense integer handle (0 to N-1)
    std::string patientId;                 // e.g. "P101"
    std::string fullName;                  // e.g. "Sarah Connor"
    int age{0};                            // e.g. 29
    std::string complaint;                 // Chief complaint
    UrgencyLevel urgency{UrgencyLevel::NON_URGENT};
    uint64_t arrivalSequence{0};           // Monotonic counter for FIFO tie-breaking
    int64_t simulatedArrivalTime{0};       // Simulation arrival timestamp in seconds
    PatientState state{PatientState::REGISTERED};
    std::string assignedResourceId;        // Empty string if unassigned, resource ID if assigned
    int currentLocationNode{1};            // Campus node ID (default 1: ER Triage)
    int targetLocationNode{1};             // Target clinical treatment unit node ID
    double billingAmount{0.0};             // Synthetic billing amount for sorting reports

    // Helper queries
    bool isDispatchAssigned() const {
        return state == PatientState::DISPATCH_PENDING && !assignedResourceId.empty();
    }

    bool isDispatchUnassigned() const {
        return state == PatientState::DISPATCH_PENDING && assignedResourceId.empty();
    }

    bool isEligibleForTriage() const {
        return state == PatientState::REGISTERED;
    }

    bool isWaitingInTriage() const {
        return state == PatientState::TRIAGED_WAITING;
    }

    bool isUnderTreatment() const {
        return state == PatientState::UNDER_TREATMENT;
    }

    bool isDischarged() const {
        return state == PatientState::DISCHARGED;
    }

    bool isCancelled() const {
        return state == PatientState::CANCELLED;
    }
};

std::string urgencyToString(UrgencyLevel level);
std::string patientStateToString(PatientState state);
UrgencyLevel intToUrgency(int level);
inline int urgencyToInt(UrgencyLevel level) noexcept {
    return static_cast<int>(level);
}
inline bool isValidUrgency(UrgencyLevel level) noexcept {
    int v = static_cast<int>(level);
    return v >= 1 && v <= 5;
}
