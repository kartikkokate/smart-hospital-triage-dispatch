#pragma once

#include <cstdint>
#include <limits>

// Safe sentinel for unreachable / infinite distance.
// Note: Sentinel selection alone is NOT an overflow safeguard; all additions
// must be performed using range-checked arithmetic via safeDistanceAdd().
constexpr int64_t kInfinity = std::numeric_limits<int64_t>::max() / 4;

// Upper bound for physical corridor weights (e.g. 1 day in seconds).
constexpr int64_t kMaxCorridorWeight = 86400; // 24 hours in seconds

/**
 * @brief Adds two distances safely with range validation and overflow saturation.
 * 
 * Policy:
 * - Negative inputs (< 0) are strictly rejected as invalid and return -1.
 * - If either operand is >= kInfinity, returns kInfinity (saturates as unreachable).
 * - If currentDist + edgeWeight would exceed kInfinity, saturates at kInfinity.
 * - Otherwise returns exact sum.
 *
 * Guaranteed free of signed integer overflow (no undefined behavior).
 */
inline int64_t safeDistanceAdd(int64_t currentDist, int64_t edgeWeight) {
    if (currentDist < 0 || edgeWeight < 0) {
        return -1; // Invalid negative distance or weight
    }
    if (currentDist >= kInfinity || edgeWeight >= kInfinity) {
        return kInfinity;
    }
    if (currentDist > kInfinity - edgeWeight) {
        return kInfinity; // Overflow saturation at kInfinity
    }
    return currentDist + edgeWeight;
}

/**
 * @brief Checks if relaxing edge (u, v) with weight w improves target distance.
 * 
 * Returns true if and only if:
 * 1. currentDist is non-negative and strictly less than kInfinity.
 * 2. edgeWeight is non-negative and valid.
 * 3. safeDistanceAdd(currentDist, edgeWeight) is strictly less than targetDist.
 */
inline bool canRelaxEdge(int64_t currentDist, int64_t edgeWeight, int64_t targetDist) {
    if (currentDist < 0 || edgeWeight < 0 || currentDist >= kInfinity) {
        return false;
    }
    int64_t candidateDist = safeDistanceAdd(currentDist, edgeWeight);
    if (candidateDist < 0) {
        return false;
    }
    return candidateDist < targetDist;
}
