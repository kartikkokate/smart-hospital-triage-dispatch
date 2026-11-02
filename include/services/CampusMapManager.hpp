#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include "models/CampusLocation.hpp"
#include "ds/CampusGraph.hpp"

/**
 * @brief Campus Map and Topology Manager.
 * 
 * DESIGN RATIONALE:
 * - Coordinates spatial and topological operations on the hospital campus.
 * - Wraps CampusGraph (synchronized adjacency matrix & adjacency list models)
 *   and maintains location metadata (CampusLocation).
 * - Provides centralized routing, corridor obstruction (maintenance/closures),
 *   reopening, and reachability inspection.
 */
class CampusMapManager {
private:
    CampusGraph graph_;
    std::vector<CampusLocation> locations_;

public:
    explicit CampusMapManager(int numNodes = 12)
        : graph_(numNodes), locations_() {
        locations_.resize(static_cast<size_t>(numNodes));
        for (int i = 0; i < numNodes; ++i) {
            locations_[static_cast<size_t>(i)].nodeId = i;
        }
    }

    void initialize(const std::vector<CampusLocation>& locations, const std::vector<CorridorEdge>& corridors) {
        int maxNodeId = 0;
        for (const auto& loc : locations) {
            if (loc.nodeId > maxNodeId) {
                maxNodeId = loc.nodeId;
            }
        }
        for (const auto& edge : corridors) {
            if (edge.u > maxNodeId) maxNodeId = edge.u;
            if (edge.v > maxNodeId) maxNodeId = edge.v;
        }

        int nodeCount = std::max(static_cast<int>(locations.size()), maxNodeId + 1);
        graph_ = CampusGraph(nodeCount);
        locations_ = locations;
        if (locations_.size() < static_cast<size_t>(nodeCount)) {
            locations_.resize(static_cast<size_t>(nodeCount));
            for (int i = 0; i < nodeCount; ++i) {
                locations_[static_cast<size_t>(i)].nodeId = i;
            }
        }

        for (const auto& edge : corridors) {
            graph_.addCorridor(edge.u, edge.v, edge.transitTimeSeconds);
            if (edge.isBlocked) {
                graph_.blockCorridor(edge.u, edge.v);
            }
        }
    }

    void reset(int numNodes = 12) {
        graph_ = CampusGraph(numNodes);
        locations_.clear();
        locations_.resize(static_cast<size_t>(numNodes));
        for (int i = 0; i < numNodes; ++i) {
            locations_[static_cast<size_t>(i)].nodeId = i;
        }
    }

    void clear() noexcept {
        graph_.clear();
        locations_.clear();
    }

    bool isValidNode(int nodeId) const noexcept {
        return graph_.isValidNode(nodeId);
    }

    int getNodeCount() const noexcept {
        return graph_.getNumNodes();
    }

    const CampusLocation* getLocation(int nodeId) const noexcept {
        if (nodeId < 0 || static_cast<size_t>(nodeId) >= locations_.size()) {
            return nullptr;
        }
        return &locations_[static_cast<size_t>(nodeId)];
    }

    const std::vector<CampusLocation>& getAllLocations() const noexcept {
        return locations_;
    }

    bool addCorridor(int u, int v, int64_t transitSeconds, const std::string& description = "") {
        return graph_.addCorridor(u, v, transitSeconds, description);
    }

    bool blockCorridor(int u, int v) {
        return graph_.blockCorridor(u, v);
    }

    bool reopenCorridor(int u, int v) {
        return graph_.reopenCorridor(u, v);
    }

    bool isCorridorBlocked(int u, int v) const {
        return graph_.isCorridorBlocked(u, v);
    }

    int64_t getCorridorWeight(int u, int v) const {
        return graph_.getCorridorWeight(u, v);
    }

    DijkstraResult findShortestRoute(int source, int destination) const {
        return graph_.findShortestRoute(source, destination);
    }

    std::vector<int> bfsAccessibleNodes(int startNode) const {
        std::vector<bool> visited;
        return graph_.bfs(startNode, visited);
    }

    MSTResult computePrimMST() const {
        return graph_.computePrimMST();
    }

    MSTResult computeKruskalMST() const {
        return graph_.computeKruskalMST();
    }

    std::string getNodeName(int nodeId) const {
        const CampusLocation* loc = getLocation(nodeId);
        return (loc && !loc->name.empty()) ? loc->name : ("Node " + std::to_string(nodeId));
    }

    std::vector<CorridorEdge> getAllCorridors() const {
        std::vector<CorridorEdge> corridors;
        int n = graph_.getNumNodes();
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                int64_t w = graph_.getOriginalCorridorWeight(u, v);
                if (w >= 0) {
                    CorridorEdge e;
                    e.u = u;
                    e.v = v;
                    e.transitTimeSeconds = w;
                    e.isBlocked = graph_.isCorridorBlocked(u, v);
                    corridors.push_back(e);
                }
            }
        }
        return corridors;
    }

    bool isCorridorOpen(int u, int v) const {
        return !graph_.isCorridorBlocked(u, v) && graph_.getCorridorWeight(u, v) >= 0;
    }

    CampusGraph& getGraph() noexcept {
        return graph_;
    }

    const CampusGraph& getGraph() const noexcept {
        return graph_;
    }
};
