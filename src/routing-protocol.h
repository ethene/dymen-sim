/**
 * Phase 4 Week 21: Routing Protocol Abstract Base Class
 *
 * Provides unified interface for protocol-agnostic simulation.
 * All routing protocols (Static, OLSR, AODV, HWMP, Babel) implement this interface.
 *
 * Design: Abstract base class with pure virtual methods
 * Usage: Subclass and implement all pure virtual methods
 *
 * Example:
 *   class StaticRoutingProtocol : public RoutingProtocol {
 *       void Install(...) override { ... }
 *       std::string GetName() const override { return "Static"; }
 *       ...
 *   };
 */

#ifndef ROUTING_PROTOCOL_H
#define ROUTING_PROTOCOL_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include <string>
#include <memory>

namespace ns3 {

/**
 * Abstract base class for all routing protocols.
 *
 * Provides unified interface for protocol-agnostic simulation harness.
 * Implementations handle protocol-specific NS-3 helper configuration.
 */
class RoutingProtocol {
public:
    /**
     * Virtual destructor (required for polymorphic base class)
     */
    virtual ~RoutingProtocol() = default;

    /**
     * Install routing protocol on node containers.
     *
     * This method configures and installs the routing protocol on the specified nodes.
     * For ISL protocols: Use islNodes, ignore groundNodes
     * For ground protocols: Use groundNodes, ignore islNodes
     * For hybrid protocols: Use both
     *
     * @param islNodes Satellite nodes (for ISL routing)
     * @param groundNodes Ground mesh nodes (for ground routing)
     */
    virtual void Install(NodeContainer islNodes, NodeContainer groundNodes) = 0;

    /**
     * Get protocol name.
     *
     * @return Protocol name (e.g., "AODV", "OLSR", "Static", "HWMP", "Babel")
     */
    virtual std::string GetName() const = 0;

    /**
     * Get protocol category.
     *
     * @return Category: "reactive", "proactive", "hybrid", or "static"
     */
    virtual std::string GetCategory() const = 0;

    /**
     * Get total control bytes transmitted.
     *
     * Used to compute Normalized Routing Load (NRL).
     * Static routing returns 0 (no control packets).
     * Dynamic protocols (OLSR, AODV) return sum of control packet bytes.
     *
     * @return Control packet bytes
     */
    virtual uint64_t GetControlBytes() const = 0;

    /**
     * Set protocol-specific parameter.
     *
     * Examples:
     *   - OLSR: SetParameter("hello_interval", "2.0")
     *   - AODV: SetParameter("rreq_retries", "2")
     *   - Static: SetParameter() is no-op
     *
     * @param key Parameter name (protocol-specific)
     * @param value Parameter value (as string, converted internally)
     */
    virtual void SetParameter(std::string key, std::string value) = 0;

    /**
     * Get protocol-specific configuration summary.
     *
     * Returns human-readable string describing current configuration.
     * Used for logging and debugging.
     *
     * Example: "OLSR[hello_interval=2.0,tc_interval=5.0,willingness=3]"
     *
     * @return Configuration string
     */
    virtual std::string GetConfig() const = 0;
};

} // namespace ns3

#endif // ROUTING_PROTOCOL_H
