/**
 * Phase 4 Week 21: Routing Protocol Factory
 *
 * Factory for creating routing protocol instances by name.
 * Supports protocol creation by name (e.g., "aodv", "olsr", "static").
 *
 * Usage:
 *   auto protocol = RoutingProtocolFactory::Create("olsr");
 *   protocol->SetParameter("hello_interval", "2.0");
 *   protocol->Install(islNodes, groundNodes);
 */

#ifndef ROUTING_PROTOCOL_FACTORY_H
#define ROUTING_PROTOCOL_FACTORY_H

#include "routing-protocol.h"
#include "static-routing-protocol.h"
#include "olsr-routing-protocol.h"
#include "aodv-routing-protocol.h"
#include "dsdv-routing-protocol.h"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace ns3 {

/**
 * Factory for creating routing protocol instances.
 *
 * Supports protocol creation by name (case-insensitive).
 */
class RoutingProtocolFactory {
public:
    /**
     * Create routing protocol instance by name.
     *
     * Supported protocols:
     * - "static" -> StaticRoutingProtocol (ISL only)
     * - "olsr" -> OlsrRoutingProtocol (ISL or ground)
     * - "aodv" -> AodvRoutingProtocol (ground only)
     * - "dsdv" -> DsdvRoutingProtocol (ground only)
     *
     * @param name Protocol name (case-insensitive)
     * @return Unique pointer to protocol instance
     * @throws std::invalid_argument if protocol name unknown
     */
    static std::unique_ptr<RoutingProtocol> Create(std::string name) {
        // Convert to lowercase for case-insensitive comparison
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name == "static") {
            return std::make_unique<StaticRoutingProtocol>();
        } else if (name == "olsr") {
            return std::make_unique<OlsrRoutingProtocol>();
        } else if (name == "aodv") {
            return std::make_unique<AodvRoutingProtocol>();
        } else if (name == "dsdv") {
            return std::make_unique<DsdvRoutingProtocol>();
        } else {
            throw std::invalid_argument("Unknown protocol: " + name);
        }
    }

    /**
     * List all supported protocols.
     *
     * @return Vector of protocol names (lowercase)
     */
    static std::vector<std::string> GetSupportedProtocols() {
        return {"static", "olsr", "aodv", "dsdv"};
    }
};

} // namespace ns3

#endif // ROUTING_PROTOCOL_FACTORY_H
