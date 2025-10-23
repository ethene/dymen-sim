/**
 * DyMeN-Sim: Unified Simulation Framework (Dual-Layer LEO+Mesh)
 *
 * NC9/NC10 Reproducibility Package
 *
 * Factory-based protocol selection for ISL + ground routing.
 * Supports multiple routing protocols via command-line:
 * - ISL protocols: static, olsr
 * - Ground protocols: aodv, olsr, dsdv
 *
 * Usage:
 *   # Dual-layer (24 satellites + 20 mobile mesh nodes)
 *   ./build/unified-simulation --isl-routing=static --ground-routing=aodv --time=60 --seed=1
 *   ./build/unified-simulation --isl-routing=olsr --ground-routing=dsdv --time=60 --seed=1
 *
 *   # Satellite-only mode (NC9 alpha coefficient measurement)
 *   ./build/unified-simulation --satellite-only=true --time=60 --seed=1
 *
 *   # Ground-only mode (NC9 beta/gamma measurement, NC10 control experiment)
 *   ./build/unified-simulation --ground-only=true --ground-routing=aodv --time=60 --seed=1
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "routing-protocol-factory.h"
#include "isl-topology-generator.h"
#include "isl-network-creator.h"
#include "static-isl-routing.h"
#include "manhattan-mobility-helper.h"
#include "packet-tracer.h"
#include <fstream>
#include <iomanip>
#include <chrono>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UnifiedSimulation");

int main(int argc, char *argv[]) {
    // Parse command-line arguments
    std::string islRouting = "static";
    std::string groundRouting = "aodv";
    uint32_t satellites = 24;
    uint32_t groundNodes = 20;
    double groundArea = 10000.0;  // 10 km radius
    double groundSpeed = 1.4;     // 1.4 m/s pedestrian
    std::string groundMobility = "waypoint";  // Week 24: mobility mode
    double groundPause = 2.0;     // Week 24: pause time at waypoints
    double groundBounds = 500.0;  // Week 24: ground area bounds (m)
    uint32_t manhattanBlocks = 5;  // Week 26: Manhattan grid size (5×5 blocks)
    double manhattanBlockSize = 100.0;  // Week 26: Manhattan block size (100m)
    double simTime = 60.0;
    uint32_t seed = 1;
    bool satelliteOnly = false;  // Week 28: Satellite-only mode (no ground layer)
    bool groundOnly = false;     // Week 28: Ground-only mode (no satellite layer)
    std::string outputFile = "results/unified_output.csv";

    CommandLine cmd;
    cmd.AddValue("isl-routing", "ISL protocol (static|olsr|aodv)", islRouting);
    cmd.AddValue("ground-routing", "Ground protocol (aodv|olsr|dsdv)", groundRouting);
    cmd.AddValue("satellites", "Number of satellites", satellites);
    cmd.AddValue("ground-nodes", "Number of ground mesh nodes", groundNodes);
    cmd.AddValue("ground-area", "Ground area radius (m)", groundArea);
    cmd.AddValue("ground-speed", "Ground node speed (m/s)", groundSpeed);
    cmd.AddValue("ground-mobility", "Ground mobility model (static|waypoint|manhattan)", groundMobility);
    cmd.AddValue("ground-pause", "Pause time at waypoints (seconds)", groundPause);
    cmd.AddValue("ground-bounds", "Ground area bounds (m, square area)", groundBounds);
    cmd.AddValue("manhattan-blocks", "Manhattan grid size (N×N blocks)", manhattanBlocks);
    cmd.AddValue("manhattan-block-size", "Manhattan block size (meters)", manhattanBlockSize);
    cmd.AddValue("time", "Simulation time (s)", simTime);
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("satellite-only", "Run satellite-only mode (no ground layer)", satelliteOnly);
    cmd.AddValue("ground-only", "Run ground-only mode (no satellite layer)", groundOnly);
    cmd.AddValue("output", "Output CSV file", outputFile);
    cmd.Parse(argc, argv);

    // Validate mode exclusivity
    if (satelliteOnly && groundOnly) {
        std::cerr << "ERROR: Cannot use both --satellite-only and --ground-only flags\n";
        return 1;
    }

    // Validate simulation time (applications start at t=20s, stop at t=simTime-10s)
    // Required: start time (20s) + minimum traffic duration (30s) + buffer (5s) = 55s
    const double CONVERGENCE_TIME = 20.0;  // Time for routing protocol convergence
    const double MIN_TRAFFIC_DURATION = 30.0;  // Minimum traffic duration
    const double END_BUFFER = 10.0;  // Buffer before simulation end
    const double MIN_SIM_TIME = CONVERGENCE_TIME + MIN_TRAFFIC_DURATION + END_BUFFER;

    if (simTime < MIN_SIM_TIME) {
        std::cerr << "ERROR: simTime (" << simTime << "s) is too short for traffic generation!\n";
        std::cerr << "       Minimum required: " << MIN_SIM_TIME << "s\n";
        std::cerr << "       Breakdown: " << CONVERGENCE_TIME << "s convergence + "
                  << MIN_TRAFFIC_DURATION << "s traffic + " << END_BUFFER << "s buffer\n";
        std::cerr << "\n";
        std::cerr << "       Applications start at t=" << CONVERGENCE_TIME << "s\n";
        std::cerr << "       Applications stop at t=" << (simTime - END_BUFFER) << "s\n";
        std::cerr << "       Traffic duration would be: " << (simTime - END_BUFFER - CONVERGENCE_TIME) << "s (need >= " << MIN_TRAFFIC_DURATION << "s)\n";
        return 1;
    }

    // Auto-adjust node counts for isolation modes
    if (satelliteOnly && groundNodes > 0) {
        std::cout << "NOTE: Ignoring --ground-nodes parameter in satellite-only mode\n";
        groundNodes = 0;  // Force no ground nodes
    }
    if (groundOnly && satellites > 0) {
        std::cout << "NOTE: Ignoring --satellites parameter in ground-only mode\n";
        satellites = 0;  // Force no satellites
    }

    // Set RNG seed
    RngSeedManager::SetSeed(seed);

    std::cout << "\n=== Phase 4 Week 24: Unified Simulation Framework (Mobile Ground Layer) ===\n";
    std::cout << "ISL routing: " << islRouting << "\n";
    std::cout << "Ground routing: " << groundRouting << "\n";
    std::cout << "Satellites: " << satellites << "\n";
    std::cout << "Ground nodes: " << groundNodes << "\n";
    std::cout << "Ground mobility: " << groundMobility << "\n";
    if (groundMobility == "waypoint") {
        std::cout << "Ground bounds: " << groundBounds << "m × " << groundBounds << "m\n";
        std::cout << "Ground pause: " << groundPause << " seconds\n";
    } else if (groundMobility == "manhattan") {
        std::cout << "Manhattan grid: " << manhattanBlocks << "×" << manhattanBlocks << " blocks\n";
        std::cout << "Manhattan block size: " << manhattanBlockSize << " meters\n";
        std::cout << "Grid bounds: " << (manhattanBlocks * manhattanBlockSize) << "m × "
                  << (manhattanBlocks * manhattanBlockSize) << "m\n";
        std::cout << "Ground pause: " << groundPause << " seconds\n";
    }
    std::cout << "Ground speed: " << groundSpeed << " m/s\n";
    std::cout << "Sim time: " << simTime << " seconds\n";
    std::cout << "RNG seed: " << seed << "\n";
    std::cout << "Output: " << outputFile << "\n\n";

    // Step 1: Create satellites with constant positions (skip if ground-only mode)
    std::cout << "[1/9] Creating " << satellites << " satellites...\n";
    NodeContainer satNodes;
    if (!groundOnly) {
        satNodes.Create(satellites);
    }

    // Satellite positioning and ISL topology (skip if ground-only mode)
    IslTopology topology;
    if (!groundOnly) {
        // Use ConstantPositionMobilityModel (Walker-Delta 53:24/3/1)
        const double ORBIT_RADIUS = 6371000.0 + 550000.0; // Earth radius + 550 km altitude (meters)
        const double INCLINATION = 53.0 * M_PI / 180.0;
        const uint32_t NUM_PLANES = 3;
        const uint32_t SATS_PER_PLANE = 8;

        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

        for (uint32_t i = 0; i < satellites; ++i) {
        uint32_t plane = i / SATS_PER_PLANE;
        uint32_t idx = i % SATS_PER_PLANE;

        // Right Ascension of Ascending Node (RAAN)
        double raan = plane * (360.0 / NUM_PLANES) * M_PI / 180.0;

        // True Anomaly
        double trueAnomaly = idx * (360.0 / SATS_PER_PLANE) * M_PI / 180.0;

        // Convert to TEME coordinates
        double x = ORBIT_RADIUS * (std::cos(raan) * std::cos(trueAnomaly) -
            std::sin(raan) * std::sin(trueAnomaly) * std::cos(INCLINATION));
        double y = ORBIT_RADIUS * (std::sin(raan) * std::cos(trueAnomaly) +
            std::cos(raan) * std::sin(trueAnomaly) * std::cos(INCLINATION));
        double z = ORBIT_RADIUS * std::sin(trueAnomaly) * std::sin(INCLINATION);

        positionAlloc->Add(Vector(x, y, z));
    }

        mobility.SetPositionAllocator(positionAlloc);
        mobility.Install(satNodes);

        std::cout << "  ✓ Satellites positioned in Walker-Delta 53:24/3/1\n";

        // Step 2: Generate ISL topology (before installing routing)
        std::cout << "[2/9] Generating ISL topology (4 neighbors per satellite)...\n";
        topology = GenerateWalkerDeltaTopology(satellites, 4);
        std::cout << "  ✓ ISL topology: " << topology.numSatellites << " satellites, "
            << topology.numLinks << " bidirectional links\n";
    }

    // Step 2a: Create ground nodes (if enabled and not satellite-only mode)
    NodeContainer meshNodes;
    if (groundNodes > 0 && !satelliteOnly) {
        std::cout << "[2a/12] Creating " << groundNodes << " ground mesh nodes...\n";
        meshNodes.Create(groundNodes);

        // Install mobility (conditional on groundMobility parameter)
        MobilityHelper meshMobility;

        if (groundMobility == "static") {
            // Week 23 baseline: Static grid layout
            meshMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                              "MinX", DoubleValue(0.0),
                                              "MinY", DoubleValue(0.0),
                                              "DeltaX", DoubleValue(100.0),
                                              "DeltaY", DoubleValue(100.0),
                                              "GridWidth", UintegerValue(5),
                                              "LayoutType", StringValue("RowFirst"));
            meshMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
            meshMobility.Install(meshNodes);
            std::cout << "  ✓ Ground nodes: static positions (grid layout)\n";
        } else if (groundMobility == "waypoint") {
            // Week 24: RandomWaypoint mobility
            // Random initial positions within bounds
            std::ostringstream boundsStr;
            boundsStr << "ns3::UniformRandomVariable[Min=0|Max=" << groundBounds << "]";

            meshMobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                              "X", StringValue(boundsStr.str()),
                                              "Y", StringValue(boundsStr.str()));

            // Create waypoint position allocator (for target destinations)
            ObjectFactory posFactory;
            posFactory.SetTypeId("ns3::RandomRectanglePositionAllocator");
            posFactory.Set("X", StringValue(boundsStr.str()));
            posFactory.Set("Y", StringValue(boundsStr.str()));
            Ptr<PositionAllocator> waypointAllocator = posFactory.Create()->GetObject<PositionAllocator>();

            // Configure mobility parameters
            std::ostringstream speedStr;
            speedStr << "ns3::ConstantRandomVariable[Constant=" << groundSpeed << "]";

            std::ostringstream pauseStr;
            pauseStr << "ns3::ConstantRandomVariable[Constant=" << groundPause << "]";

            meshMobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                          "Speed", StringValue(speedStr.str()),
                                          "Pause", StringValue(pauseStr.str()),
                                          "PositionAllocator", PointerValue(waypointAllocator));
            meshMobility.Install(meshNodes);
            std::cout << "  ✓ Ground nodes: RandomWaypoint mobility"
                      << " (speed=" << groundSpeed << " m/s, pause=" << groundPause << "s)\n";
        } else if (groundMobility == "manhattan") {
            // Week 26: Manhattan Grid mobility
            // Create Manhattan Grid helper
            ManhattanGridHelper grid(manhattanBlocks, manhattanBlockSize, groundBounds);
            std::vector<Vector> intersections = grid.GetIntersections();

            std::cout << "  Manhattan Grid: " << manhattanBlocks << "×" << manhattanBlocks
                      << " blocks, " << intersections.size() << " intersections\n";

            // Set initial positions at random intersections
            Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
            for (uint32_t i = 0; i < groundNodes; i++) {
                Vector pos = grid.GetRandomIntersection();
                positionAlloc->Add(pos);
            }

            // Use WaypointMobilityModel with grid-constrained waypoints
            meshMobility.SetPositionAllocator(positionAlloc);
            meshMobility.SetMobilityModel("ns3::WaypointMobilityModel");
            meshMobility.Install(meshNodes);

            // Schedule waypoints for each node (pre-compute path)
            double currentTime = 0.0;
            uint32_t waypointsPerNode = static_cast<uint32_t>((simTime - 20.0) /
                (grid.GetBlockSize() / groundSpeed + groundPause)) + 1;
            waypointsPerNode = std::min(waypointsPerNode, 20u);  // Cap at 20 waypoints

            for (uint32_t i = 0; i < groundNodes; i++) {
                Ptr<WaypointMobilityModel> waypoint =
                    meshNodes.Get(i)->GetObject<WaypointMobilityModel>();

                // Schedule random waypoints on grid intersections
                double nodeTime = 20.0;  // Start movement after convergence
                for (uint32_t w = 0; w < waypointsPerNode; w++) {
                    Vector target = grid.GetRandomIntersection();
                    Vector current = waypoint->GetPosition();

                    // Calculate travel time based on distance and speed
                    double distance = std::sqrt(std::pow(target.x - current.x, 2) +
                                                std::pow(target.y - current.y, 2));
                    double travelTime = distance / groundSpeed;

                    nodeTime += travelTime;
                    waypoint->AddWaypoint(Waypoint(Time(Seconds(nodeTime)), target));

                    // Add pause time
                    nodeTime += groundPause;
                }
            }

            std::cout << "  ✓ Ground nodes: Manhattan Grid mobility"
                      << " (speed=" << groundSpeed << " m/s, pause=" << groundPause
                      << "s, " << waypointsPerNode << " waypoints/node)\n";
        } else {
            std::cout << "  ERROR: Unknown mobility model '" << groundMobility << "'\n";
            std::cout << "  Valid options: static, waypoint, manhattan\n";
            return 1;
        }
    }

    // Step 3: Create ISL protocol via factory (skip if ground-only mode)
    std::unique_ptr<RoutingProtocol> islProtocol;
    if (!groundOnly) {
        std::cout << "[3/" << (groundNodes > 0 ? "12" : "9") << "] Creating ISL routing protocol...\n";
        islProtocol = RoutingProtocolFactory::Create(islRouting);
        std::cout << "  ✓ ISL Protocol: " << islProtocol->GetName()
                  << " (category: " << islProtocol->GetCategory() << ")\n";
    }

    // Step 3a: Create ground protocol via factory (if ground layer enabled and not satellite-only)
    std::unique_ptr<RoutingProtocol> groundProtocol;
    if (groundNodes > 0 && !satelliteOnly) {
        std::cout << "[3a/12] Creating ground routing protocol...\n";
        groundProtocol = RoutingProtocolFactory::Create(groundRouting);
        std::cout << "  ✓ Ground Protocol: " << groundProtocol->GetName()
                  << " (category: " << groundProtocol->GetCategory() << ")\n";
    }

    // Step 3b: Create ground WiFi ad-hoc network BEFORE installing protocols
    // (Devices must exist before InternetStackHelper is installed)
    NetDeviceContainer groundDevices;
    Ipv4InterfaceContainer groundInterfaces;
    if (groundNodes > 0 && !satelliteOnly) {
        std::cout << "[3b/12] Creating ground WiFi ad-hoc network...\n";

        // WiFi physical layer with explicit propagation model
        YansWifiChannelHelper channel;
        channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
        channel.AddPropagationLoss("ns3::RangePropagationLossModel",
                                   "MaxRange", DoubleValue(200.0)); // 200m range (realistic 802.11n outdoor mesh)

        YansWifiPhyHelper phy;
        phy.SetChannel(channel.Create());

        // WiFi MAC layer (ad-hoc mode)
        WifiMacHelper mac;
        mac.SetType("ns3::AdhocWifiMac");

        // WiFi helper
        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211n);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HtMcs7"),
                                     "ControlMode", StringValue("HtMcs0"));

        groundDevices = wifi.Install(phy, mac, meshNodes);
        std::cout << "  ✓ Ground WiFi devices: " << groundDevices.GetN() << "\n";
    }

    // Step 4: Install ISL protocol (creates internet stack for satellites, skip if ground-only)
    if (!groundOnly) {
        std::cout << "[4/" << (groundNodes > 0 ? "12" : "9") << "] Installing ISL routing protocol...\n";
        NodeContainer emptyNodes;  // ISL protocol doesn't use ground nodes
        islProtocol->Install(satNodes, emptyNodes);
        std::cout << "  ✓ ISL routing protocol installed on " << satellites << " satellites\n";
    }

    // Step 4a: Install ground protocol (if ground layer enabled and not satellite-only)
    // (Must happen AFTER WiFi devices are created but BEFORE IP addresses are assigned)
    if (groundNodes > 0 && !satelliteOnly) {
        std::cout << "[4a/12] Installing ground routing protocol...\n";
        NodeContainer emptyIslNodes;  // Ground protocol doesn't use ISL nodes
        groundProtocol->Install(emptyIslNodes, meshNodes);
        std::cout << "  ✓ Ground routing protocol installed on " << groundNodes << " mesh nodes\n";
    }

    // Step 4b: Assign IP addresses to ground mesh
    // (Must happen AFTER InternetStackHelper is installed)
    if (groundNodes > 0 && !satelliteOnly) {
        std::cout << "[4b/12] Assigning IP addresses to ground mesh...\n";
        Ipv4AddressHelper groundAddress;
        groundAddress.SetBase("10.1.0.0", "255.255.0.0");
        groundInterfaces = groundAddress.Assign(groundDevices);
        std::cout << "  ✓ Ground IP addresses: " << groundInterfaces.GetN() << " (10.1.0.x)\n";
    }

    // ISL network creation (Steps 5-7, skip if ground-only mode)
    NetDeviceContainer islDevices;
    Ipv4InterfaceContainer islInterfaces;
    if (!groundOnly) {
        // Step 5: Create ISL mesh with PointToPoint links
        std::cout << "[5/9] Creating ISL mesh with distance-based delays...\n";
        IslNetworkCreator creator;
        islDevices = creator.CreateIslMesh(satNodes, topology);
        std::cout << "  ✓ ISL devices: " << islDevices.GetN() << " (48 links × 2 devices/link)\n";

        // Step 6: Assign IP addresses
        std::cout << "[6/9] Assigning IP addresses to ISL links...\n";
        islInterfaces = creator.AssignIslAddresses(islDevices);
        std::cout << "  ✓ ISL interfaces: " << islInterfaces.GetN() << "\n";

        // Step 7: Install routes (if static) or wait for convergence (if dynamic)
        std::cout << "[7/9] Route installation...\n";
        if (islRouting == "static") {
            // Static routing: compute and install routes
            RoutingTables routes = ComputeStaticRoutes(topology);
            creator.InstallStaticRoutes(satNodes, routes, islInterfaces);
            std::cout << "  ✓ Static routes computed and installed\n";
        } else {
            // Dynamic routing: OLSR/AODV will auto-discover routes
            std::cout << "  ✓ Dynamic routing will discover routes during simulation\n";
        }
    }

    // Step 8: Create test traffic (reuse from baselines)
    std::cout << "[8/9] Creating test traffic...\n";

    uint16_t basePort = 9;

    // Satellite traffic (skip if ground-only mode)
    if (!groundOnly) {
        // Test 1: Single-hop ISL (Sat 0 → Sat 1, direct neighbors)
        Ipv4Address sat0Addr = satNodes.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        Ipv4Address sat1Addr = satNodes.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        std::cout << "  Sat flow 1: " << sat0Addr << " → " << sat1Addr << " (port " << basePort << ")\n";
        OnOffHelper onoff1("ns3::UdpSocketFactory", InetSocketAddress(sat1Addr, basePort));
        onoff1.SetConstantRate(DataRate("10Mbps"));
        ApplicationContainer senderApps1 = onoff1.Install(satNodes.Get(0));
        std::cout << "    Installed " << senderApps1.GetN() << " sender apps\n";
        senderApps1.Start(Seconds(20.0));  // Start after convergence (if dynamic)
        senderApps1.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink1("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort));
        ApplicationContainer sinkApps1 = sink1.Install(satNodes.Get(1));
        sinkApps1.Start(Seconds(0.0));

        // Test 2: Multi-hop ISL (Sat 0 → Sat 23, diagonal opposite)
        Ipv4Address sat23Addr = satNodes.Get(23)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        OnOffHelper onoff2("ns3::UdpSocketFactory", InetSocketAddress(sat23Addr, basePort + 1));
        onoff2.SetConstantRate(DataRate("10Mbps"));
        ApplicationContainer senderApps2 = onoff2.Install(satNodes.Get(0));
        senderApps2.Start(Seconds(20.0));
        senderApps2.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink2("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + 1));
        ApplicationContainer sinkApps2 = sink2.Install(satNodes.Get(23));
        sinkApps2.Start(Seconds(0.0));
    }

    // Additional satellite flows for satellite-only mode (total 5 flows)
    if (satelliteOnly) {
        std::cout << "  Satellite-only mode: Adding 3 additional ISL flows (total 5)...\n";

        // Flow 3: Sat 3 → Sat 10
        Ipv4Address sat10Addr = satNodes.Get(10)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        OnOffHelper onoff3("ns3::UdpSocketFactory", InetSocketAddress(sat10Addr, basePort + 2));
        onoff3.SetConstantRate(DataRate("10Mbps"));
        ApplicationContainer satSenderApps3 = onoff3.Install(satNodes.Get(3));
        satSenderApps3.Start(Seconds(20.0));
        satSenderApps3.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink3("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + 2));
        ApplicationContainer satSinkApps3 = sink3.Install(satNodes.Get(10));
        satSinkApps3.Start(Seconds(0.0));

        // Flow 4: Sat 6 → Sat 13
        Ipv4Address sat13Addr = satNodes.Get(13)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        OnOffHelper onoff4("ns3::UdpSocketFactory", InetSocketAddress(sat13Addr, basePort + 3));
        onoff4.SetConstantRate(DataRate("10Mbps"));
        ApplicationContainer satSenderApps4 = onoff4.Install(satNodes.Get(6));
        satSenderApps4.Start(Seconds(20.0));
        satSenderApps4.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink4("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + 3));
        ApplicationContainer satSinkApps4 = sink4.Install(satNodes.Get(13));
        satSinkApps4.Start(Seconds(0.0));

        // Flow 5: Sat 9 → Sat 20
        Ipv4Address sat20Addr = satNodes.Get(20)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        OnOffHelper onoff5("ns3::UdpSocketFactory", InetSocketAddress(sat20Addr, basePort + 4));
        onoff5.SetConstantRate(DataRate("10Mbps"));
        ApplicationContainer satSenderApps5 = onoff5.Install(satNodes.Get(9));
        satSenderApps5.Start(Seconds(20.0));
        satSenderApps5.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink5("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + 4));
        ApplicationContainer satSinkApps5 = sink5.Install(satNodes.Get(20));
        satSinkApps5.Start(Seconds(0.0));
    }

    // Test 3: Ground mesh traffic (if ground layer enabled and not satellite-only mode)
    if (groundNodes > 0 && !satelliteOnly) {
        uint32_t destNode = groundNodes - 1;  // Last ground node

        // Get destination address from ground interface container
        Ipv4Address meshDestAddr = groundInterfaces.GetAddress(destNode);
        Ipv4Address meshSrcAddr = groundInterfaces.GetAddress(0);

        std::cout << "  Ground mesh flow: " << meshSrcAddr << " → " << meshDestAddr << "\n";

        OnOffHelper onoff3("ns3::UdpSocketFactory", InetSocketAddress(meshDestAddr, basePort + 2));
        onoff3.SetConstantRate(DataRate("1Mbps"));  // Lower rate for ground mesh
        ApplicationContainer senderApps3 = onoff3.Install(meshNodes.Get(0));
        senderApps3.Start(Seconds(20.0));
        senderApps3.Stop(Seconds(simTime - 10.0));

        PacketSinkHelper sink3("ns3::UdpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), basePort + 2));
        ApplicationContainer sinkApps3 = sink3.Install(meshNodes.Get(destNode));
        sinkApps3.Start(Seconds(0.0));

        // Flow 2: Node 5 → Node 14 (mid-range, tests different spatial region)
        if (groundNodes > 14) {
            Ipv4Address dest2 = groundInterfaces.GetAddress(14);
            Ipv4Address src2 = groundInterfaces.GetAddress(5);

            std::cout << "  Ground mesh flow 2: " << src2 << " → " << dest2 << "\n";

            OnOffHelper onoff4("ns3::UdpSocketFactory", InetSocketAddress(dest2, basePort + 3));
            onoff4.SetConstantRate(DataRate("1Mbps"));
            ApplicationContainer senderApps4 = onoff4.Install(meshNodes.Get(5));
            senderApps4.Start(Seconds(20.0));
            senderApps4.Stop(Seconds(simTime - 10.0));

            PacketSinkHelper sink4("ns3::UdpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), basePort + 3));
            ApplicationContainer sinkApps4 = sink4.Install(meshNodes.Get(14));
            sinkApps4.Start(Seconds(0.0));
        }

        // Flow 3: Node 3 → Node 17 (random path)
        if (groundNodes > 17) {
            Ipv4Address dest3 = groundInterfaces.GetAddress(17);
            Ipv4Address src3 = groundInterfaces.GetAddress(3);

            std::cout << "  Ground mesh flow 3: " << src3 << " → " << dest3 << "\n";

            OnOffHelper onoff5("ns3::UdpSocketFactory", InetSocketAddress(dest3, basePort + 4));
            onoff5.SetConstantRate(DataRate("1Mbps"));
            ApplicationContainer senderApps5 = onoff5.Install(meshNodes.Get(3));
            senderApps5.Start(Seconds(20.0));
            senderApps5.Stop(Seconds(simTime - 10.0));

            PacketSinkHelper sink5("ns3::UdpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), basePort + 4));
            ApplicationContainer sinkApps5 = sink5.Install(meshNodes.Get(17));
            sinkApps5.Start(Seconds(0.0));
        }

        // Flow 4: Node 8 → Node 12 (random path)
        if (groundNodes > 12) {
            Ipv4Address dest4 = groundInterfaces.GetAddress(12);
            Ipv4Address src4 = groundInterfaces.GetAddress(8);

            std::cout << "  Ground mesh flow 4: " << src4 << " → " << dest4 << "\n";

            OnOffHelper onoff6("ns3::UdpSocketFactory", InetSocketAddress(dest4, basePort + 5));
            onoff6.SetConstantRate(DataRate("1Mbps"));
            ApplicationContainer senderApps6 = onoff6.Install(meshNodes.Get(8));
            senderApps6.Start(Seconds(20.0));
            senderApps6.Stop(Seconds(simTime - 10.0));

            PacketSinkHelper sink6("ns3::UdpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), basePort + 5));
            ApplicationContainer sinkApps6 = sink6.Install(meshNodes.Get(12));
            sinkApps6.Start(Seconds(0.0));
        }

        // Flow 5: Node 2 → Node 18 (random path)
        if (groundNodes > 18) {
            Ipv4Address dest5 = groundInterfaces.GetAddress(18);
            Ipv4Address src5 = groundInterfaces.GetAddress(2);

            std::cout << "  Ground mesh flow 5: " << src5 << " → " << dest5 << "\n";

            OnOffHelper onoff7("ns3::UdpSocketFactory", InetSocketAddress(dest5, basePort + 6));
            onoff7.SetConstantRate(DataRate("1Mbps"));
            ApplicationContainer senderApps7 = onoff7.Install(meshNodes.Get(2));
            senderApps7.Start(Seconds(20.0));
            senderApps7.Stop(Seconds(simTime - 10.0));

            PacketSinkHelper sink7("ns3::UdpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), basePort + 6));
            ApplicationContainer sinkApps7 = sink7.Install(meshNodes.Get(18));
            sinkApps7.Start(Seconds(0.0));
        }
    }

    std::cout << "  ✓ Test traffic configured:\n";
    std::cout << "    - Sat 0 → Sat 1 (1-hop ISL, 10 Mbps UDP)\n";
    std::cout << "    - Sat 0 → Sat 23 (5-hop ISL, 10 Mbps UDP)\n";
    if (groundNodes > 0) {
        std::cout << "    - Mesh flows: 5 random pairs (multi-hop ground, 1 Mbps UDP each)\n";
    }
    std::cout << "  ✓ Traffic starts at t=20s (allows convergence for dynamic protocols)\n";
    std::cout << "\n=== DIAGNOSTIC: Application Install Time ===\n";
    std::cout << "  Current simulation time: " << Simulator::Now().GetSeconds() << "s\n";

    // Step 9: Install FlowMonitor
    std::cout << "\n[9/9] Installing FlowMonitor...\n";
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    std::cout << "  ✓ FlowMonitor installed (using InstallAll())\n";

    std::cout << "\n=== DIAGNOSTIC: FlowMonitor Install Time ===\n";
    std::cout << "  Current simulation time: " << Simulator::Now().GetSeconds() << "s\n";

    // Phase 6 Week 27: Install PacketTracer for NRL metrics (ground layer only)
    // Note: HWMP excluded from NRL tracking (FlowMonitor incompatibility already excludes it from final experiments)
    PacketTracer tracer;
    if (groundNodes > 0 && groundRouting != "hwmp") {
        tracer.Install(groundDevices);
        std::cout << "  ✓ PacketTracer installed on " << groundDevices.GetN() << " ground devices\n";
    }


    // Log initial and final positions to verify movement (waypoint mode only)
    if (groundNodes > 0 && !satelliteOnly && groundMobility == "waypoint") {
        std::cout << "\n=== Initial Ground Node Positions (t=0) ===\n";
        for (uint32_t i = 0; i < std::min(5u, groundNodes); ++i) {
            Ptr<MobilityModel> mob = meshNodes.Get(i)->GetObject<MobilityModel>();
            Vector pos = mob->GetPosition();
            std::cout << "  Node " << i << ": (" << std::fixed << std::setprecision(2)
                      << pos.x << ", " << pos.y << ")\n";
        }

        // Schedule position check at end of simulation
        Simulator::Schedule(Seconds(simTime - 0.1), [&meshNodes, groundNodes, simTime]() {
            std::cout << "\n=== Final Ground Node Positions (t="
                      << std::fixed << std::setprecision(1) << (simTime - 0.1) << ") ===\n";
            for (uint32_t i = 0; i < std::min(5u, groundNodes); ++i) {
                Ptr<MobilityModel> mob = meshNodes.Get(i)->GetObject<MobilityModel>();
                Vector pos = mob->GetPosition();
                std::cout << "  Node " << i << ": (" << std::fixed << std::setprecision(2)
                          << pos.x << ", " << pos.y << ")\n";
            }
        });
    }

    // Debug: Schedule event to check application status and routing tables
    Simulator::Schedule(Seconds(21.0), [&meshNodes, groundNodes, groundRouting]() {
        std::cout << "\n=== DIAGNOSTIC: t=21s Application Status ===\n";

        // Check if applications exist on source nodes
        std::vector<uint32_t> sourceNodes = {0, 5, 3, 8, 2};
        for (size_t i = 0; i < sourceNodes.size() && i < 5; ++i) {
            uint32_t nodeId = sourceNodes[i];
            if (nodeId >= groundNodes) continue;

            Ptr<Node> node = meshNodes.Get(nodeId);
            uint32_t nApps = node->GetNApplications();
            std::cout << "  Node " << nodeId << ": " << nApps << " applications installed\n";

            for (uint32_t appIdx = 0; appIdx < nApps; ++appIdx) {
                Ptr<Application> app = node->GetApplication(appIdx);
                std::cout << "    App " << appIdx << " installed (type unknown)\n";
            }
        }

        // Check routing table on Node 0 (first source)
        if (groundNodes > 0) {
            std::cout << "\n  Node 0 routing table:\n";
            Ptr<Ipv4> ipv4 = meshNodes.Get(0)->GetObject<Ipv4>();
            Ptr<Ipv4RoutingProtocol> routing = ipv4->GetRoutingProtocol();

            // Print routing table
            Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
            routing->PrintRoutingTable(stream);
        }
    });

    // Run simulation
    std::cout << "\nRunning simulation for " << simTime << " seconds...\n";
    std::cout << "\n=== DIAGNOSTIC: Simulation Start Time ===\n";
    std::cout << "  Current simulation time: " << Simulator::Now().GetSeconds() << "s\n";
    auto startTime = std::chrono::high_resolution_clock::now();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

    std::cout << "  ✓ Simulation complete (runtime: " << duration << " seconds)\n\n";

    // Analyze results
    std::cout << "=== Analyzing Results ===\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::cout << "[DEBUG] FlowMonitor found " << stats.size() << " flows\n";

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    double totalDelay = 0.0;

    for (auto const& [flowId, flowStats] : stats) {
        totalTxPackets += flowStats.txPackets;
        totalRxPackets += flowStats.rxPackets;
        if (flowStats.rxPackets > 0) {
            totalDelay += flowStats.delaySum.GetSeconds();
        }

        // Per-flow details
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);
        double flowPdr = (flowStats.txPackets > 0) ?
            (100.0 * flowStats.rxPackets / flowStats.txPackets) : 0.0;
        double flowDelay = (flowStats.rxPackets > 0) ?
            (flowStats.delaySum.GetSeconds() / flowStats.rxPackets * 1000.0) : 0.0;

        std::cout << "Flow " << flowId << ": " << t.sourceAddress << " → " << t.destinationAddress
            << "\n  TX: " << flowStats.txPackets << ", RX: " << flowStats.rxPackets
            << ", PDR: " << std::fixed << std::setprecision(2) << flowPdr << "%"
            << ", Delay: " << flowDelay << " ms\n";
    }

    double pdr = (totalTxPackets > 0) ?
        (100.0 * totalRxPackets / totalTxPackets) : 0.0;
    double avgDelay = (totalRxPackets > 0) ?
        (totalDelay / totalRxPackets * 1000.0) : 0.0; // ms

    std::cout << "\n=== Aggregate Results ===\n";
    std::cout << "Packets TX: " << totalTxPackets << "\n";
    std::cout << "Packets RX: " << totalRxPackets << "\n";
    std::cout << "PDR: " << std::fixed << std::setprecision(2) << pdr << "%\n";
    std::cout << "Avg delay: " << avgDelay << " ms\n\n";

    // Export to CSV
    std::cout << "=== Exporting Results ===\n";
    std::ofstream csv(outputFile);
    csv << "metric,value\n";
    if (!groundOnly) {
        csv << "isl_routing," << islProtocol->GetName() << "\n";
        csv << "isl_category," << islProtocol->GetCategory() << "\n";
    }
    if (groundNodes > 0 && !satelliteOnly) {
        csv << "ground_routing," << groundProtocol->GetName() << "\n";
        csv << "ground_category," << groundProtocol->GetCategory() << "\n";
        csv << "ground_nodes," << groundNodes << "\n";
    }
    csv << "satellites," << satellites << "\n";
    csv << "sim_time," << simTime << "\n";
    csv << "seed," << seed << "\n";
    csv << "flows," << (groundNodes > 0 ? 7 : 2) << "\n";
    csv << "tx_packets," << totalTxPackets << "\n";
    csv << "rx_packets," << totalRxPackets << "\n";
    csv << "pdr," << pdr << "\n";
    csv << "avg_delay_ms," << avgDelay << "\n";
    csv << "runtime_seconds," << duration << "\n";

    // Phase 6 Week 27: Add NRL metrics (if ground layer enabled)
    if (groundNodes > 0) {
        uint64_t dataBytesTx = tracer.GetDataBytesTx();
        uint64_t controlBytesTx = tracer.GetControlBytesTx();
        double nrl = (dataBytesTx > 0) ? (double)controlBytesTx / dataBytesTx : 0.0;

        csv << "data_bytes_tx," << dataBytesTx << "\n";
        csv << "control_bytes_tx," << controlBytesTx << "\n";
        csv << "nrl," << std::fixed << std::setprecision(6) << nrl << "\n";

        std::cout << "\n=== NRL Metrics (Week 27) ===\n";
        std::cout << "Data bytes TX: " << dataBytesTx << "\n";
        std::cout << "Control bytes TX: " << controlBytesTx << "\n";
        std::cout << "NRL: " << std::fixed << std::setprecision(4) << nrl << "\n";
    }

    csv.close();

    std::cout << "  ✓ Results exported to: " << outputFile << "\n\n";

    Simulator::Destroy();

    // Validation (informational only - do not fail on metrics during experiments)
    std::cout << "=== Validation ===\n";

    if (pdr < 95.0) {
        std::cout << "⚠ NOTE: PDR " << pdr << "% < 95% target (data collection mode)\n";
    } else {
        std::cout << "✓ PASS: PDR " << pdr << "% >= 95% target\n";
    }

    if (avgDelay > 100.0) {
        std::cout << "⚠ NOTE: Avg delay " << avgDelay << " ms > 100 ms\n";
    } else {
        std::cout << "✓ PASS: Avg delay " << avgDelay << " ms <= 100 ms\n";
    }

    if (groundNodes > 0) {
        std::cout << "\n✓ Week 22 Day 3-4: Unified Simulation Framework (Dual-Layer) COMPLETE\n";
    } else {
        std::cout << "\n✓ Week 21 Day 4: Unified Simulation Framework (ISL-only) COMPLETE\n";
    }
    return 0;
}
