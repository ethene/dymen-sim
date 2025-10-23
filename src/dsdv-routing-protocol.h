/**
 * Phase 4 Week 23: DSDV Routing Protocol
 *
 * Implements RoutingProtocol interface for DSDV (Destination-Sequenced Distance-Vector).
 * Wraps NS-3 DsdvHelper with unified interface.
 *
 * Key characteristics:
 * - Category: "proactive"
 * - Control overhead: Moderate (periodic table updates)
 * - Convergence: Fast (proactive maintenance)
 * - Algorithm: Distance-vector with sequence numbers
 */

#ifndef DSDV_ROUTING_PROTOCOL_H
#define DSDV_ROUTING_PROTOCOL_H

#include "routing-protocol.h"
#include "ns3/dsdv-module.h"

namespace ns3 {

/**
 * DSDV routing protocol implementation.
 *
 * Wraps NS-3 DsdvHelper with unified interface.
 */
class DsdvRoutingProtocol : public RoutingProtocol {
public:
    DsdvRoutingProtocol();
    ~DsdvRoutingProtocol() override = default;

    void Install(NodeContainer islNodes, NodeContainer groundNodes) override;
    std::string GetName() const override { return "DSDV"; }
    std::string GetCategory() const override { return "proactive"; }
    uint64_t GetControlBytes() const override;
    void SetParameter(std::string key, std::string value) override;
    std::string GetConfig() const override;

private:
    DsdvHelper m_dsdvHelper;

    // Protocol parameters (configurable)
    double m_periodicUpdateInterval;  // Seconds between full table updates
    double m_settlingTime;            // Time to wait before using new route
};

} // namespace ns3

#endif // DSDV_ROUTING_PROTOCOL_H
