#pragma once

#include <string>
#include <cstdint>
#include "algorithms/DistanceArithmetic.hpp"

struct CampusLocation {
    int nodeId{0};                  // 0 to 11
    std::string name;               // e.g. "ER Triage & Main Waiting Hall"
    std::string buildingWing;       // e.g. "Emergency Wing, Ground Floor"
    bool isTreatmentUnit{false};    // true if medical care is delivered here
};

struct CorridorEdge {
    int u{0};                       // Source node ID
    int v{0};                       // Destination node ID
    int64_t transitTimeSeconds{0};  // Transit cost in seconds (non-negative, validated)
    bool isBlocked{false};          // true if corridor is closed/obstructed
};
