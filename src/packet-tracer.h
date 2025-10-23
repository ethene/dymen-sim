/**
 * Phase 6 Week 27: PacketTracer - Packet Classification and Byte Tracking
 *
 * Classifies packets as control vs data and tracks byte counts.
 * Used to compute Normalized Routing Load (NRL) metric.
 *
 * Classification logic:
 * - Data packets: UDP destination port ∈ [9, 14] (application traffic)
 * - Control packets: All other IP traffic (routing protocols AODV/OLSR/DSDV)
 *
 * Usage:
 *   PacketTracer tracer;
 *   tracer.Install(groundDevices);  // Hook into WiFi device trace sources
 *   ...
 *   uint64_t controlBytes = tracer.GetControlBytesTx();
 *   uint64_t dataBytes = tracer.GetDataBytesTx();
 *   double nrl = (dataBytes > 0) ? (double)controlBytes / dataBytes : 0.0;
 */

#ifndef PACKET_TRACER_H
#define PACKET_TRACER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

namespace ns3 {

/**
 * Packet tracer for NRL (Normalized Routing Load) computation.
 *
 * Hooks into NetDevice trace sources to classify and count packets.
 */
class PacketTracer {
public:
    /**
     * Constructor - initializes counters to 0.
     */
    PacketTracer();

    /**
     * Install packet tracer on devices.
     *
     * Hooks into PromiscSniffer trace source to capture all packets
     * (both TX and RX) on the specified devices.
     *
     * @param devices NetDeviceContainer to monitor
     */
    void Install(NetDeviceContainer devices);

    /**
     * Get total control packet bytes transmitted.
     *
     * Control packets: Routing protocol traffic (AODV/OLSR/DSDV).
     * Classification: IP packets with UDP dest port NOT in [9, 14].
     *
     * @return Total control bytes TX
     */
    uint64_t GetControlBytesTx() const;

    /**
     * Get total control packet bytes received.
     *
     * @return Total control bytes RX
     */
    uint64_t GetControlBytesRx() const;

    /**
     * Get total data packet bytes transmitted.
     *
     * Data packets: Application traffic (UDP dest port ∈ [9, 14]).
     *
     * @return Total data bytes TX
     */
    uint64_t GetDataBytesTx() const;

    /**
     * Get total data packet bytes received.
     *
     * @return Total data bytes RX
     */
    uint64_t GetDataBytesRx() const;

    /**
     * Reset all counters to 0.
     *
     * Used for multi-run simulations where counters need to be cleared
     * between runs.
     */
    void Reset();

private:
    /**
     * Check if packet is a data packet (application traffic).
     *
     * Classification logic:
     * 1. Extract UDP header from packet
     * 2. Check if destination port ∈ [9, 14]
     * 3. If yes -> data packet, else -> control packet
     *
     * @param packet Packet to classify
     * @return true if data packet, false if control packet
     */
    bool IsDataPacket(Ptr<const Packet> packet) const;

    /**
     * TX callback - called when packet is transmitted at IP layer.
     *
     * @param packet Transmitted packet
     * @param ipv4 IPv4 protocol instance
     * @param interface Interface index
     */
    void TxCallback(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface);

    /**
     * RX callback - called when packet is received at IP layer.
     *
     * @param packet Received packet
     * @param ipv4 IPv4 protocol instance
     * @param interface Interface index
     */
    void RxCallback(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface);

    // Byte counters
    uint64_t m_controlBytesTx;  ///< Control packet bytes transmitted
    uint64_t m_controlBytesRx;  ///< Control packet bytes received
    uint64_t m_dataBytesTx;     ///< Data packet bytes transmitted
    uint64_t m_dataBytesRx;     ///< Data packet bytes received
};

} // namespace ns3

#endif // PACKET_TRACER_H
