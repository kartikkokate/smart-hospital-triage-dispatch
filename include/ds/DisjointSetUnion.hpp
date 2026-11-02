#pragma once

#include <cstddef>
#include <vector>

/**
 * @brief Custom Disjoint Set Union (DSU) with Path Compression and Union by Rank.
 * 
 * DESIGN RATIONALE:
 * - Efficiently manages disjoint equivalence classes of graph vertices.
 * - Used by Kruskal's algorithm to detect cycles in nearly O(1) amortized time (alpha(V)).
 * 
 * COMPLEXITY:
 * - find:   O(alpha(n)) amortized time via path compression.
 * - unite:  O(alpha(n)) amortized time via union by rank.
 * - space:  O(n) auxiliary space for parent and rank arrays.
 */
class DisjointSetUnion {
private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    int componentCount_ = 0;

public:
    explicit DisjointSetUnion(int n = 0) {
        init(n);
    }

    void init(int n) {
        if (n < 0) n = 0;
        parent_.resize(n);
        rank_.assign(n, 0);
        for (int i = 0; i < n; ++i) {
            parent_[i] = i;
        }
        componentCount_ = n;
    }

    // Path-compressed find
    int find(int i) {
        if (i < 0 || static_cast<size_t>(i) >= parent_.size()) {
            return -1;
        }
        if (parent_[i] != i) {
            parent_[i] = find(parent_[i]); // Path compression
        }
        return parent_[i];
    }

    // Union by rank
    bool unite(int i, int j) {
        int rootI = find(i);
        int rootJ = find(j);
        if (rootI == -1 || rootJ == -1 || rootI == rootJ) {
            return false; // Invalid or already connected (cycle detected)
        }

        if (rank_[rootI] < rank_[rootJ]) {
            parent_[rootI] = rootJ;
        } else if (rank_[rootI] > rank_[rootJ]) {
            parent_[rootJ] = rootI;
        } else {
            parent_[rootJ] = rootI;
            rank_[rootI]++;
        }
        componentCount_--;
        return true;
    }

    bool isConnected(int i, int j) {
        int rI = find(i);
        int rJ = find(j);
        return (rI != -1 && rI == rJ);
    }

    int componentCount() const noexcept {
        return componentCount_;
    }

    size_t size() const noexcept {
        return parent_.size();
    }
};
