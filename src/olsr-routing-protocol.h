/**
 * Phase 4 Week 21 Day 3: OLSR Routing Protocol
 *
 * Implements RoutingProtocol interface for OLSR (Optimized Link State Routing).
 * Wraps NS-3 OlsrHelper with unified interface.
 * Supports parameter configuration (HELLO interval, TC interval, etc.)
 *
 * Key characteristics:
 * - Category: "proactive"
 * - Control overhead: ~15% (HELLO + TC messages)
 * - Convergence: 10-20 seconds
 * - Failures: Automatic rerouting
 */

#ifndef OLSR_ROUTING_PROTOCOL_H
#define OLSR_ROUTING_PROTOCOL_H

#include "routing-protocol.h"
#include "ns3/olsr-module.h"

namespace ns3 {

/**
 * OLSR routing protocol implementation.
 *
 * Wraps NS-3 OlsrHelper with unified interface.
 * Supports parameter configuration (HELLO interval, TC interval, etc.)
 */
class OlsrRoutingProtocol : public RoutingProtocol {
public:
    OlsrRoutingProtocol();
    ~OlsrRoutingProtocol() override = default;

    void Install(NodeContainer islNodes, NodeContainer groundNodes) override;
    std::string GetName() const override { return "OLSR"; }
    std::string GetCategory() const override { return "proactive"; }
    uint64_t GetControlBytes() const override;
    void SetParameter(std::string key, std::string value) override;
    std::string GetConfig() const override;

private:
    OlsrHelper m_olsrHelper;

    // Protocol parameters (configurable)
    double m_helloInterval; // seconds
    double m_tcInterval;    // seconds
    uint8_t m_willingness;  // MPR willingness (0-7)
};

} // namespace ns3

#endif // OLSR_ROUTING_PROTOCOL_H
