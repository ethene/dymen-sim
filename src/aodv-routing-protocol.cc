/**
 * Phase 4 Week 21 Day 4: AODV Routing Protocol Implementation
 */

#include "aodv-routing-protocol.h"
#include "ns3/internet-stack-helper.h"
#include <sstream>

namespace ns3 {

AodvRoutingProtocol::AodvRoutingProtocol()
    : m_rreqRetries(2),
      m_activeRouteTimeout(3.0) {
    // Constructor - initialize with defaults
}

void AodvRoutingProtocol::Install(NodeContainer islNodes, NodeContainer groundNodes) {
    // Install internet stack with AODV routing on ISL nodes
    if (islNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_aodvHelper);
        internet.Install(islNodes);
    }

    // Install internet stack with AODV routing on ground nodes
    if (groundNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_aodvHelper);
        internet.Install(groundNodes);
    }
}

uint64_t AodvRoutingProtocol::GetControlBytes() const {
    // TODO: Will track control bytes in Day 4
    // For now, return 0
    return 0;
}

void AodvRoutingProtocol::SetParameter(std::string key, std::string value) {
    if (key == "rreq_retries") {
        m_rreqRetries = std::stoi(value);
    } else if (key == "active_route_timeout") {
        m_activeRouteTimeout = std::stod(value);
    }
}

std::string AodvRoutingProtocol::GetConfig() const {
    std::ostringstream oss;
    oss << "AODV[rreq_retries=" << m_rreqRetries
        << ",active_route_timeout=" << m_activeRouteTimeout << "]";
    return oss.str();
}

} // namespace ns3
