#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "services/CampusMapManager.hpp"

/**
 * @brief Dispatch and Transport Routing Manager.
 * 
 * DESIGN RATIONALE:
 * - Manages mobile hospital transport resources (e.g. gurneys, mobile triage units).
 * - Implements deterministic two-leg routing:
 *     Leg 1: Resource current station -> Patient pickup location.
 *     Leg 2: Patient pickup location -> Target clinical unit.
 * - Resource selection policy:
 *     1. Validates reachable destination route (Leg 2) prior to assignment.
 *     2. Minimum reachable pickup cost (Leg 1).
 *     3. Deterministic tie-breaker: Lexicographically lowest resourceId.
 * - Simulation boundaries:
 *     - Pickup arrival: patient becomes IN_TRANSIT; resource becomes TRANSPORTING_PATIENT.
 *     - Handover arrival: patient becomes UNDER_TREATMENT; resource released to AVAILABLE
 *       at destination node; both references cleared.
 *     - Corridor closure handling:
 *         - During Leg 1: Pickup aborted, resource released, patient reverted to unassigned pending.
 *         - During Leg 2: Transfer paused at intermediate/current node, assignment retained for retry.
 */
class DispatchManager {
private:
    std::vector<DispatchResource> resources_;
    CampusMapManager& map_;

public:
    explicit DispatchManager(CampusMapManager& map) noexcept
        : map_(map) {}

    /**
     * @brief Registers a dispatch resource into the fleet.
     */
    bool registerResource(const DispatchResource& res, std::string& outError) {
        if (res.resourceId.empty()) {
            outError = "Resource ID cannot be empty.";
            return false;
        }

        if (!map_.isValidNode(res.currentNodeId)) {
            outError = "Resource node ID " + std::to_string(res.currentNodeId) + " is not a valid campus node.";
            return false;
        }

        for (const auto& r : resources_) {
            if (r.resourceId == res.resourceId) {
                outError = "Duplicate resource ID '" + res.resourceId + "'.";
                return false;
            }
        }

        resources_.push_back(res);
        return true;
    }

    DispatchResource* findResource(const std::string& resourceId) noexcept {
        for (auto& r : resources_) {
            if (r.resourceId == resourceId) return &r;
        }
        return nullptr;
    }

    const DispatchResource* findResource(const std::string& resourceId) const noexcept {
        for (const auto& r : resources_) {
            if (r.resourceId == resourceId) return &r;
        }
        return nullptr;
    }

    const std::vector<DispatchResource>& getAllResources() const noexcept {
        return resources_;
    }

    std::vector<DispatchResource>& getAllResources() noexcept {
        return resources_;
    }

    void clear() noexcept {
        resources_.clear();
    }

    /**
     * @brief Assigns the optimal available resource to an unassigned pending patient.
     * 
     * Validates:
     * 1. Patient is in DISPATCH_PENDING state.
     * 2. Patient is not already assigned (duplicate assignment prevention).
     * 3. Leg 2 (pickup -> destination) is reachable.
     * 4. At least one available resource can reach the pickup location (Leg 1).
     */
    bool assignResource(Patient* patient, DispatchResource*& outResource,
                        DijkstraResult& outLeg1Route, DijkstraResult& outLeg2Route,
                        std::string& outError) {
        if (!patient) {
            outError = "Cannot assign resource to null patient.";
            return false;
        }

        if (patient->state != PatientState::DISPATCH_PENDING) {
            outError = "Patient " + patient->patientId + " is in state " + 
                       patientStateToString(patient->state) + "; must be DISPATCH_PENDING.";
            return false;
        }

        if (!patient->assignedResourceId.empty()) {
            outError = "Patient " + patient->patientId + " is already assigned to resource " + 
                       patient->assignedResourceId + ". Duplicate assignment rejected.";
            return false;
        }

        // Validate Leg 2 route first
        DijkstraResult leg2 = map_.findShortestRoute(patient->currentLocationNode, patient->targetLocationNode);
        if (!leg2.isReachable) {
            outError = "Target clinical unit (node " + std::to_string(patient->targetLocationNode) + 
                       ") is unreachable from pickup node " + std::to_string(patient->currentLocationNode) + 
                       " due to corridor closure.";
            return false;
        }

        // Candidate evaluation: lowest Leg 1 cost, tie-broken by resourceId
        DispatchResource* bestRes = nullptr;
        DijkstraResult bestLeg1;
        bestLeg1.totalCost = kInfinity;

        for (auto& res : resources_) {
            if (!res.isAvailable()) continue;

            DijkstraResult leg1 = map_.findShortestRoute(res.currentNodeId, patient->currentLocationNode);
            if (!leg1.isReachable) continue;

            if (bestRes == nullptr) {
                bestRes = &res;
                bestLeg1 = leg1;
            } else if (leg1.totalCost < bestLeg1.totalCost) {
                bestRes = &res;
                bestLeg1 = leg1;
            } else if (leg1.totalCost == bestLeg1.totalCost) {
                if (res.resourceId < bestRes->resourceId) {
                    bestRes = &res;
                    bestLeg1 = leg1;
                }
            }
        }

        if (!bestRes) {
            outError = "No available transport resource can reach pickup location (node " + 
                       std::to_string(patient->currentLocationNode) + ").";
            return false;
        }

        // Commit assignment
        bestRes->state = ResourceState::EN_ROUTE_PICKUP;
        bestRes->assignedPatientId = patient->patientId;
        patient->assignedResourceId = bestRes->resourceId;

        outResource = bestRes;
        outLeg1Route = bestLeg1;
        outLeg2Route = leg2;
        return true;
    }

    /**
     * @brief Executes Leg 1: Resource travels to patient pickup point.
     *        On arrival: patient -> IN_TRANSIT, resource -> TRANSPORTING_PATIENT.
     *        Verifies reciprocal patient/resource assignment and expected resource state.
     */
    bool stepPickup(Patient* patient, int64_t& outElapsedSeconds, std::string& outError) {
        if (!patient) {
            outError = "Cannot execute pickup on null patient.";
            return false;
        }

        if (patient->state != PatientState::DISPATCH_PENDING) {
            outError = "Patient " + patient->patientId + " is in state " + 
                       patientStateToString(patient->state) + " (expected DISPATCH_PENDING).";
            return false;
        }

        if (patient->assignedResourceId.empty()) {
            outError = "Patient " + patient->patientId + " has no assigned resource.";
            return false;
        }

        DispatchResource* res = findResource(patient->assignedResourceId);
        if (!res) {
            outError = "Assigned resource '" + patient->assignedResourceId + "' not found in fleet.";
            return false;
        }

        // Verify reciprocal assignment
        if (res->assignedPatientId != patient->patientId) {
            outError = "Assignment mismatch: resource " + res->resourceId + " is assigned to patient '" + 
                       res->assignedPatientId + "', not '" + patient->patientId + "'.";
            return false;
        }

        // Verify expected resource state
        if (res->state != ResourceState::EN_ROUTE_PICKUP) {
            outError = "Resource state mismatch: resource " + res->resourceId + " is in state " + 
                       resourceStateToString(res->state) + " (expected EN_ROUTE_PICKUP).";
            return false;
        }

        // Revalidate remaining pickup route prior to movement
        DijkstraResult route = map_.findShortestRoute(res->currentNodeId, patient->currentLocationNode);
        if (!route.isReachable) {
            // Corridor closure during pickup: Abort pickup, release resource, revert patient to unassigned
            res->state = ResourceState::AVAILABLE;
            res->assignedPatientId = "";
            patient->assignedResourceId = "";
            outError = "Corridor closure during pickup leg. Resource released; patient reverted to unassigned pending.";
            return false;
        }

        outElapsedSeconds = static_cast<int64_t>(route.totalCost);
        res->currentNodeId = patient->currentLocationNode;
        res->state = ResourceState::TRANSPORTING_PATIENT;
        patient->state = PatientState::IN_TRANSIT;
        return true;
    }

    /**
     * @brief Executes Leg 2: Transport patient to destination and complete handover.
     *        On arrival: patient -> UNDER_TREATMENT, resource -> AVAILABLE at destination.
     *        Verifies reciprocal patient/resource assignment and expected resource state.
     */
    bool stepTransportAndHandover(Patient* patient, int64_t& outElapsedSeconds, std::string& outError) {
        if (!patient) {
            outError = "Cannot execute transport on null patient.";
            return false;
        }

        if (patient->state != PatientState::IN_TRANSIT) {
            outError = "Patient " + patient->patientId + " is in state " + 
                       patientStateToString(patient->state) + " (expected IN_TRANSIT).";
            return false;
        }

        if (patient->assignedResourceId.empty()) {
            outError = "Patient " + patient->patientId + " has no assigned resource.";
            return false;
        }

        DispatchResource* res = findResource(patient->assignedResourceId);
        if (!res) {
            outError = "Assigned resource '" + patient->assignedResourceId + "' not found in fleet.";
            return false;
        }

        // Verify reciprocal assignment
        if (res->assignedPatientId != patient->patientId) {
            outError = "Assignment mismatch: resource " + res->resourceId + " is assigned to patient '" + 
                       res->assignedPatientId + "', not '" + patient->patientId + "'.";
            return false;
        }

        // Verify expected resource state
        if (res->state != ResourceState::TRANSPORTING_PATIENT) {
            outError = "Resource state mismatch: resource " + res->resourceId + " is in state " + 
                       resourceStateToString(res->state) + " (expected TRANSPORTING_PATIENT).";
            return false;
        }

        // Revalidate remaining route to destination clinical unit
        DijkstraResult route = map_.findShortestRoute(patient->currentLocationNode, patient->targetLocationNode);
        if (!route.isReachable) {
            // Corridor closure during transport leg: Pause and retain assignment for retry
            outError = "Corridor closure during transport leg. Transport paused at node " + 
                       std::to_string(patient->currentLocationNode) + "; assignment retained for retry.";
            return false;
        }

        outElapsedSeconds = static_cast<int64_t>(route.totalCost);

        // Handover execution & immediate resource release
        patient->state = PatientState::UNDER_TREATMENT;
        patient->currentLocationNode = patient->targetLocationNode;
        patient->assignedResourceId = "";

        res->currentNodeId = patient->targetLocationNode;
        res->assignedPatientId = "";
        res->state = ResourceState::AVAILABLE;

        return true;
    }

    /**
     * @brief Cancels dispatch during pickup leg, restoring resource to AVAILABLE.
     */
    bool cancelPickup(Patient* patient, std::string& outError) {
        if (!patient) {
            outError = "Cannot cancel null patient.";
            return false;
        }

        if (patient->state == PatientState::DISPATCH_PENDING) {
            if (!patient->assignedResourceId.empty()) {
                DispatchResource* res = findResource(patient->assignedResourceId);
                if (res) {
                    if (res->assignedPatientId == patient->patientId && res->state == ResourceState::EN_ROUTE_PICKUP) {
                        res->state = ResourceState::AVAILABLE;
                        res->assignedPatientId = "";
                    }
                }
                patient->assignedResourceId = "";
            }
            patient->state = PatientState::CANCELLED;
            return true;
        }

        if (patient->state == PatientState::IN_TRANSIT) {
            outError = "Cannot cancel patient while actively IN_TRANSIT.";
            return false;
        }

        outError = "Cannot cancel dispatch for patient in state " + patientStateToString(patient->state) + ".";
        return false;
    }
};
