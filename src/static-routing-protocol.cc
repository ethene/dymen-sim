/**
 * Phase 4 Week 21 Day 2: Static Routing Protocol Implementation
 */

#include "static-routing-protocol.h"
#include "ns3/internet-stack-helper.h"

namespace ns3 {

StaticRoutingProtocol::StaticRoutingProtocol() {
    // Constructor
}

void StaticRoutingProtocol::Install(NodeContainer islNodes, NodeContainer groundNodes) {
    // For static routing, install basic internet stack (no routing helper)
    // Routes will be installed manually later via InstallStaticRoutes()

    InternetStackHelper internet;

    // Install on ISL nodes
    if (islNodes.GetN() > 0) {
        internet.Install(islNodes);
    }

    // Install on ground nodes
    if (groundNodes.GetN() > 0) {
        internet.Install(groundNodes);
    }
}

void StaticRoutingProtocol::SetParameter(std::string key, std::string value) {
    // Static routing has no configurable parameters
    // Ignore all parameter sets
}

std::string StaticRoutingProtocol::GetConfig() const {
    return "Static[no_parameters]";
}

} // namespace ns3
