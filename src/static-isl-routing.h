/**
 * Static ISL Routing
 *
 * Purpose: Compute and store static routing tables for ISL mesh using Dijkstra's algorithm
 * Design: All-pairs shortest path with next-hop lookup tables
 * Use case: Phase 3 Week 17 - Static routing baseline before OLSR comparison
 *
 * Research Evidence:
 * - Static routing optimal for stable LEO topologies (minimal link churn)
 * - Zero routing overhead (no control messages)
 * - Dijkstra optimal for uniform-cost graphs (all ISL links = 1 hop cost)
 *
 * Performance Targets:
 * - Compute routing tables: <1ms for 24 satellites
 * - Next-hop lookup: O(1) constant time
 * - Memory: O(V²) where V = number of satellites
 */

#ifndef STATIC_ISL_ROUTING_H
#define STATIC_ISL_ROUTING_H

#include "isl-topology-generator.h"
#include <map>
#include <vector>
#include <cstdint>

namespace ns3 {

/**
 * Static routing tables for ISL mesh
 *
 * Data structure: nextHop[src][dst] = next hop from src to dst
 * Lookup: O(1) average case (hash map)
 * Memory: O(V²) where V = number of satellites
 */
class RoutingTables {
public:
    /**
     * Get next hop for routing from src to dst
     * @param src Source satellite ID
     * @param dst Destination satellite ID
     * @return Next hop satellite ID, or UINT32_MAX if no route exists
     */
    uint32_t GetNextHop(uint32_t src, uint32_t dst) const;

    /**
     * Set next hop for routing from src to dst
     * @param src Source satellite ID
     * @param dst Destination satellite ID
     * @param nextHop Next hop satellite ID
     */
    void SetNextHop(uint32_t src, uint32_t dst, uint32_t nextHop);

    /**
     * Get all next hops from a source (for debugging)
     * @param src Source satellite ID
     * @return Map of destination → next hop
     */
    std::map<uint32_t, uint32_t> GetAllNextHops(uint32_t src) const;

private:
    // nextHop[src][dst] = next hop from src to dst
    std::map<uint32_t, std::map<uint32_t, uint32_t>> m_nextHop;
};

/**
 * Compute static routing tables using Dijkstra's algorithm
 *
 * Algorithm:
 * 1. For each source satellite, run Dijkstra to compute shortest paths
 * 2. Extract next-hop from predecessor array
 * 3. Store in routing tables
 *
 * Complexity: O(V × (V² + E)) = O(V³) for dense graph
 * For 24 satellites: ~14,000 operations (negligible)
 *
 * @param topology ISL topology (from isl-topology-generator)
 * @return Routing tables with next-hop for each (src, dst) pair
 */
RoutingTables ComputeStaticRoutes(const IslTopology& topology);

/**
 * Get hop count from src to dst using routing tables
 *
 * Follows next-hop pointers until destination is reached.
 * Detects routing loops (returns UINT32_MAX if loop detected).
 *
 * @param routes Routing tables
 * @param src Source satellite ID
 * @param dst Destination satellite ID
 * @return Number of hops, or UINT32_MAX if unreachable or loop detected
 */
uint32_t GetHopCount(const RoutingTables& routes, uint32_t src, uint32_t dst);

} // namespace ns3

#endif // STATIC_ISL_ROUTING_H
