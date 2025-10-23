/**
 * ISL Topology Generator
 *
 * Purpose: Generate Inter-Satellite Link (ISL) topology for Walker-Delta constellation
 * Design: 4 neighbors per satellite (2 intra-plane, 2 inter-plane)
 * Based on: Real-world Starlink and Iridium ISL patterns
 *
 * Research Evidence:
 * - Starlink: 4 laser terminals per satellite ("+Grid" topology)
 * - Iridium: 4 Ka-band RF cross-links per satellite
 * - Pattern: 2 intra-plane (fore/aft) + 2 inter-plane (left/right adjacent planes)
 */

#ifndef ISL_TOPOLOGY_GENERATOR_H
#define ISL_TOPOLOGY_GENERATOR_H

#include <vector>
#include <map>
#include <cstdint>

namespace ns3 {

/**
 * ISL Topology Data Structure
 *
 * Represents the Inter-Satellite Link mesh topology for a Walker-Delta constellation.
 */
struct IslTopology {
    uint32_t numSatellites;                              // Total number of satellites
    uint32_t numLinks;                                    // Number of bidirectional ISL links
    std::map<uint32_t, std::vector<uint32_t>> neighbors; // satId → list of neighbor satIds

    IslTopology() : numSatellites(0), numLinks(0) {}
};

/**
 * Generate ISL topology for Walker-Delta 53:24/3/1 constellation
 *
 * Algorithm:
 * 1. Intra-plane neighbors (2): Previous and next satellite in same orbital plane (ring topology)
 * 2. Inter-plane neighbors (2): Satellites in adjacent planes (fixed index or distance-based)
 *
 * @param numSatellites Total number of satellites (24 for Walker-Delta 53:24/3/1)
 * @param neighborsPerSat Number of ISL neighbors per satellite (4 recommended)
 * @return ISL topology structure with neighbor relationships
 *
 * Complexity: O(V) where V = numSatellites
 * Memory: O(V × neighborsPerSat)
 */
IslTopology GenerateWalkerDeltaTopology(uint32_t numSatellites, uint32_t neighborsPerSat);

/**
 * Compute mesh connectivity (percentage of satellite pairs that can reach each other)
 *
 * @param topology ISL topology to analyze
 * @return Connectivity percentage (0.0 to 1.0)
 *
 * Target: ≥0.95 (95% connectivity minimum)
 */
double ComputeMeshConnectivity(const IslTopology& topology);

/**
 * Breadth-first search from source satellite
 *
 * @param topology ISL topology
 * @param src Source satellite ID
 * @return Vector of reachable satellites (boolean array, indexed by satellite ID)
 */
std::vector<bool> BFS(const IslTopology& topology, uint32_t src);

/**
 * Dijkstra shortest path from source satellite
 *
 * @param topology ISL topology
 * @param src Source satellite ID
 * @return Vector of distances (hop count) to each satellite
 *
 * Uses hop count as metric (each ISL = 1 hop)
 */
std::vector<uint32_t> Dijkstra(const IslTopology& topology, uint32_t src);

} // namespace ns3

#endif // ISL_TOPOLOGY_GENERATOR_H
