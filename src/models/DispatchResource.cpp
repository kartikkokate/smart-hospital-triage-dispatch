#include "models/DispatchResource.hpp"

std::string resourceStateToString(ResourceState state) {
    switch (state) {
        case ResourceState::AVAILABLE:            return "AVAILABLE";
        case ResourceState::EN_ROUTE_PICKUP:      return "EN_ROUTE_PICKUP";
        case ResourceState::TRANSPORTING_PATIENT: return "TRANSPORTING_PATIENT";
        default:                                  return "UNKNOWN";
    }
}
