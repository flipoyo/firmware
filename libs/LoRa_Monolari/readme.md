Written in 2026 by Nabil El Houssein (26elhousse)

**LoRa_Molonari Library (`v3.1.0`)**

This library (`LoRa_Molonari.cpp` and `LoRa_Molonari.hpp`) handles all the point-to-point LoRa communication between the dataloggers (`SLAVE`) and the relay (`MASTER`). Since raw LoRa doesn't have built-in connections or delivery guarantees like TCP, this library implements a custom mini-protocol on top of it: a 3-way handshake, packet numbering, XOR checksums, acknowledgements (`ACK`), and session closing (`FIN`).

**How a packet is structured over the air**

Every packet sent by `sendPacket()` has a 35-byte header followed by the text payload:
1. `checksum` (1 byte): An XOR of the packet number, the request type, and every character in the sender address, destination address, and payload.
2. `sender address` (16 bytes): The sender's `Address_sent` (`devEui` for a datalogger, `appEui` for the relay).
3. `destination address` (16 bytes): The target's `Address_waited` (`appEui` for a datalogger, or the specific datalogger's `devEui` for the relay).
4. `packetNumber` (1 byte): Tracks the packet index so the receiver can ignore duplicates if an `ACK` gets lost.
5. `requestType` (1 byte): Either `SYN` (`0xcc`), `ACK` (`0x33`), `DATA` (`0xc3`), or `FIN` (`0x3c`).
6. `payload` (variable length): The actual CSV measurement line (or control strings like `"SYN"` / `"FIN"`).

When `receivePacket()` hears a transmission, it checks two things before accepting it: `isValidDestination()` verifies that the packet is actually addressed to this Arduino (`recipient == Address_sent`) and comes from the expected peer (`sender == Address_waited`), and `calculateChecksum()` makes sure no bits were corrupted in flight.

**How a communication session works**

A full exchange between the relay and a datalogger goes through four steps:

1. **Switching targets (`setAddressWaited`)**: Because a single relay talks to multiple dataloggers one after another, the relay calls `lora.setAddressWaited(targetEui)` right before each time slot so it only accepts packets from the datalogger scheduled for that slot.
2. **The Handshake (`handshake`)**: The relay (`MASTER`) sends `"SYN"`, waits for the datalogger (`SLAVE`) to reply with `"SYN-ACK"`, and then sends back a burst of 10 `"SYN-ACK-ACK"` packets to make sure the datalogger hears at least one and starts sending data.
3. **Sending and Timestamping Data (`sendAllPacketsAndManageMemory` & `receiveAllPackets`)**:
   * The datalogger reads lines from its SD card and sends them one by one as `DATA` packets. For each line, it waits for the relay to reply with an `ACK` containing the exact same CSV line as the payload. Only when that `ACK` arrives does the datalogger pop the line from its queue and update `SDOffset` to `memory_successor`. If a packet fails after 10 retries, it aborts so `SDOffset` stays pointing at the unsent line for the next cycle.
   * On the relay side, `receiveAllPackets()` does something very important before pushing a received CSV line into `receiveQueue`: it calls `insertRelayReceptionDateTimeIntoLine()`, which injects the relay's current RTC date and time right after the 4th semicolon (`UI ; id ; sensor_date ; sensor_time ; relay_date ; relay_time ; v1...`). However, when the relay sends the `ACK` back to the datalogger, it sends the **original** unmodified `payload`, otherwise the datalogger's `payload == packet.flush` check would fail and it would retry forever!
4. **Closing the Session (`closeSession`)**: Once the queue is empty, the datalogger sends a `FIN` packet and waits for the relay to reply with a matching `FIN` (up to 15 retries).