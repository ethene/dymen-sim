/**
 * Manhattan Grid Mobility Helper
 * Phase 5 Week 26 - Advanced Mobility Models
 *
 * Provides grid-constrained waypoint selection for Manhattan Grid mobility.
 * Nodes move only on street grid lines (no diagonal movement).
 *
 * Grid Structure:
 * - N×N blocks (default 5×5)
 * - Each block is M meters wide (default 100m)
 * - (N+1)×(N+1) intersections where streets cross
 * - Nodes select random intersections as waypoints
 *
 * Usage:
 *   ManhattanGridHelper grid(5, 100.0, 500.0);
 *   Vector waypoint = grid.GetRandomIntersection();
 */

#ifndef MANHATTAN_MOBILITY_HELPER_H
#define MANHATTAN_MOBILITY_HELPER_H

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include <vector>
#include <cmath>

namespace ns3 {

class ManhattanGridHelper {
public:
    /**
     * Constructor
     *
     * @param blocks Number of blocks per dimension (creates N×N grid)
     * @param blockSize Width of each block in meters
     * @param areaBounds Total area bounds (must be >= blocks × blockSize)
     */
    ManhattanGridHelper(uint32_t blocks, double blockSize, double areaBounds)
        : m_blocks(blocks), m_blockSize(blockSize), m_areaBounds(areaBounds)
    {
        // Validate parameters
        double requiredBounds = blocks * blockSize;
        if (areaBounds < requiredBounds) {
            NS_LOG_WARN("Area bounds " << areaBounds << "m < required " << requiredBounds
                << "m for " << blocks << "×" << blocks << " grid with "
                << blockSize << "m blocks. Adjusting bounds.");
            m_areaBounds = requiredBounds;
        }

        // Pre-compute all grid intersections
        ComputeIntersections();
    }

    /**
     * Get all grid intersections.
     *
     * For N×N blocks, there are (N+1)×(N+1) intersections.
     * Example: 5×5 blocks → 6×6 = 36 intersections
     *
     * @return Vector of all intersection positions
     */
    std::vector<Vector> GetIntersections() const {
        return m_intersections;
    }

    /**
     * Get random intersection for waypoint selection.
     *
     * Selects uniformly at random from all grid intersections.
     *
     * @return Random intersection position
     */
    Vector GetRandomIntersection() const {
        if (m_intersections.empty()) {
            NS_LOG_ERROR("No intersections available!");
            return Vector(0, 0, 0);
        }

        // Use NS-3 RNG for consistency with simulation
        Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
        uint32_t idx = rand->GetInteger(0, m_intersections.size() - 1);
        return m_intersections[idx];
    }

    /**
     * Check if position is aligned to grid.
     *
     * Position is grid-aligned if:
     * - x is divisible by blockSize (on vertical street)
     * - OR y is divisible by blockSize (on horizontal street)
     * - Intersections: both x AND y divisible by blockSize
     *
     * @param position Position to check
     * @param tolerance Numerical tolerance in meters (default 1.0m)
     * @return True if position is on grid
     */
    bool IsOnGrid(Vector position, double tolerance = 1.0) const {
        double x = position.x;
        double y = position.y;

        // Check if x or y aligns to grid
        double x_remainder = std::fmod(x, m_blockSize);
        double y_remainder = std::fmod(y, m_blockSize);

        bool x_aligned = (x_remainder < tolerance) || (x_remainder > m_blockSize - tolerance);
        bool y_aligned = (y_remainder < tolerance) || (y_remainder > m_blockSize - tolerance);

        return x_aligned || y_aligned;
    }

    /**
     * Check if position is an intersection.
     *
     * @param position Position to check
     * @param tolerance Numerical tolerance in meters (default 1.0m)
     * @return True if position is an intersection
     */
    bool IsIntersection(Vector position, double tolerance = 1.0) const {
        double x = position.x;
        double y = position.y;

        double x_remainder = std::fmod(x, m_blockSize);
        double y_remainder = std::fmod(y, m_blockSize);

        bool x_aligned = (x_remainder < tolerance) || (x_remainder > m_blockSize - tolerance);
        bool y_aligned = (y_remainder < tolerance) || (y_remainder > m_blockSize - tolerance);

        return x_aligned && y_aligned;
    }

    /**
     * Get grid parameters.
     */
    uint32_t GetBlocks() const { return m_blocks; }
    double GetBlockSize() const { return m_blockSize; }
    double GetAreaBounds() const { return m_areaBounds; }
    uint32_t GetNumIntersections() const { return m_intersections.size(); }

private:
    /**
     * Compute all grid intersections.
     *
     * For N×N blocks with block size M:
     * - Intersections at (i*M, j*M) for i,j ∈ [0, N]
     * - Total: (N+1)×(N+1) intersections
     */
    void ComputeIntersections() {
        m_intersections.clear();

        uint32_t pointsPerDim = m_blocks + 1;  // N blocks → N+1 points
        for (uint32_t i = 0; i < pointsPerDim; ++i) {
            for (uint32_t j = 0; j < pointsPerDim; ++j) {
                double x = i * m_blockSize;
                double y = j * m_blockSize;
                m_intersections.push_back(Vector(x, y, 0));
            }
        }

        NS_LOG_INFO("Manhattan Grid: " << m_blocks << "×" << m_blocks << " blocks, "
            << m_blockSize << "m block size → " << m_intersections.size() << " intersections");
    }

    uint32_t m_blocks;               // Number of blocks per dimension
    double m_blockSize;              // Block width in meters
    double m_areaBounds;             // Total area bounds
    std::vector<Vector> m_intersections;  // Pre-computed intersection positions
};

} // namespace ns3

#endif // MANHATTAN_MOBILITY_HELPER_H
