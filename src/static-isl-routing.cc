/**
 * Static ISL Routing Implementation
 *
 * Algorithm: Dijkstra's all-pairs shortest path
 * - Run Dijkstra from each source satellite
 * - Extract next-hop from predecessor array
 * - Store in O(1) lookup table
 *
 * Complexity: O(V³) for dense graph (V = number of satellites)
 * For 24 satellites: ~14,000 operations (negligible)
 *
 * Status: FULL IMPLEMENTATION (GREEN PHASE)
 */

#include "static-isl-routing.h"
#include <queue>
#include <limits>
#include <algorithm>
#include <set>

namespace ns3 {

uint32_t RoutingTables::GetNextHop(uint32_t src, uint32_t dst) const {
    auto srcIt = m_nextHop.find(src);
    if (srcIt == m_nextHop.end()) {
        return UINT32_MAX; // No routes from src
    }

    auto dstIt = srcIt->second.find(dst);
    if (dstIt == srcIt->second.end()) {
        return UINT32_MAX; // No route to dst
    }

    return dstIt->second;
}

void RoutingTables::SetNextHop(uint32_t src, uint32_t dst, uint32_t nextHop) {
    m_nextHop[src][dst] = nextHop;
}

std::map<uint32_t, uint32_t> RoutingTables::GetAllNextHops(uint32_t src) const {
    auto it = m_nextHop.find(src);
    if (it == m_nextHop.end()) {
        return {};
    }
    return it->second;
}

RoutingTables ComputeStaticRoutes(const IslTopology& topology) {
    RoutingTables routes;

    // For each source satellite, run Dijkstra to compute shortest paths
    for (uint32_t src = 0; src < topology.numSatellites; ++src) {
        // Dijkstra's algorithm
        const uint32_t INF = std::numeric_limits<uint32_t>::max();
        std::vector<uint32_t> dist(topology.numSatellites, INF);
        std::vector<uint32_t> prev(topology.numSatellites, UINT32_MAX);
        std::vector<bool> visited(topology.numSatellites, false);

        dist[src] = 0;

        // Priority queue: (distance, node)
        using PQNode = std::pair<uint32_t, uint32_t>;
        std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> pq;
        pq.push({0, src});

        while (!pq.empty()) {
            uint32_t u = pq.top().second;
            pq.pop();

            if (visited[u]) continue;
            visited[u] = true;

            // Relaxation step
            auto neighborIt = topology.neighbors.find(u);
            if (neighborIt != topology.neighbors.end()) {
                const auto& neighbors = neighborIt->second;
                for (uint32_t v : neighbors) {
                    if (!visited[v]) {
                        uint32_t newDist = dist[u] + 1; // Each ISL hop = cost 1
                        if (newDist < dist[v]) {
                            dist[v] = newDist;
                            prev[v] = u;
                            pq.push({newDist, v});
                        }
                    }
                }
            }
        }

        // Extract next-hops from prev[] array
        for (uint32_t dst = 0; dst < topology.numSatellites; ++dst) {
            if (src == dst) continue; // No route to self
            if (dist[dst] == INF) continue; // Unreachable (should never happen with 100% connectivity)

            // Trace back from dst to src to find first hop
            uint32_t current = dst;
            while (prev[current] != src && prev[current] != UINT32_MAX) {
                current = prev[current];
            }

            // current is now the first hop from src to dst
            routes.SetNextHop(src, dst, current);
        }
    }

    return routes;
}

uint32_t GetHopCount(const RoutingTables& routes, uint32_t src, uint32_t dst) {
    if (src == dst) return 0;

    uint32_t hops = 0;
    uint32_t current = src;
    std::set<uint32_t> visited;

    while (current != dst) {
        if (visited.count(current)) {
            return UINT32_MAX; // Loop detected
        }
        visited.insert(current);

        current = routes.GetNextHop(current, dst);
        if (current == UINT32_MAX) {
            return UINT32_MAX; // No route
        }

        hops++;
        if (hops > 100) {
            return UINT32_MAX; // Infinite loop protection
        }
    }

    return hops;
}

} // namespace ns3
