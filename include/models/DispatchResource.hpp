#pragma once

#include <string>

enum class ResourceState {
    AVAILABLE,
    EN_ROUTE_PICKUP,
    TRANSPORTING_PATIENT
};

struct DispatchResource {
    std::string resourceId;         // e.g. "GURNEY-01"
    std::string name;               // e.g. "Trauma Transfer Gurney"
    ResourceState state{ResourceState::AVAILABLE};
    int currentNodeId{0};           // Campus node ID (0 to 11)
    std::string assignedPatientId;  // Empty string if idle

    bool isAvailable() const {
        return state == ResourceState::AVAILABLE && assignedPatientId.empty();
    }
};

std::string resourceStateToString(ResourceState state);
