/**
 * Phase 4 Week 23: DSDV Routing Protocol Implementation
 *
 * TDD GREEN Phase: Full implementation to pass all tests
 */

#include "dsdv-routing-protocol.h"
#include "ns3/internet-stack-helper.h"
#include <sstream>

namespace ns3 {

DsdvRoutingProtocol::DsdvRoutingProtocol()
    : m_periodicUpdateInterval(15.0),
      m_settlingTime(6.0) {
    // Constructor - initialize with DSDV defaults
    // PeriodicUpdateInterval: 15 seconds (NS-3 default)
    // SettlingTime: 6 seconds (NS-3 default)
}

void DsdvRoutingProtocol::Install(NodeContainer islNodes, NodeContainer groundNodes) {
    // Configure DSDV helper with parameters
    m_dsdvHelper.Set("PeriodicUpdateInterval", TimeValue(Seconds(m_periodicUpdateInterval)));
    m_dsdvHelper.Set("SettlingTime", TimeValue(Seconds(m_settlingTime)));

    // Install internet stack with DSDV routing on ISL nodes
    if (islNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_dsdvHelper);
        internet.Install(islNodes);
    }

    // Install internet stack with DSDV routing on ground nodes
    if (groundNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_dsdvHelper);
        internet.Install(groundNodes);
    }
}

uint64_t DsdvRoutingProtocol::GetControlBytes() const {
    // TODO: Will track control bytes in future work
    // For now, return 0 (consistent with other protocols)
    return 0;
}

void DsdvRoutingProtocol::SetParameter(std::string key, std::string value) {
    if (key == "periodic_update_interval") {
        m_periodicUpdateInterval = std::stod(value);
    } else if (key == "settling_time") {
        m_settlingTime = std::stod(value);
    }
    // Ignore unknown parameters (consistent with other protocols)
}

std::string DsdvRoutingProtocol::GetConfig() const {
    std::ostringstream oss;
    oss << "DSDV[periodic_update_interval=" << m_periodicUpdateInterval
        << ",settling_time=" << m_settlingTime << "]";
    return oss.str();
}

} // namespace ns3
