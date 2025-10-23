/**
 * ISL Network Creator
 *
 * Purpose: Create ISL mesh network with PointToPoint links
 * Features:
 * - 48 bidirectional ISL links (Walker-Delta 4-neighbor topology)
 * - 10 Gbps data rate (optical ISL standard)
 * - Distance-based propagation delay (realistic LEO ISL)
 * - IP address assignment (10.x.x.x/30 subnets)
 * - Static routing table installation
 *
 * Usage:
 *   IslNetworkCreator creator;
 *   NetDeviceContainer islDevices = creator.CreateIslMesh(satellites, topology);
 *   Ipv4InterfaceContainer islInterfaces = creator.AssignIslAddresses(islDevices);
 *   creator.InstallStaticRoutes(satellites, routes, islInterfaces);
 */

#ifndef ISL_NETWORK_CREATOR_H
#define ISL_NETWORK_CREATOR_H

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv4-interface-container.h"
#include "isl-topology-generator.h"
#include "static-isl-routing.h"
#include <map>

namespace ns3 {

/**
 * Helper class to create ISL network infrastructure
 */
class IslNetworkCreator {
public:
    /**
     * Create ISL mesh network with PointToPoint links
     *
     * @param satellites Node container with 24 satellites
     * @param topology ISL topology (from GenerateWalkerDeltaTopology)
     * @return ISL device container (all 96 ISL devices = 48 links × 2 devices/link)
     */
    NetDeviceContainer CreateIslMesh(NodeContainer satellites, const IslTopology& topology);

    /**
     * Assign IP addresses to ISL links
     *
     * Uses 10.x.x.x/30 subnets (one per ISL link)
     * Example: Link 0 = 10.0.0.0/30, Link 1 = 10.0.0.4/30, etc.
     *
     * @param islDevices ISL device container (from CreateIslMesh)
     * @return ISL interface container (96 interfaces)
     */
    Ipv4InterfaceContainer AssignIslAddresses(const NetDeviceContainer& islDevices);

    /**
     * Install static routes for ISL mesh
     *
     * @param satellites Node container with 24 satellites
     * @param routes Static routing tables (from ComputeStaticRoutes)
     * @param islInterfaces ISL interface container (from AssignIslAddresses)
     */
    void InstallStaticRoutes(NodeContainer satellites,
                            const RoutingTables& routes,
                            const Ipv4InterfaceContainer& islInterfaces);

    /**
     * Compute distance between two satellites (in meters)
     * Uses satellite positions from SatelliteMobilityModel
     *
     * @param sat1 First satellite node
     * @param sat2 Second satellite node
     * @return Distance in meters
     */
    double ComputeSatelliteDistance(Ptr<Node> sat1, Ptr<Node> sat2);

    /**
     * Compute propagation delay from distance
     *
     * Uses speed of light in vacuum: 299,792,458 m/s
     *
     * @param distance_m Distance in meters
     * @return Propagation delay (Time)
     */
    Time ComputePropagationDelay(double distance_m);

private:
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> m_linkToInterface; // (sat1, sat2) -> interface index
};

} // namespace ns3

#endif // ISL_NETWORK_CREATOR_H
