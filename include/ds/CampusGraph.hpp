#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include "models/CampusLocation.hpp"
#include "algorithms/DistanceArithmetic.hpp"
#include "algorithms/CustomSorting.hpp"
#include "ds/SinglyLinkedList.hpp"
#include "ds/CustomStack.hpp"
#include "ds/CustomQueue.hpp"
#include "ds/DisjointSetUnion.hpp"
#include "ds/BinaryMinHeap.hpp"

/**
 * @brief Authoritative Campus Graph Network Engine with Synchronized Representations.
 * 
 * DESIGN RATIONALE:
 * - Maintains synchronized Adjacency List (using SinglyLinkedList) and Adjacency Matrix.
 * - Central mutation interface guarantees representations never diverge.
 * - Explicit sentinels:
 *     NO_EDGE = -1      (No corridor exists)
 *     BLOCKED_EDGE = -2 (Corridor exists but is currently blocked)
 *     Weight >= 0       (Open corridor with non-negative transit time; 0 is valid)
 * - Deterministic neighbor traversal order (sorted by target node ID).
 * - Full graph traversal (BFS via CustomQueue, DFS List, DFS Matrix).
 * - Undirected cycle detection with parent-edge handling across all components.
 * - Dijkstra shortest path using generic BinaryMinHeap, safe distance arithmetic,
 *   and CustomStack path reconstruction.
 * - Minimum Spanning Trees via Kruskal (customMergeSort + DSU) and Prim (BinaryMinHeap).
 */

struct GraphNeighbor {
    int to = -1;
    int64_t weight = 0;
    bool isBlocked = false;
    std::string description;

    GraphNeighbor() = default;
    GraphNeighbor(int t, int64_t w, bool b = false, const std::string& desc = "")
        : to(t), weight(w), isBlocked(b), description(desc) {}

    bool operator==(const GraphNeighbor& other) const noexcept {
        return to == other.to;
    }
};

struct DijkstraResult {
    bool isReachable = false;
    int64_t totalCost = kInfinity;
    std::vector<int> path; // Ordered node path from source to destination
};

struct MSTResult {
    bool isConnectedSpanningTree = false;
    int64_t totalTransitWeight = 0;
    std::vector<CorridorEdge> edges;
    int componentCount = 1;
    bool hasOverflow = false;
};

class CampusGraph {
public:
    enum : int64_t {
        NO_EDGE = -1,
        BLOCKED_EDGE = -2
    };

private:
    int numNodes_ = 0;
    std::vector<CampusLocation> locations_;
    std::vector<SinglyLinkedList<GraphNeighbor>> adjList_;
    std::vector<std::vector<int64_t>> adjMatrix_;
    std::vector<std::vector<int64_t>> originalWeightMatrix_; // Preserves baseline weights when blocked


    // Comparator for sorting unique undirected edges in Kruskal's algorithm
    struct KruskalEdgeComparator {
        bool operator()(const CorridorEdge& a, const CorridorEdge& b) const noexcept {
            if (a.transitTimeSeconds != b.transitTimeSeconds) {
                return a.transitTimeSeconds < b.transitTimeSeconds;
            }
            if (a.u != b.u) return a.u < b.u;
            return a.v < b.v;
        }
    };

    // Heap node for Dijkstra's algorithm
    struct DijkstraHeapNode {
        int u;
        int64_t dist;
    };

    struct DijkstraHeapNodeComp {
        bool operator()(const DijkstraHeapNode& a, const DijkstraHeapNode& b) const noexcept {
            return a.dist < b.dist; // Min-heap based on distance
        }
    };

    // Heap edge for Prim's algorithm
    struct PrimHeapEdge {
        int u;
        int v;
        int64_t weight;
    };

    struct PrimHeapEdgeComp {
        bool operator()(const PrimHeapEdge& a, const PrimHeapEdge& b) const noexcept {
            return a.weight < b.weight;
        }
    };

    // DFS Cycle Detection helper with parent check
    bool dfsCycleInternal(int u, int parentNode, std::vector<bool>& visited) const {
        visited[u] = true;

        // Iterate through neighbors in deterministic sorted order
        std::vector<GraphNeighbor> neighbors = getSortedNeighbors(u);
        for (const auto& edge : neighbors) {
            if (edge.isBlocked) continue; // Ignore blocked corridors

            int v = edge.to;
            if (!visited[v]) {
                if (dfsCycleInternal(v, u, visited)) {
                    return true;
                }
            } else if (v != parentNode) {
                // Back-edge to an already visited vertex that is NOT the immediate parent
                return true;
            }
        }
        return false;
    }

    void dfsListInternal(int u, std::vector<bool>& visited, std::vector<int>& outOrder) const {
        visited[u] = true;
        outOrder.push_back(u);

        std::vector<GraphNeighbor> neighbors = getSortedNeighbors(u);
        for (const auto& edge : neighbors) {
            if (!edge.isBlocked && !visited[edge.to]) {
                dfsListInternal(edge.to, visited, outOrder);
            }
        }
    }

    void dfsMatrixInternal(int u, std::vector<bool>& visited, std::vector<int>& outOrder) const {
        visited[u] = true;
        outOrder.push_back(u);

        for (int v = 0; v < numNodes_; ++v) {
            if (adjMatrix_[u][v] >= 0 && !visited[v]) {
                dfsMatrixInternal(v, visited, outOrder);
            }
        }
    }

public:
    explicit CampusGraph(int numNodes = 0) {
        init(numNodes);
    }

    void init(int numNodes) {
        if (numNodes < 0) numNodes = 0;
        numNodes_ = numNodes;
        locations_.clear();
        locations_.resize(numNodes_);
        for (int i = 0; i < numNodes_; ++i) {
            locations_[i].nodeId = i;
            locations_[i].name = "Node " + std::to_string(i);
        }

        adjList_ = std::vector<SinglyLinkedList<GraphNeighbor>>(numNodes_);
        adjMatrix_ = std::vector<std::vector<int64_t>>(numNodes_, std::vector<int64_t>(numNodes_, NO_EDGE));
        originalWeightMatrix_ = std::vector<std::vector<int64_t>>(numNodes_, std::vector<int64_t>(numNodes_, NO_EDGE));
    }

    void clear() noexcept {
        numNodes_ = 0;
        locations_.clear();
        for (auto& list : adjList_) {
            list.clear();
        }
        adjList_.clear();
        adjMatrix_.clear();
        originalWeightMatrix_.clear();
    }

    int getNumNodes() const noexcept { return numNodes_; }
    bool isValidNode(int u) const noexcept { return (u >= 0 && u < numNodes_); }

    void setLocation(int nodeId, const CampusLocation& loc) {
        if (isValidNode(nodeId)) {
            locations_[nodeId] = loc;
        }
    }

    const CampusLocation* getLocation(int nodeId) const noexcept {
        return isValidNode(nodeId) ? &locations_[nodeId] : nullptr;
    }

    // Helper: extracts and returns neighbors of u directly from custom SinglyLinkedList in ascending target order.
    // Neighbors are maintained in ascending order during insertion, avoiding std::sort overhead during traversals.
    std::vector<GraphNeighbor> getSortedNeighbors(int u) const {
        std::vector<GraphNeighbor> result;
        if (!isValidNode(u)) return result;

        for (const auto& neighbor : adjList_[u]) {
            result.push_back(neighbor);
        }
        return result;
    }

    /**
     * @brief Retrieves the adjacency list for vertex u.
     * 
     * @param u Vertex index to query.
     * @return const SinglyLinkedList<GraphNeighbor>* Pointer to vertex u's adjacency list,
     *         or nullptr if u is invalid (< 0 or >= numNodes_).
     */
    const SinglyLinkedList<GraphNeighbor>* getAdjacencyList(int u) const noexcept {
        return isValidNode(u) ? &adjList_[u] : nullptr;
    }

    const std::vector<std::vector<int64_t>>& getAdjacencyMatrix() const noexcept {
        return adjMatrix_;
    }

    // Central mutation: transactional bidirectional corridor addition
    bool addCorridor(int u, int v, int64_t transitSeconds, const std::string& description = "") {
        if (!isValidNode(u) || !isValidNode(v)) return false;
        if (u == v) return false; // Self-loops not permitted
        if (transitSeconds < 0 || transitSeconds > kMaxCorridorWeight) return false; // Enforce range [0, 86400]

        // Reject duplicate undirected edges
        if (adjMatrix_[u][v] != NO_EDGE) {
            return false;
        }

        // Transactional insertion: maintain ascending neighbor order without std::sort
        auto neighborComp = [](const GraphNeighbor& a, const GraphNeighbor& b) noexcept {
            return a.to < b.to;
        };

        try {
            adjList_[u].insert_sorted(GraphNeighbor(v, transitSeconds, false, description), neighborComp);
            try {
                adjList_[v].insert_sorted(GraphNeighbor(u, transitSeconds, false, description), neighborComp);
            } catch (...) {
                adjList_[u].remove_value(GraphNeighbor(v, 0));
                throw;
            }
        } catch (...) {
            return false;
        }

        // Publish matrix updates only AFTER both list insertions succeed
        adjMatrix_[u][v] = transitSeconds;
        adjMatrix_[v][u] = transitSeconds;
        originalWeightMatrix_[u][v] = transitSeconds;
        originalWeightMatrix_[v][u] = transitSeconds;

        return true;
    }

    // Central mutation: block corridor bidirectionally
    bool blockCorridor(int u, int v) {
        if (!isValidNode(u) || !isValidNode(v)) return false;
        if (adjMatrix_[u][v] == NO_EDGE) return false; // No such corridor
        if (adjMatrix_[u][v] == BLOCKED_EDGE) return true; // Already blocked

        adjMatrix_[u][v] = BLOCKED_EDGE;
        adjMatrix_[v][u] = BLOCKED_EDGE;

        // Update list
        GraphNeighbor* e1 = adjList_[u].find(GraphNeighbor(v, 0));
        if (e1) e1->isBlocked = true;
        GraphNeighbor* e2 = adjList_[v].find(GraphNeighbor(u, 0));
        if (e2) e2->isBlocked = true;

        return true;
    }

    // Central mutation: reopen corridor bidirectionally
    bool reopenCorridor(int u, int v) {
        if (!isValidNode(u) || !isValidNode(v)) return false;
        if (adjMatrix_[u][v] == NO_EDGE) return false; // No such corridor
        if (adjMatrix_[u][v] >= 0) return true; // Already open

        int64_t originalWeight = originalWeightMatrix_[u][v];
        adjMatrix_[u][v] = originalWeight;
        adjMatrix_[v][u] = originalWeight;

        GraphNeighbor* e1 = adjList_[u].find(GraphNeighbor(v, 0));
        if (e1) e1->isBlocked = false;
        GraphNeighbor* e2 = adjList_[v].find(GraphNeighbor(u, 0));
        if (e2) e2->isBlocked = false;

        return true;
    }

    bool isCorridorBlocked(int u, int v) const {
        if (!isValidNode(u) || !isValidNode(v)) return false;
        return adjMatrix_[u][v] == BLOCKED_EDGE;
    }

    int64_t getCorridorWeight(int u, int v) const {
        if (!isValidNode(u) || !isValidNode(v)) return NO_EDGE;
        return adjMatrix_[u][v];
    }

    int64_t getOriginalCorridorWeight(int u, int v) const {
        if (!isValidNode(u) || !isValidNode(v)) return NO_EDGE;
        return originalWeightMatrix_[u][v];
    }

    // Representation agreement verification helper
    bool validateRepresentationAgreement() const {
        for (int u = 0; u < numNodes_; ++u) {
            for (int v = 0; v < numNodes_; ++v) {
                int64_t matVal = adjMatrix_[u][v];
                // Check symmetry
                if (matVal != adjMatrix_[v][u]) return false;
                if (originalWeightMatrix_[u][v] != originalWeightMatrix_[v][u]) return false;

                const GraphNeighbor* edgeInU = adjList_[u].find(GraphNeighbor(v, 0));
                const GraphNeighbor* edgeInV = adjList_[v].find(GraphNeighbor(u, 0));

                if (matVal == NO_EDGE) {
                    if (edgeInU != nullptr || edgeInV != nullptr) return false;
                } else if (matVal == BLOCKED_EDGE) {
                    if (edgeInU == nullptr || !edgeInU->isBlocked) return false;
                    if (edgeInV == nullptr || !edgeInV->isBlocked) return false;
                    if (edgeInU->weight != originalWeightMatrix_[u][v]) return false;
                } else { // Open edge with weight >= 0
                    if (edgeInU == nullptr || edgeInU->isBlocked || edgeInU->weight != matVal) return false;
                    if (edgeInV == nullptr || edgeInV->isBlocked || edgeInV->weight != matVal) return false;
                }
            }
        }
        return true;
    }

    // ========================================================================
    // B. TRAVERSAL & CYCLE DETECTION
    // ========================================================================

    // BFS using CustomQueue
    std::vector<int> bfs(int startNode, std::vector<bool>& outVisited) const {
        std::vector<int> visitOrder;
        outVisited.assign(numNodes_, false);
        if (!isValidNode(startNode)) return visitOrder;

        CustomQueue<int> queue;
        outVisited[startNode] = true;
        queue.enqueue(startNode);

        while (!queue.isEmpty()) {
            int u = -1;
            queue.dequeue(u);
            visitOrder.push_back(u);

            std::vector<GraphNeighbor> neighbors = getSortedNeighbors(u);
            for (const auto& edge : neighbors) {
                if (!edge.isBlocked && !outVisited[edge.to]) {
                    outVisited[edge.to] = true;
                    queue.enqueue(edge.to);
                }
            }
        }
        return visitOrder;
    }

    // DFS List implementation
    std::vector<int> dfsList(int startNode, std::vector<bool>& outVisited) const {
        std::vector<int> visitOrder;
        outVisited.assign(numNodes_, false);
        if (!isValidNode(startNode)) return visitOrder;
        dfsListInternal(startNode, outVisited, visitOrder);
        return visitOrder;
    }

    // DFS Matrix implementation (matches deterministic ascending traversal order of dfsList)
    std::vector<int> dfsMatrix(int startNode, std::vector<bool>& outVisited) const {
        std::vector<int> visitOrder;
        outVisited.assign(numNodes_, false);
        if (!isValidNode(startNode)) return visitOrder;
        dfsMatrixInternal(startNode, outVisited, visitOrder);
        return visitOrder;
    }

    std::vector<int> bfs(int startNode) const {
        std::vector<bool> visited;
        return bfs(startNode, visited);
    }

    std::vector<int> dfsList(int startNode) const {
        std::vector<bool> visited;
        return dfsList(startNode, visited);
    }

    std::vector<int> dfsMatrix(int startNode) const {
        std::vector<bool> visited;
        return dfsMatrix(startNode, visited);
    }

    // DFS Matrix iterative implementation using CustomStack
    std::vector<int> dfsMatrixStack(int startNode, std::vector<bool>& outVisited) const {
        std::vector<int> visitOrder;
        outVisited.assign(numNodes_, false);
        if (!isValidNode(startNode)) return visitOrder;

        CustomStack<int> stack;
        stack.push(startNode);

        while (!stack.isEmpty()) {
            int u = -1;
            stack.pop(u);
            if (!outVisited[u]) {
                outVisited[u] = true;
                visitOrder.push_back(u);

                // Push neighbors in reverse order so smaller node IDs are visited first
                for (int v = numNodes_ - 1; v >= 0; --v) {
                    if (adjMatrix_[u][v] >= 0 && !outVisited[v]) {
                        stack.push(v);
                    }
                }
            }
        }
        return visitOrder;
    }

    // Connected components reporting using a single shared visited array across all components in O(V + E) time
    std::vector<std::vector<int>> getConnectedComponents() const {
        std::vector<std::vector<int>> components;
        std::vector<bool> visited(numNodes_, false);

        for (int i = 0; i < numNodes_; ++i) {
            if (!visited[i]) {
                std::vector<int> compNodes;
                CustomQueue<int> queue;
                visited[i] = true;
                queue.enqueue(i);

                while (!queue.isEmpty()) {
                    int u = -1;
                    queue.dequeue(u);
                    compNodes.push_back(u);

                    for (const auto& edge : adjList_[u]) {
                        if (!edge.isBlocked && !visited[edge.to]) {
                            visited[edge.to] = true;
                            queue.enqueue(edge.to);
                        }
                    }
                }
                components.push_back(compNodes);
            }
        }
        return components;
    }

    // Undirected cycle detection searching EVERY component
    bool hasCycle() const {
        std::vector<bool> visited(numNodes_, false);

        for (int i = 0; i < numNodes_; ++i) {
            if (!visited[i]) {
                if (dfsCycleInternal(i, -1, visited)) {
                    return true;
                }
            }
        }
        return false;
    }

    // ========================================================================
    // C. DIJKSTRA SHORTEST ROUTE
    // ========================================================================

    DijkstraResult findShortestRoute(int source, int destination) const {
        DijkstraResult result;
        if (!isValidNode(source) || !isValidNode(destination)) {
            result.isReachable = false;
            result.totalCost = kInfinity;
            return result;
        }

        // Zero-cost identity check
        if (source == destination) {
            result.isReachable = true;
            result.totalCost = 0;
            result.path = { source };
            return result;
        }

        std::vector<int64_t> dist(numNodes_, kInfinity);
        std::vector<int> parent(numNodes_, -1);
        dist[source] = 0;

        BinaryMinHeap<DijkstraHeapNode, DijkstraHeapNodeComp> minHeap;
        minHeap.insert({source, 0});

        while (!minHeap.empty()) {
            DijkstraHeapNode current;
            minHeap.extractMin(current);
            int u = current.u;
            int64_t d = current.dist;

            // Stale entry check
            if (d > dist[u]) continue;

            if (u == destination) break; // Reached target

            std::vector<GraphNeighbor> neighbors = getSortedNeighbors(u);
            for (const auto& edge : neighbors) {
                if (edge.isBlocked) continue; // Ignore blocked corridors

                if (canRelaxEdge(dist[u], edge.weight, dist[edge.to])) {
                    int64_t newDist = safeDistanceAdd(dist[u], edge.weight);
                    dist[edge.to] = newDist;
                    parent[edge.to] = u;
                    minHeap.insert({edge.to, newDist});
                }
            }
        }

        if (dist[destination] == kInfinity) {
            result.isReachable = false;
            result.totalCost = kInfinity;
            result.path.clear();
            return result;
        }

        // Reconstruct path using CustomStack (LIFO order backtracks from target to source)
        CustomStack<int> pathStack;
        int curr = destination;
        while (curr != -1) {
            pathStack.push(curr);
            curr = parent[curr];
        }

        result.isReachable = true;
        result.totalCost = dist[destination];
        result.path.clear();
        while (!pathStack.isEmpty()) {
            int node = -1;
            pathStack.pop(node);
            result.path.push_back(node);
        }

        return result;
    }

    // ========================================================================
    // D. MINIMUM SPANNING TREES (Kruskal & Prim)
    // ========================================================================

    /**
     * @brief Computes Minimum Spanning Tree / Forest via Kruskal's algorithm.
     * 
     * COMPLEXITY:
     * - Edge extraction: O(V^2) scan of the upper-triangular adjacency matrix.
     * - Sorting: O(E log E) via customMergeSort.
     * - DisjointSetUnion: O(E * alpha(V)) path compression + union by rank.
     * - Overall Time: O(V^2 + E log E). Space: O(V + E).
     * 
     * EMPTY GRAPH SEMANTICS:
     * - V == 0: componentCount = 0, isConnectedSpanningTree = false, totalTransitWeight = 0.
     * - V == 1: componentCount = 1, isConnectedSpanningTree = true, totalTransitWeight = 0.
     */
    MSTResult computeKruskalMST() const {
        MSTResult result;
        if (numNodes_ == 0) {
            result.componentCount = 0;
            result.isConnectedSpanningTree = false;
            result.totalTransitWeight = 0;
            result.edges.clear();
            result.hasOverflow = false;
            return result;
        }
        if (numNodes_ == 1) {
            result.componentCount = 1;
            result.isConnectedSpanningTree = true;
            result.totalTransitWeight = 0;
            result.edges.clear();
            result.hasOverflow = false;
            return result;
        }

        // 1. Collect all unique open undirected edges (u < v) - O(V^2) matrix scan
        std::vector<CorridorEdge> allEdges;
        for (int u = 0; u < numNodes_; ++u) {
            for (int v = u + 1; v < numNodes_; ++v) {
                int64_t w = adjMatrix_[u][v];
                if (w >= 0) { // Open edge
                    CorridorEdge e;
                    e.u = u;
                    e.v = v;
                    e.transitTimeSeconds = w;
                    e.isBlocked = false;
                    allEdges.push_back(e);
                }
            }
        }

        // 2. Sort edges by transit time using customMergeSort in O(E log E)
        ds_algorithms::customMergeSort(allEdges, KruskalEdgeComparator());

        // 3. Kruskal's algorithm with DisjointSetUnion
        DisjointSetUnion dsu(numNodes_);
        for (const auto& edge : allEdges) {
            if (dsu.unite(edge.u, edge.v)) {
                result.edges.push_back(edge);
                if (result.totalTransitWeight > kInfinity - edge.transitTimeSeconds) {
                    result.totalTransitWeight = kInfinity;
                    result.hasOverflow = true;
                } else {
                    result.totalTransitWeight += edge.transitTimeSeconds;
                }
                if (result.edges.size() == static_cast<size_t>(numNodes_ - 1)) {
                    break;
                }
            }
        }

        result.componentCount = dsu.componentCount();
        result.isConnectedSpanningTree = (result.edges.size() == static_cast<size_t>(numNodes_ - 1) &&
                                          result.componentCount == 1);
        return result;
    }

    /**
     * @brief Computes Minimum Spanning Tree / Forest via Prim's algorithm.
     * 
     * COMPLEXITY:
     * - Uses generic BinaryMinHeap with lazy deletion / stale-edge discards.
     * - Time: O((V + E) log V). Space: O(V + E).
     * 
     * EMPTY GRAPH SEMANTICS:
     * - V == 0: componentCount = 0, isConnectedSpanningTree = false, totalTransitWeight = 0.
     * - V == 1: componentCount = 1, isConnectedSpanningTree = true, totalTransitWeight = 0.
     */
    MSTResult computePrimMST(int startNode = 0) const {
        MSTResult result;
        if (numNodes_ == 0) {
            result.componentCount = 0;
            result.isConnectedSpanningTree = false;
            result.totalTransitWeight = 0;
            result.edges.clear();
            result.hasOverflow = false;
            return result;
        }
        if (numNodes_ == 1) {
            result.componentCount = 1;
            result.isConnectedSpanningTree = true;
            result.totalTransitWeight = 0;
            result.edges.clear();
            result.hasOverflow = false;
            return result;
        }
        if (!isValidNode(startNode)) startNode = 0;

        std::vector<bool> inMST(numNodes_, false);
        BinaryMinHeap<PrimHeapEdge, PrimHeapEdgeComp> minHeap;

        auto addNodeEdges = [&](int u) {
            inMST[u] = true;
            std::vector<GraphNeighbor> neighbors = getSortedNeighbors(u);
            for (const auto& edge : neighbors) {
                if (!edge.isBlocked && !inMST[edge.to]) {
                    minHeap.insert({u, edge.to, edge.weight});
                }
            }
        };

        // Grow MST from startNode
        addNodeEdges(startNode);

        while (!minHeap.empty() && result.edges.size() < static_cast<size_t>(numNodes_ - 1)) {
            PrimHeapEdge edge;
            minHeap.extractMin(edge);

            if (inMST[edge.v]) {
                continue; // Stale edge: vertex already in MST
            }

            CorridorEdge mstEdge;
            mstEdge.u = std::min(edge.u, edge.v);
            mstEdge.v = std::max(edge.u, edge.v);
            mstEdge.transitTimeSeconds = edge.weight;
            mstEdge.isBlocked = false;

            result.edges.push_back(mstEdge);
            if (result.totalTransitWeight > kInfinity - edge.weight) {
                result.totalTransitWeight = kInfinity;
                result.hasOverflow = true;
            } else {
                result.totalTransitWeight += edge.weight;
            }

            addNodeEdges(edge.v);
        }

        // If graph is disconnected, span all remaining components to construct the complete Minimum Spanning Forest
        for (int i = 0; i < numNodes_; ++i) {
            if (!inMST[i]) {
                addNodeEdges(i);
                while (!minHeap.empty() && result.edges.size() < static_cast<size_t>(numNodes_ - 1)) {
                    PrimHeapEdge edge;
                    minHeap.extractMin(edge);

                    if (inMST[edge.v]) {
                        continue;
                    }

                    CorridorEdge mstEdge;
                    mstEdge.u = std::min(edge.u, edge.v);
                    mstEdge.v = std::max(edge.u, edge.v);
                    mstEdge.transitTimeSeconds = edge.weight;
                    mstEdge.isBlocked = false;

                    result.edges.push_back(mstEdge);
                    if (result.totalTransitWeight > kInfinity - edge.weight) {
                        result.totalTransitWeight = kInfinity;
                        result.hasOverflow = true;
                    } else {
                        result.totalTransitWeight += edge.weight;
                    }

                    addNodeEdges(edge.v);
                }
            }
        }

        result.componentCount = numNodes_ - static_cast<int>(result.edges.size());
        result.isConnectedSpanningTree = (result.componentCount == 1 &&
                                          result.edges.size() == static_cast<size_t>(numNodes_ - 1));
        return result;
    }
};
