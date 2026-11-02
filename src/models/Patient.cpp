#include "models/Patient.hpp"

std::string urgencyToString(UrgencyLevel level) {
    switch (level) {
        case UrgencyLevel::RESUSCITATION: return "1 - Resuscitation";
        case UrgencyLevel::EMERGENT:      return "2 - Emergent";
        case UrgencyLevel::URGENT:        return "3 - Urgent";
        case UrgencyLevel::LESS_URGENT:   return "4 - Less Urgent";
        case UrgencyLevel::NON_URGENT:    return "5 - Non-Urgent";
        default:                          return "Unknown";
    }
}

std::string patientStateToString(PatientState state) {
    switch (state) {
        case PatientState::REGISTERED:            return "REGISTERED";
        case PatientState::TRIAGED_WAITING:       return "TRIAGED_WAITING";
        case PatientState::APPOINTMENT_SCHEDULED: return "APPOINTMENT_SCHEDULED";
        case PatientState::DISPATCH_PENDING:      return "DISPATCH_PENDING";
        case PatientState::IN_TRANSIT:            return "IN_TRANSIT";
        case PatientState::UNDER_TREATMENT:       return "UNDER_TREATMENT";
        case PatientState::DISCHARGED:            return "DISCHARGED";
        case PatientState::CANCELLED:             return "CANCELLED";
        default:                                  return "UNKNOWN";
    }
}

UrgencyLevel intToUrgency(int level) {
    switch (level) {
        case 1: return UrgencyLevel::RESUSCITATION;
        case 2: return UrgencyLevel::EMERGENT;
        case 3: return UrgencyLevel::URGENT;
        case 4: return UrgencyLevel::LESS_URGENT;
        case 5: return UrgencyLevel::NON_URGENT;
        default: return UrgencyLevel::NON_URGENT;
    }
}
