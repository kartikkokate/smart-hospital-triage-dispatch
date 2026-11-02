#pragma once

#include <string>
#include <vector>
#include "models/Patient.hpp"
#include "models/DispatchResource.hpp"
#include "models/CampusLocation.hpp"

class DataLoader {
public:
    static bool loadPatientsCSV(const std::string& filepath, std::vector<Patient>& outPatients, std::string& outError);
    static bool loadResourcesCSV(const std::string& filepath, std::vector<DispatchResource>& outResources, std::string& outError);
    static bool loadCampusMapTXT(const std::string& filepath, 
                                std::vector<CampusLocation>& outLocations, 
                                std::vector<CorridorEdge>& outEdges, 
                                std::string& outError);

    // Cross-validates patient and resource node references against loaded campus locations
    static bool validateCampusReferences(const std::vector<Patient>& patients,
                                         const std::vector<DispatchResource>& resources,
                                         const std::vector<CampusLocation>& locations,
                                         std::string& outError);
};
