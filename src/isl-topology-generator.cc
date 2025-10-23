/**
 * ISL Topology Generator Implementation
 *
 * Algorithm: Walker-Delta 53:24/3/1 with 4-neighbor ISL topology
 * - 2 intra-plane neighbors: Previous and next satellite in same orbital plane
 * - 2 inter-plane neighbors: Satellites in adjacent planes (fixed index approach)
 *
 * Based on real-world systems:
 * - Starlink: 4 laser terminals, "+Grid" topology
 * - Iridium: 4 Ka-band RF cross-links
 */

#include "isl-topology-generator.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <set>

namespace ns3 {

IslTopology GenerateWalkerDeltaTopology(uint32_t numSatellites, uint32_t neighborsPerSat) {
    IslTopology topology;
    topology.numSatellites = numSatellites;

    // Walker-Delta 53:24/3/1: 3 planes, 8 satellites per plane
    const uint32_t NUM_PLANES = 3;
    const uint32_t SATS_PER_PLANE = 8;

    // Validate input
    if (numSatellites != 24) {
        // For now, only support 24-satellite constellation
        // Future: generalize to other Walker-Delta configurations
        return topology;
    }

    if (neighborsPerSat != 4) {
        // Only 4-neighbor topology supported (industry standard)
        return topology;
    }

    // Generate neighbor relationships for each satellite
    for (uint32_t plane = 0; plane < NUM_PLANES; ++plane) {
        for (uint32_t idx = 0; idx < SATS_PER_PLANE; ++idx) {
            uint32_t satId = plane * SATS_PER_PLANE + idx;

            std::vector<uint32_t> neighbors;

            // === INTRA-PLANE NEIGHBORS (2) ===
            // Forward neighbor (next in same plane)
            uint32_t forward = plane * SATS_PER_PLANE + ((idx + 1) % SATS_PER_PLANE);
            neighbors.push_back(forward);

            // Backward neighbor (previous in same plane)
            uint32_t backward = plane * SATS_PER_PLANE + ((idx + SATS_PER_PLANE - 1) % SATS_PER_PLANE);
            neighbors.push_back(backward);

            // === INTER-PLANE NEIGHBORS (2) ===
            // Use fixed index approach (Option A from research):
            // Connect to same index in adjacent planes
            //
            // Rationale:
            // - Simple and deterministic
            // - Stable links (no dynamic recomputation needed)
            // - Validated by Starlink and Iridium architectures

            uint32_t nextPlane = (plane + 1) % NUM_PLANES;
            uint32_t prevPlane = (plane + NUM_PLANES - 1) % NUM_PLANES;

            // Same index in next plane
            uint32_t rightNeighbor = nextPlane * SATS_PER_PLANE + idx;
            neighbors.push_back(rightNeighbor);

            // Same index in previous plane
            uint32_t leftNeighbor = prevPlane * SATS_PER_PLANE + idx;
            neighbors.push_back(leftNeighbor);

            // Store neighbors for this satellite
            topology.neighbors[satId] = neighbors;
        }
    }

    // Count unique bidirectional links
    // Each link counted once (not twice for both directions)
    std::set<std::pair<uint32_t, uint32_t>> uniqueLinks;
    for (const auto& [satId, neighbors] : topology.neighbors) {
        for (uint32_t neighbor : neighbors) {
            // Normalize link direction (always store smaller ID first)
            auto link = (satId < neighbor) ? std::make_pair(satId, neighbor)
                                            : std::make_pair(neighbor, satId);
            uniqueLinks.insert(link);
        }
    }

    topology.numLinks = uniqueLinks.size();

    return topology;
}

double ComputeMeshConnectivity(const IslTopology& topology) {
    uint32_t reachablePairs = 0;
    uint32_t totalPairs = topology.numSatellites * (topology.numSatellites - 1);

    if (totalPairs == 0) {
        return 0.0; // Edge case: 0 or 1 satellites
    }

    for (uint32_t src = 0; src < topology.numSatellites; ++src) {
        std::vector<bool> reachable = BFS(topology, src);
        reachablePairs += std::count(reachable.begin(), reachable.end(), true) - 1; // Exclude self
    }

    return static_cast<double>(reachablePairs) / totalPairs;
}

std::vector<bool> BFS(const IslTopology& topology, uint32_t src) {
    std::vector<bool> visited(topology.numSatellites, false);
    std::queue<uint32_t> q;

    if (src >= topology.numSatellites) {
        return visited; // Invalid source
    }

    visited[src] = true;
    q.push(src);

    while (!q.empty()) {
        uint32_t current = q.front();
        q.pop();

        // Get neighbors of current satellite
        auto it = topology.neighbors.find(current);
        if (it != topology.neighbors.end()) {
            for (uint32_t neighbor : it->second) {
                if (neighbor < topology.numSatellites && !visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                }
            }
        }
    }

    return visited;
}

std::vector<uint32_t> Dijkstra(const IslTopology& topology, uint32_t src) {
    const uint32_t INF = std::numeric_limits<uint32_t>::max();
    std::vector<uint32_t> dist(topology.numSatellites, INF);
    std::vector<bool> visited(topology.numSatellites, false);

    if (src >= topology.numSatellites) {
        return dist; // Invalid source
    }

    dist[src] = 0;

    for (uint32_t i = 0; i < topology.numSatellites; ++i) {
        // Find unvisited node with minimum distance
        uint32_t u = INF;
        uint32_t minDist = INF;
        for (uint32_t v = 0; v < topology.numSatellites; ++v) {
            if (!visited[v] && dist[v] < minDist) {
                u = v;
                minDist = dist[v];
            }
        }

        if (u == INF) break; // No more reachable nodes

        visited[u] = true;

        // Update distances to neighbors
        auto it = topology.neighbors.find(u);
        if (it != topology.neighbors.end()) {
            for (uint32_t neighbor : it->second) {
                if (neighbor < topology.numSatellites && !visited[neighbor]) {
                    uint32_t newDist = dist[u] + 1; // Each ISL hop = cost 1
                    if (newDist < dist[neighbor]) {
                        dist[neighbor] = newDist;
                    }
                }
            }
        }
    }

    return dist;
}

} // namespace ns3
