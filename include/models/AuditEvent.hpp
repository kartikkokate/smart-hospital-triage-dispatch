#pragma once

#include <string>
#include "models/Patient.hpp"

struct AuditEvent {
    std::string patientId;
    std::string action;                 // e.g. "REGISTERED", "TRIAGED", "DISPATCH_ASSIGNED", "HANDOVER_COMPLETE", "DISCHARGED"
    PatientState priorState{PatientState::REGISTERED};
    PatientState newState{PatientState::REGISTERED};
    UrgencyLevel urgencyAtEvent{UrgencyLevel::NON_URGENT};
    int locationAtEvent{0};
    std::string resourceIdAtEvent;      // Assigned resource ID at event time, if any
    int64_t simulatedTimestamp{0};      // Simulation clock tick in seconds when event was recorded
    std::string details;                // Descriptive message
};
