/**
 * Phase 4 Week 21 Day 4: AODV Routing Protocol
 *
 * Implements RoutingProtocol interface for AODV (Ad-hoc On-Demand Distance Vector).
 * Wraps NS-3 AodvHelper with unified interface.
 *
 * Key characteristics:
 * - Category: "reactive"
 * - Control overhead: Low (only when discovering routes)
 * - Convergence: On-demand (1-3 seconds per route)
 * - Failures: Automatic rerouting via route discovery
 */

#ifndef AODV_ROUTING_PROTOCOL_H
#define AODV_ROUTING_PROTOCOL_H

#include "routing-protocol.h"
#include "ns3/aodv-module.h"

namespace ns3 {

/**
 * AODV routing protocol implementation.
 *
 * Wraps NS-3 AodvHelper with unified interface.
 */
class AodvRoutingProtocol : public RoutingProtocol {
public:
    AodvRoutingProtocol();
    ~AodvRoutingProtocol() override = default;

    void Install(NodeContainer islNodes, NodeContainer groundNodes) override;
    std::string GetName() const override { return "AODV"; }
    std::string GetCategory() const override { return "reactive"; }
    uint64_t GetControlBytes() const override;
    void SetParameter(std::string key, std::string value) override;
    std::string GetConfig() const override;

private:
    AodvHelper m_aodvHelper;

    // Protocol parameters (configurable)
    uint32_t m_rreqRetries;
    double m_activeRouteTimeout;
};

} // namespace ns3

#endif // AODV_ROUTING_PROTOCOL_H
