#include "services/DataLoader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cerrno>
#include <cmath>

namespace {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    // Strictly check for unsupported double quotes (CSV string quoting).
    // Note: Ordinary apostrophes in names such as "O'Connor" are explicitly allowed.
    bool checkNoQuotes(const std::string& token, int lineNum, const std::string& fieldName, std::string& outErr) {
        if (token.find('"') != std::string::npos) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Double-quoted fields are not supported ('" + token + "'). Standard unquoted CSV required.";
            return false;
        }
        return true;
    }

    bool parseStrictInt(const std::string& str, int& outVal, int minVal, int maxVal, 
                        const std::string& fieldName, int lineNum, std::string& outErr) {
        if (str.empty()) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Field cannot be empty.";
            return false;
        }
        char* endPtr = nullptr;
        errno = 0;
        long val = std::strtol(str.c_str(), &endPtr, 10);
        if (errno != 0 || endPtr == str.c_str() || *endPtr != '\0') {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Invalid integer token '" + str + "'.";
            return false;
        }
        if (val < minVal || val > maxVal) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Value " + std::to_string(val) + 
                     " out of valid range [" + std::to_string(minVal) + ", " + std::to_string(maxVal) + "].";
            return false;
        }
        outVal = static_cast<int>(val);
        return true;
    }

    bool parseStrictUint64(const std::string& str, uint64_t& outVal, uint64_t minVal, uint64_t maxVal, 
                           const std::string& fieldName, int lineNum, std::string& outErr) {
        if (str.empty() || str[0] == '-') {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Non-negative integer required (found '" + str + "').";
            return false;
        }
        char* endPtr = nullptr;
        errno = 0;
        unsigned long long val = std::strtoull(str.c_str(), &endPtr, 10);
        if (errno != 0 || endPtr == str.c_str() || *endPtr != '\0') {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Invalid unsigned integer token '" + str + "'.";
            return false;
        }
        if (val < minVal || val > maxVal) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Value out of valid range.";
            return false;
        }
        outVal = static_cast<uint64_t>(val);
        return true;
    }

    bool parseStrictInt64(const std::string& str, int64_t& outVal, int64_t minVal, int64_t maxVal, 
                          const std::string& fieldName, int lineNum, std::string& outErr) {
        if (str.empty()) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Field cannot be empty.";
            return false;
        }
        char* endPtr = nullptr;
        errno = 0;
        long long val = std::strtoll(str.c_str(), &endPtr, 10);
        if (errno != 0 || endPtr == str.c_str() || *endPtr != '\0') {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Invalid 64-bit integer token '" + str + "'.";
            return false;
        }
        if (val < minVal || val > maxVal) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Value out of valid range [" + 
                     std::to_string(minVal) + ", " + std::to_string(maxVal) + "].";
            return false;
        }
        outVal = static_cast<int64_t>(val);
        return true;
    }

    bool parseStrictDouble(const std::string& str, double& outVal, double minVal, double maxVal, 
                           const std::string& fieldName, int lineNum, std::string& outErr) {
        if (str.empty()) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Field cannot be empty.";
            return false;
        }
        char* endPtr = nullptr;
        errno = 0;
        double val = std::strtod(str.c_str(), &endPtr);
        if (errno != 0 || endPtr == str.c_str() || *endPtr != '\0' || std::isnan(val) || std::isinf(val)) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Invalid finite floating-point token '" + str + "'.";
            return false;
        }
        if (val < minVal || val > maxVal) {
            outErr = "Line " + std::to_string(lineNum) + " [" + fieldName + "]: Value out of range.";
            return false;
        }
        outVal = val;
        return true;
    }

    bool parsePatientStateStrict(const std::string& str, PatientState& outState, int lineNum, std::string& outErr) {
        if (str == "REGISTERED") { outState = PatientState::REGISTERED; return true; }
        if (str == "TRIAGED_WAITING") { outState = PatientState::TRIAGED_WAITING; return true; }
        if (str == "APPOINTMENT_SCHEDULED") { outState = PatientState::APPOINTMENT_SCHEDULED; return true; }
        if (str == "DISPATCH_PENDING") { outState = PatientState::DISPATCH_PENDING; return true; }
        if (str == "IN_TRANSIT") { outState = PatientState::IN_TRANSIT; return true; }
        if (str == "UNDER_TREATMENT") { outState = PatientState::UNDER_TREATMENT; return true; }
        if (str == "DISCHARGED") { outState = PatientState::DISCHARGED; return true; }
        if (str == "CANCELLED") { outState = PatientState::CANCELLED; return true; }
        outErr = "Line " + std::to_string(lineNum) + " [initialState]: Unknown state '" + str + "'.";
        return false;
    }

    bool parseResourceStateStrict(const std::string& str, ResourceState& outState, int lineNum, std::string& outErr) {
        if (str == "AVAILABLE") { outState = ResourceState::AVAILABLE; return true; }
        if (str == "EN_ROUTE_PICKUP") { outState = ResourceState::EN_ROUTE_PICKUP; return true; }
        if (str == "TRANSPORTING_PATIENT") { outState = ResourceState::TRANSPORTING_PATIENT; return true; }
        outErr = "Line " + std::to_string(lineNum) + " [state]: Unknown resource state '" + str + "'.";
        return false;
    }
}

bool DataLoader::loadPatientsCSV(const std::string& filepath, std::vector<Patient>& outPatients, std::string& outError) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        outError = "Could not open patient seed file at: " + filepath;
        return false;
    }

    std::vector<Patient> tempPatients;
    std::vector<std::string> seenPatientIds;
    std::vector<uint64_t> seenArrivalSequences;

    std::string line;
    int lineNum = 0;

    if (!std::getline(file, line)) {
        outError = "Empty patient seed file at: " + filepath;
        return false;
    }
    lineNum++;

    int handleCounter = 0;
    while (std::getline(file, line)) {
        lineNum++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ',')) {
            fields.push_back(trim(token));
        }

        if (fields.size() != 11) {
            outError = "Line " + std::to_string(lineNum) + ": Expected exactly 11 fields, found " + 
                       std::to_string(fields.size()) + " in line: " + line;
            return false;
        }

        for (size_t i = 0; i < fields.size(); ++i) {
            if (!checkNoQuotes(fields[i], lineNum, "field " + std::to_string(i), outError)) {
                return false;
            }
        }

        Patient p;
        p.internalHandle = handleCounter++;

        // 1. Patient ID
        p.patientId = fields[0];
        if (p.patientId.empty() || p.patientId.size() > kMaxPatientIdLength) {
            outError = "Line " + std::to_string(lineNum) + " [patientId]: ID length must be between 1 and " +
                       std::to_string(kMaxPatientIdLength) + " characters.";
            return false;
        }
        if (std::find(seenPatientIds.begin(), seenPatientIds.end(), p.patientId) != seenPatientIds.end()) {
            outError = "Line " + std::to_string(lineNum) + " [patientId]: Duplicate patient ID '" + p.patientId + "' detected.";
            return false;
        }
        seenPatientIds.push_back(p.patientId);

        // 2. Full Name
        p.fullName = fields[1];
        if (p.fullName.empty() || p.fullName.size() > kMaxPatientNameLength) {
            outError = "Line " + std::to_string(lineNum) + " [fullName]: Name cannot be empty or exceed " +
                       std::to_string(kMaxPatientNameLength) + " characters.";
            return false;
        }

        // 3. Age
        if (!parseStrictInt(fields[2], p.age, 0, 150, "age", lineNum, outError)) return false;

        // 4. Complaint
        p.complaint = fields[3];
        if (p.complaint.empty() || p.complaint.size() > kMaxComplaintLength) {
            outError = "Line " + std::to_string(lineNum) + " [complaint]: Complaint cannot be empty or exceed " +
                       std::to_string(kMaxComplaintLength) + " characters.";
            return false;
        }

        // 5. Urgency
        int urgVal = 0;
        if (!parseStrictInt(fields[4], urgVal, 1, 5, "urgency", lineNum, outError)) return false;
        p.urgency = intToUrgency(urgVal);

        // 6. Arrival Sequence
        if (!parseStrictUint64(fields[5], p.arrivalSequence, 1, 1000000, "arrivalSequence", lineNum, outError)) return false;
        if (std::find(seenArrivalSequences.begin(), seenArrivalSequences.end(), p.arrivalSequence) != seenArrivalSequences.end()) {
            outError = "Line " + std::to_string(lineNum) + " [arrivalSequence]: Duplicate arrival sequence " + 
                       std::to_string(p.arrivalSequence) + " detected.";
            return false;
        }
        seenArrivalSequences.push_back(p.arrivalSequence);

        // 7. Simulated Arrival Time (in seconds)
        if (!parseStrictInt64(fields[6], p.simulatedArrivalTime, 0, 1000000000LL, "simulatedArrivalTime", lineNum, outError)) return false;

        // 8. Current Location Node (0 to 11)
        if (!parseStrictInt(fields[7], p.currentLocationNode, 0, 11, "currentLocationNode", lineNum, outError)) return false;

        // 9. Target Location Node (0 to 11)
        if (!parseStrictInt(fields[8], p.targetLocationNode, 0, 11, "targetLocationNode", lineNum, outError)) return false;

        // 10. Billing Amount
        if (!parseStrictDouble(fields[9], p.billingAmount, 0.0, 1000000.0, "billingAmount", lineNum, outError)) return false;

        // 11. Initial State
        if (!parsePatientStateStrict(fields[10], p.state, lineNum, outError)) return false;
        if (p.state == PatientState::IN_TRANSIT || 
            p.state == PatientState::DISPATCH_PENDING || 
            p.state == PatientState::UNDER_TREATMENT) {
            outError = "Line " + std::to_string(lineNum) + " [initialState]: State '" + fields[10] + 
                       "' requires active assignment/clinical context that the seed format cannot reconstruct.";
            return false;
        }

        tempPatients.push_back(p);
    }

    if (tempPatients.empty()) {
        outError = "Patient seed file contains no data rows: " + filepath;
        return false;
    }

    outPatients = std::move(tempPatients);
    return true;
}

bool DataLoader::loadResourcesCSV(const std::string& filepath, std::vector<DispatchResource>& outResources, std::string& outError) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        outError = "Could not open resources seed file at: " + filepath;
        return false;
    }

    std::vector<DispatchResource> tempResources;
    std::vector<std::string> seenResourceIds;

    std::string line;
    int lineNum = 0;
    if (!std::getline(file, line)) {
        outError = "Empty resources seed file at: " + filepath;
        return false;
    }
    lineNum++;

    while (std::getline(file, line)) {
        lineNum++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ',')) {
            fields.push_back(trim(token));
        }

        if (fields.size() < 4 || fields.size() > 5) {
            outError = "Line " + std::to_string(lineNum) + ": Expected 4 or 5 fields, found " + 
                       std::to_string(fields.size()) + " in line: " + line;
            return false;
        }

        for (size_t i = 0; i < fields.size(); ++i) {
            if (!checkNoQuotes(fields[i], lineNum, "field " + std::to_string(i), outError)) {
                return false;
            }
        }

        DispatchResource res;
        res.resourceId = fields[0];
        if (res.resourceId.empty() || res.resourceId.size() > 16) {
            outError = "Line " + std::to_string(lineNum) + " [resourceId]: Length must be between 1 and 16 characters.";
            return false;
        }
        if (std::find(seenResourceIds.begin(), seenResourceIds.end(), res.resourceId) != seenResourceIds.end()) {
            outError = "Line " + std::to_string(lineNum) + " [resourceId]: Duplicate resource ID '" + res.resourceId + "'.";
            return false;
        }
        seenResourceIds.push_back(res.resourceId);

        res.name = fields[1];
        if (res.name.empty() || res.name.size() > 64) {
            outError = "Line " + std::to_string(lineNum) + " [name]: Name cannot be empty or exceed 64 characters.";
            return false;
        }

        if (!parseResourceStateStrict(fields[2], res.state, lineNum, outError)) return false;
        if (res.state != ResourceState::AVAILABLE) {
            outError = "Line " + std::to_string(lineNum) + " [state]: Initial seed resource " + res.resourceId + 
                       " must be in AVAILABLE state (found '" + fields[2] + "').";
            return false;
        }

        if (!parseStrictInt(fields[3], res.currentNodeId, 0, 11, "currentNodeId", lineNum, outError)) return false;

        if (fields.size() >= 5) {
            res.assignedPatientId = fields[4];
        }

        tempResources.push_back(res);
    }

    if (tempResources.empty()) {
        outError = "Resources seed file contains no data rows: " + filepath;
        return false;
    }

    outResources = std::move(tempResources);
    return true;
}

bool DataLoader::loadCampusMapTXT(const std::string& filepath, 
                                 std::vector<CampusLocation>& outLocations, 
                                 std::vector<CorridorEdge>& outEdges, 
                                 std::string& outError) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        outError = "Could not open campus map seed file at: " + filepath;
        return false;
    }

    std::vector<CampusLocation> tempLocations;
    std::vector<CorridorEdge> tempEdges;
    std::vector<int> seenNodeIds;
    std::vector<std::pair<int, int>> seenCorridors;

    std::string line;
    std::string currentSection = "";
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[NODES]") {
            currentSection = "NODES";
            continue;
        } else if (line == "[EDGES]") {
            currentSection = "EDGES";
            continue;
        } else if (line.front() == '[' && line.back() == ']') {
            outError = "Line " + std::to_string(lineNum) + ": Unknown map section header '" + line + "'. Only [NODES] and [EDGES] are supported.";
            return false;
        }

        if (currentSection.empty()) {
            outError = "Line " + std::to_string(lineNum) + ": Data row found before any section header.";
            return false;
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ',')) {
            fields.push_back(trim(token));
        }

        for (size_t i = 0; i < fields.size(); ++i) {
            if (!checkNoQuotes(fields[i], lineNum, "field " + std::to_string(i), outError)) {
                return false;
            }
        }

        if (currentSection == "NODES") {
            if (fields.size() != 4) {
                outError = "Line " + std::to_string(lineNum) + " [NODES]: Expected exactly 4 fields (NodeId,Name,BuildingWing,IsTreatmentUnit), found " + 
                           std::to_string(fields.size());
                return false;
            }

            CampusLocation loc;
            if (!parseStrictInt(fields[0], loc.nodeId, 0, 11, "nodeId", lineNum, outError)) return false;
            if (std::find(seenNodeIds.begin(), seenNodeIds.end(), loc.nodeId) != seenNodeIds.end()) {
                outError = "Line " + std::to_string(lineNum) + " [nodeId]: Duplicate campus location Node ID " + std::to_string(loc.nodeId);
                return false;
            }
            seenNodeIds.push_back(loc.nodeId);

            loc.name = fields[1];
            if (loc.name.empty() || loc.name.size() > 64) {
                outError = "Line " + std::to_string(lineNum) + " [name]: Node name cannot be empty or exceed 64 characters.";
                return false;
            }

            loc.buildingWing = fields[2];
            if (loc.buildingWing.empty() || loc.buildingWing.size() > 64) {
                outError = "Line " + std::to_string(lineNum) + " [buildingWing]: Building wing cannot be empty.";
                return false;
            }

            if (fields[3] == "true" || fields[3] == "1") {
                loc.isTreatmentUnit = true;
            } else if (fields[3] == "false" || fields[3] == "0") {
                loc.isTreatmentUnit = false;
            } else {
                outError = "Line " + std::to_string(lineNum) + " [isTreatmentUnit]: Expected 'true' or 'false', found '" + fields[3] + "'.";
                return false;
            }

            tempLocations.push_back(loc);

        } else if (currentSection == "EDGES") {
            if (fields.size() < 3 || fields.size() > 4) {
                outError = "Line " + std::to_string(lineNum) + " [EDGES]: Expected 3 or 4 fields (NodeU,NodeV,TransitSeconds[,Description]), found " + 
                           std::to_string(fields.size());
                return false;
            }

            CorridorEdge edge;
            if (!parseStrictInt(fields[0], edge.u, 0, 11, "u", lineNum, outError)) return false;
            if (!parseStrictInt(fields[1], edge.v, 0, 11, "v", lineNum, outError)) return false;

            if (edge.u == edge.v) {
                outError = "Line " + std::to_string(lineNum) + " [EDGES]: Self-loop corridors (u == v == " + 
                           std::to_string(edge.u) + ") are not permitted.";
                return false;
            }

            if (std::find(seenNodeIds.begin(), seenNodeIds.end(), edge.u) == seenNodeIds.end()) {
                outError = "Line " + std::to_string(lineNum) + " [EDGES]: Node U " + std::to_string(edge.u) + " is not a declared campus location.";
                return false;
            }
            if (std::find(seenNodeIds.begin(), seenNodeIds.end(), edge.v) == seenNodeIds.end()) {
                outError = "Line " + std::to_string(lineNum) + " [EDGES]: Node V " + std::to_string(edge.v) + " is not a declared campus location.";
                return false;
            }

            int minNode = std::min(edge.u, edge.v);
            int maxNode = std::max(edge.u, edge.v);
            auto corridorPair = std::make_pair(minNode, maxNode);
            if (std::find(seenCorridors.begin(), seenCorridors.end(), corridorPair) != seenCorridors.end()) {
                outError = "Line " + std::to_string(lineNum) + " [EDGES]: Duplicate corridor between nodes " + 
                           std::to_string(minNode) + " and " + std::to_string(maxNode) + " detected.";
                return false;
            }
            seenCorridors.push_back(corridorPair);

            if (!parseStrictInt64(fields[2], edge.transitTimeSeconds, 0, kMaxCorridorWeight, "transitTimeSeconds", lineNum, outError)) {
                return false;
            }
            edge.isBlocked = false;

            tempEdges.push_back(edge);
        }
    }

    if (tempLocations.empty()) {
        outError = "Campus map file contains no locations: " + filepath;
        return false;
    }
    if (tempEdges.empty()) {
        outError = "Campus map file contains no corridors: " + filepath;
        return false;
    }

    outLocations = std::move(tempLocations);
    outEdges = std::move(tempEdges);
    return true;
}

bool DataLoader::validateCampusReferences(const std::vector<Patient>& patients,
                                         const std::vector<DispatchResource>& resources,
                                         const std::vector<CampusLocation>& locations,
                                         std::string& outError) {
    auto isValidLocation = [&](int nodeId) -> bool {
        for (const auto& loc : locations) {
            if (loc.nodeId == nodeId) return true;
        }
        return false;
    };

    for (const auto& p : patients) {
        if (!isValidLocation(p.currentLocationNode)) {
            outError = "Patient " + p.patientId + " references non-existent currentLocationNode " + std::to_string(p.currentLocationNode);
            return false;
        }
        if (!isValidLocation(p.targetLocationNode)) {
            outError = "Patient " + p.patientId + " references non-existent targetLocationNode " + std::to_string(p.targetLocationNode);
            return false;
        }
    }

    for (const auto& r : resources) {
        if (!isValidLocation(r.currentNodeId)) {
            outError = "Resource " + r.resourceId + " references non-existent currentNodeId " + std::to_string(r.currentNodeId);
            return false;
        }
    }

    return true;
}
