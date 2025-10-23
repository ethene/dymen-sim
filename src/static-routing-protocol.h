/**
 * Phase 4 Week 21 Day 2: Static Routing Protocol
 *
 * Implements RoutingProtocol interface for static routing.
 * Pre-computes shortest paths using Dijkstra algorithm.
 * Installs static routes in NS-3 Ipv4StaticRouting tables.
 *
 * Key characteristics:
 * - Category: "static"
 * - Control bytes: 0 (no control packets)
 * - Convergence: Instant (pre-computed)
 * - Failures: No automatic rerouting (static routes)
 */

#ifndef STATIC_ROUTING_PROTOCOL_H
#define STATIC_ROUTING_PROTOCOL_H

#include "routing-protocol.h"
#include "ns3/ipv4-static-routing-helper.h"

namespace ns3 {

/**
 * Static routing protocol implementation.
 *
 * Pre-computes shortest paths using Dijkstra algorithm.
 * Installs static routes in NS-3 Ipv4StaticRouting tables.
 */
class StaticRoutingProtocol : public RoutingProtocol {
public:
    StaticRoutingProtocol();
    ~StaticRoutingProtocol() override = default;

    void Install(NodeContainer islNodes, NodeContainer groundNodes) override;
    std::string GetName() const override { return "Static"; }
    std::string GetCategory() const override { return "static"; }
    uint64_t GetControlBytes() const override { return 0; } // No control packets
    void SetParameter(std::string key, std::string value) override;
    std::string GetConfig() const override;
};

} // namespace ns3

#endif // STATIC_ROUTING_PROTOCOL_H
