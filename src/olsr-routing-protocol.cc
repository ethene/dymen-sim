/**
 * Phase 4 Week 21 Day 3: OLSR Routing Protocol Implementation
 */

#include "olsr-routing-protocol.h"
#include "ns3/internet-stack-helper.h"
#include <sstream>

namespace ns3 {

OlsrRoutingProtocol::OlsrRoutingProtocol()
    : m_helloInterval(2.0),
      m_tcInterval(5.0),
      m_willingness(3) {
    // Constructor - initialize with defaults for LEO
    // Configure OlsrHelper with default parameters
    m_olsrHelper.Set("HelloInterval", TimeValue(Seconds(m_helloInterval)));
    m_olsrHelper.Set("TcInterval", TimeValue(Seconds(m_tcInterval)));
}

void OlsrRoutingProtocol::Install(NodeContainer islNodes, NodeContainer groundNodes) {
    // Reconfigure OlsrHelper with current parameters
    m_olsrHelper.Set("HelloInterval", TimeValue(Seconds(m_helloInterval)));
    m_olsrHelper.Set("TcInterval", TimeValue(Seconds(m_tcInterval)));

    // Install internet stack with OLSR routing on ISL nodes
    if (islNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_olsrHelper);
        internet.Install(islNodes);
    }

    // Install internet stack with OLSR routing on ground nodes
    if (groundNodes.GetN() > 0) {
        InternetStackHelper internet;
        internet.SetRoutingHelper(m_olsrHelper);
        internet.Install(groundNodes);
    }
}

uint64_t OlsrRoutingProtocol::GetControlBytes() const {
    // TODO: Will track control bytes in Day 3
    // For now, return 0
    return 0;
}

void OlsrRoutingProtocol::SetParameter(std::string key, std::string value) {
    if (key == "hello_interval") {
        m_helloInterval = std::stod(value);
    } else if (key == "tc_interval") {
        m_tcInterval = std::stod(value);
    } else if (key == "willingness") {
        m_willingness = static_cast<uint8_t>(std::stoi(value));
    }
}

std::string OlsrRoutingProtocol::GetConfig() const {
    std::ostringstream oss;
    oss << "OLSR[hello_interval=" << m_helloInterval
        << ",tc_interval=" << m_tcInterval
        << ",willingness=" << static_cast<int>(m_willingness) << "]";
    return oss.str();
}

} // namespace ns3
