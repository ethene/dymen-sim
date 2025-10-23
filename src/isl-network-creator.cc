/**
 * ISL Network Creator Implementation
 *
 * Creates ISL mesh network with realistic PointToPoint links.
 * Handles:
 * - Link creation with 10 Gbps data rate
 * - Distance-based propagation delay computation
 * - IP address assignment (10.x.x.x/30 subnets)
 * - Static routing table installation
 */

#include "isl-network-creator.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IslNetworkCreator");

NetDeviceContainer IslNetworkCreator::CreateIslMesh(NodeContainer satellites, const IslTopology& topology) {
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(satellites.GetN() == topology.numSatellites,
        "Satellite count mismatch: " << satellites.GetN() << " vs " << topology.numSatellites);

    NetDeviceContainer allIslDevices;
    PointToPointHelper islHelper;

    // Configure ISL link properties (10 Gbps optical ISL)
    islHelper.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    islHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));

    // Create ISL links
    std::set<std::pair<uint32_t, uint32_t>> createdLinks; // Track created links

    for (uint32_t sat = 0; sat < topology.numSatellites; ++sat) {
        const auto& neighbors = topology.neighbors.at(sat);

        for (uint32_t neighbor : neighbors) {
            // Avoid duplicate links (only create sat < neighbor)
            if (sat >= neighbor) continue;

            // Check if link already created
            std::pair<uint32_t, uint32_t> linkPair = {sat, neighbor};
            if (createdLinks.count(linkPair)) continue;

            // Compute distance-based delay
            Ptr<Node> node1 = satellites.Get(sat);
            Ptr<Node> node2 = satellites.Get(neighbor);
            double distance = ComputeSatelliteDistance(node1, node2);
            Time delay = ComputePropagationDelay(distance);

            // Set channel delay
            islHelper.SetChannelAttribute("Delay", TimeValue(delay));

            // Install link
            NetDeviceContainer linkDevices = islHelper.Install(node1, node2);
            allIslDevices.Add(linkDevices);

            createdLinks.insert(linkPair);

            NS_LOG_INFO("Created ISL: Sat " << sat << " ↔ Sat " << neighbor
                << " (distance: " << distance / 1000.0 << " km, delay: "
                << delay.GetMilliSeconds() << " ms)");
        }
    }

    NS_LOG_INFO("Created " << createdLinks.size() << " ISL links ("
        << allIslDevices.GetN() << " devices)");

    return allIslDevices;
}

Ipv4InterfaceContainer IslNetworkCreator::AssignIslAddresses(const NetDeviceContainer& islDevices) {
    NS_LOG_FUNCTION(this);

    Ipv4AddressHelper address;
    Ipv4InterfaceContainer interfaces;

    // Assign IP addresses to each ISL link
    // Use 10.x.x.x range for ISL network
    // Each link gets its own /30 subnet (4 addresses: network, 2 hosts, broadcast)
    uint32_t linkCount = islDevices.GetN() / 2; // Each link has 2 devices

    for (uint32_t i = 0; i < linkCount; ++i) {
        // Each link gets its own /30 subnet
        // Example: 10.0.0.0/30, 10.0.0.4/30, 10.0.0.8/30, etc.
        std::ostringstream subnet;
        subnet << "10." << (i / 64) << "." << ((i % 64) * 4) << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.252");

        // Assign addresses to this link's 2 devices
        NetDeviceContainer linkDevices;
        linkDevices.Add(islDevices.Get(i * 2));
        linkDevices.Add(islDevices.Get(i * 2 + 1));

        Ipv4InterfaceContainer linkInterfaces = address.Assign(linkDevices);
        interfaces.Add(linkInterfaces);

        NS_LOG_DEBUG("Assigned subnet " << subnet.str() << "/30 to link " << i);
    }

    NS_LOG_INFO("Assigned IP addresses to " << linkCount << " ISL links ("
        << interfaces.GetN() << " interfaces)");

    return interfaces;
}

void IslNetworkCreator::InstallStaticRoutes(NodeContainer satellites,
                                           const RoutingTables& routes,
                                           const Ipv4InterfaceContainer& islInterfaces) {
    NS_LOG_FUNCTION(this);

    Ipv4StaticRoutingHelper staticRoutingHelper;
    uint32_t totalRoutes = 0;

    // Step 1: Build mapping from (satA, satB) -> LOCAL interface index on satA
    // Key insight: Each satellite has local interfaces (0=loopback, 1-4=ISL links)
    // We need to map: "Which local interface on satA connects to satB?"
    std::map<std::pair<uint32_t, uint32_t>, std::pair<uint32_t, Ipv4Address>> linkToLocalInterface;

    for (uint32_t i = 0; i < islInterfaces.GetN(); i += 2) {
        // Get the two interfaces connected by this link
        // islInterfaces.Get(i) returns std::pair<Ptr<Ipv4>, uint32_t>
        Ptr<Ipv4> ipv4A = islInterfaces.Get(i).first;
        uint32_t interfaceIdxA = islInterfaces.Get(i).second;

        Ptr<Ipv4> ipv4B = islInterfaces.Get(i + 1).first;
        uint32_t interfaceIdxB = islInterfaces.Get(i + 1).second;

        Ptr<Node> nodeA = ipv4A->GetObject<Node>();
        Ptr<Node> nodeB = ipv4B->GetObject<Node>();

        uint32_t satA = nodeA->GetId();
        uint32_t satB = nodeB->GetId();

        Ipv4Address addrA = islInterfaces.GetAddress(i);     // IP address on satA's interface
        Ipv4Address addrB = islInterfaces.GetAddress(i + 1); // IP address on satB's interface

        // Store: (satA, satB) -> (local interface on satA, IP address on satA)
        linkToLocalInterface[{satA, satB}] = {interfaceIdxA, addrA};
        linkToLocalInterface[{satB, satA}] = {interfaceIdxB, addrB};

        NS_LOG_DEBUG("Link " << i/2 << ": Sat " << satA << " (interface " << interfaceIdxA
                     << ", " << addrA << ") ↔ Sat " << satB << " (interface " << interfaceIdxB
                     << ", " << addrB << ")");
    }

    // Step 2: Build mapping from satellite ID to ANY valid IP address on that satellite
    // This is used as the destination address for routing
    std::map<uint32_t, Ipv4Address> satelliteAddress;
    for (uint32_t sat = 0; sat < satellites.GetN(); ++sat) {
        Ptr<Ipv4> ipv4 = satellites.Get(sat)->GetObject<Ipv4>();
        // Use the first non-loopback interface's address
        if (ipv4->GetNInterfaces() > 1) {
            satelliteAddress[sat] = ipv4->GetAddress(1, 0).GetLocal();
        }
    }

    // Step 3: Install routes for each satellite
    for (uint32_t src = 0; src < satellites.GetN(); ++src) {
        Ptr<Node> srcNode = satellites.Get(src);
        Ptr<Ipv4> srcIpv4 = srcNode->GetObject<Ipv4>();
        Ptr<Ipv4StaticRouting> staticRouting = staticRoutingHelper.GetStaticRouting(srcIpv4);

        // Install routes to all other satellites
        for (uint32_t dst = 0; dst < satellites.GetN(); ++dst) {
            if (src == dst) continue;

            uint32_t nextHop = routes.GetNextHop(src, dst);
            if (nextHop == UINT32_MAX) {
                NS_LOG_WARN("No route from Sat " << src << " to Sat " << dst);
                continue;
            }

            // Get destination satellite's IP address
            auto dstAddrIt = satelliteAddress.find(dst);
            if (dstAddrIt == satelliteAddress.end()) {
                NS_LOG_WARN("No IP address found for Sat " << dst);
                continue;
            }
            Ipv4Address dstAddr = dstAddrIt->second;

            // Find the LOCAL interface on src that connects to nextHop
            auto linkIt = linkToLocalInterface.find({src, nextHop});
            if (linkIt == linkToLocalInterface.end()) {
                NS_LOG_WARN("No interface found for link Sat " << src << " → Sat " << nextHop);
                continue;
            }

            uint32_t localInterface = linkIt->second.first;  // Local interface index on src

            // Get the gateway (next-hop IP address)
            // Gateway is the IP address on the nextHop satellite's side of the link
            auto gatewayIt = linkToLocalInterface.find({nextHop, src});
            if (gatewayIt == linkToLocalInterface.end()) {
                NS_LOG_WARN("No gateway found for reverse link Sat " << nextHop << " → Sat " << src);
                continue;
            }
            Ipv4Address gateway = gatewayIt->second.second;

            // Add route: destination host, gateway, local interface index
            staticRouting->AddHostRouteTo(dstAddr, gateway, localInterface);

            totalRoutes++;

            NS_LOG_DEBUG("Route: Sat " << src << " → Sat " << dst << " via Sat " << nextHop
                << " (local_if=" << localInterface << ", gateway=" << gateway << ", dst=" << dstAddr << ")");
        }
    }

    NS_LOG_INFO("Installed " << totalRoutes << " static routes across "
        << satellites.GetN() << " satellites");

    // Step 4: Dump routing tables for verification (only if NS_LOG enabled)
    if (g_log.IsEnabled(ns3::LOG_DEBUG)) {
        NS_LOG_DEBUG("\n=== Routing Table Dump ===");
        for (uint32_t src = 0; src < satellites.GetN(); ++src) {
            Ptr<Ipv4> ipv4 = satellites.Get(src)->GetObject<Ipv4>();
            Ptr<Ipv4StaticRouting> sr = staticRoutingHelper.GetStaticRouting(ipv4);

            NS_LOG_DEBUG("Sat " << src << " has " << sr->GetNRoutes() << " routes:");
            for (uint32_t j = 0; j < sr->GetNRoutes() && j < 5; ++j) { // Show first 5 routes
                Ipv4RoutingTableEntry entry = sr->GetRoute(j);
                NS_LOG_DEBUG("  " << entry.GetDest() << " via " << entry.GetGateway()
                             << " on interface " << entry.GetInterface());
            }
        }
    }
}

double IslNetworkCreator::ComputeSatelliteDistance(Ptr<Node> sat1, Ptr<Node> sat2) {
    NS_LOG_FUNCTION(this);

    // Get satellite positions from MobilityModel
    Ptr<MobilityModel> mob1 = sat1->GetObject<MobilityModel>();
    Ptr<MobilityModel> mob2 = sat2->GetObject<MobilityModel>();

    NS_ASSERT_MSG(mob1 && mob2, "Satellites must have mobility models");

    Vector pos1 = mob1->GetPosition(); // TEME coordinates (meters)
    Vector pos2 = mob2->GetPosition();

    // Euclidean distance in 3D space
    double dx = pos2.x - pos1.x;
    double dy = pos2.y - pos1.y;
    double dz = pos2.z - pos1.z;

    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);

    return distance;
}

Time IslNetworkCreator::ComputePropagationDelay(double distance_m) {
    NS_LOG_FUNCTION(this << distance_m);

    const double SPEED_OF_LIGHT = 299792458.0; // m/s
    double delay_s = distance_m / SPEED_OF_LIGHT;

    return Seconds(delay_s);
}

} // namespace ns3
