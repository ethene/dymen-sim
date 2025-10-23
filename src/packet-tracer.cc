/**
 * Phase 6 Week 27: PacketTracer Implementation
 */

#include "packet-tracer.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"

namespace ns3 {

PacketTracer::PacketTracer()
    : m_controlBytesTx(0),
      m_controlBytesRx(0),
      m_dataBytesTx(0),
      m_dataBytesRx(0) {
    // Constructor - counters initialized to 0
}

void PacketTracer::Install(NetDeviceContainer devices) {
    // Hook into device trace sources at IP layer (after WiFi/LLC/SNAP headers removed)
    //
    // Strategy: Connect to Ipv4L3Protocol Send/Receive traces
    // This gives us packets at IP layer, making classification much simpler
    //
    // Note: This requires access to the node's Ipv4 object
    for (uint32_t i = 0; i < devices.GetN(); ++i) {
        Ptr<NetDevice> dev = devices.Get(i);
        Ptr<Node> node = dev->GetNode();

        // Get Ipv4 protocol object from node
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        if (ipv4) {
            Ptr<Ipv4L3Protocol> ipv4L3 = DynamicCast<Ipv4L3Protocol>(ipv4);
            if (ipv4L3) {
                // TX trace: Connect to Send (packet being sent from IP layer)
                ipv4L3->TraceConnectWithoutContext(
                    "Tx",
                    MakeCallback(&PacketTracer::TxCallback, this));

                // RX trace: Connect to Rx (packet received at IP layer)
                ipv4L3->TraceConnectWithoutContext(
                    "Rx",
                    MakeCallback(&PacketTracer::RxCallback, this));
            }
        }
    }
}

uint64_t PacketTracer::GetControlBytesTx() const {
    return m_controlBytesTx;
}

uint64_t PacketTracer::GetControlBytesRx() const {
    return m_controlBytesRx;
}

uint64_t PacketTracer::GetDataBytesTx() const {
    return m_dataBytesTx;
}

uint64_t PacketTracer::GetDataBytesRx() const {
    return m_dataBytesRx;
}

void PacketTracer::Reset() {
    m_controlBytesTx = 0;
    m_controlBytesRx = 0;
    m_dataBytesTx = 0;
    m_dataBytesRx = 0;
}

bool PacketTracer::IsDataPacket(Ptr<const Packet> packet) const {
    // At IP layer, packet structure is: [IPv4 Header][Payload (UDP/TCP/ICMP/etc)]
    // We need to extract IPv4 + UDP headers to check destination port

    // Create a copy for peeking (const packet cannot be modified)
    Ptr<Packet> copy = packet->Copy();

    // Extract IPv4 header
    Ipv4Header ipv4Header;
    if (copy->RemoveHeader(ipv4Header) == 0) {
        // No IPv4 header found (shouldn't happen at IP layer)
        return false;
    }

    // Check if it's UDP protocol
    if (ipv4Header.GetProtocol() != 17) {  // 17 = UDP
        // Not UDP -> control packet (could be ICMP, AODV, OLSR, etc.)
        return false;
    }

    // Extract UDP header
    UdpHeader udpHeader;
    if (copy->RemoveHeader(udpHeader) == 0) {
        // No UDP header found (malformed packet?)
        return false;
    }

    // Check destination port
    uint16_t destPort = udpHeader.GetDestinationPort();

    // Data packets: UDP port ∈ [9, 14]
    // (Application traffic from unified-simulation.cc uses ports 9-14)
    return (destPort >= 9 && destPort <= 14);
}

void PacketTracer::TxCallback(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
    // Note: We ignore ipv4 and interface parameters - only interested in packet
    uint32_t size = packet->GetSize();

    if (IsDataPacket(packet)) {
        m_dataBytesTx += size;
    } else {
        m_controlBytesTx += size;
    }
}

void PacketTracer::RxCallback(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface) {
    // Note: We ignore ipv4 and interface parameters - only interested in packet
    uint32_t size = packet->GetSize();

    if (IsDataPacket(packet)) {
        m_dataBytesRx += size;
    } else {
        m_controlBytesRx += size;
    }
}

} // namespace ns3
