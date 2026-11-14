# LoRa_Molonari

Arduino LoRa communication library for the Molonari sensor and relay firmware.
The library provides point-to-point packet transmission, a master/slave
handshake, acknowledgements, duplicate detection, and session termination.

## Requirements

- Arduino framework
- [LoRa](https://github.com/sandeepmistry/arduino-LoRa) library
- `Reader.hpp`, which provides the `memory_line` type used by the
  SD-offset-aware send method

## Packet format

Each packet uses the following layout:

```text
checksum (1 byte)
sender address (16 bytes)
destination address (16 bytes)
packet number (1 byte)
request type (1 byte)
payload (variable length)
```

Addresses are logical `String` values and are encoded in fixed-width 16-byte
fields. Shorter addresses are padded with zero bytes. Received addresses are
converted back to logical strings before destination validation and checksum
calculation. Addresses longer than 16 characters are rejected by
`sendPacket()`.

The checksum is a one-byte XOR of:

- Every character in the destination address.
- Every character in the sender address.
- The packet number.
- The request type.
- Every character in the payload.

The checksum is an integrity check, not authentication.

## Public API

### Construction and radio state

```cpp
LoraCommunication(
    long frequency,
    String Address_sent,
    String Address_waited,
    RoleType role
);

void LoraUpdateAttributes(
    long frequency,
    String Address_sent,
    String Address_waited,
    RoleType role
);

void startLoRa();
void stopLoRa();
bool isLoRaActive();
void setdesttodefault();
```

- `Address_sent` is this device's logical address.
- `Address_waited` is the expected peer address.
- `role` is `MASTER` or `SLAVE`.
- `startLoRa()` attempts to initialize the radio up to three times.
- `setdesttodefault()` changes the expected destination to the default
  broadcast value used by the existing protocol.

### Individual packets

```cpp
void sendPacket(
    uint8_t packetNumber,
    RequestType requestType,
    const String& payload
);

bool receivePacket(
    uint8_t& packetNumber,
    RequestType& requestType,
    String& payload
);

bool isValidDestination(
    const String& recipient,
    const String& dest,
    RequestType requestType
);

uint8_t calculateChecksum(
    const String& recipient,
    const String& dest,
    uint8_t packetNumber,
    RequestType requestType,
    const String& payload
);
```

`sendPacket()` requires the radio to be active. `receivePacket()` waits for a
packet for up to approximately one second. It rejects packets shorter than
16 bytes, validates the destination, and then validates the checksum.

The request types are:

```cpp
SYN  = 0xcc
ACK  = 0x33
DATA = 0xc3
FIN  = 0x3c
```

### Sessions

```cpp
bool handshake(uint8_t& shift);

uint8_t sendAllPacketsAndManageMemory(
    std::queue<memory_line>& sendQueue,
    unsigned long& initial_adress
);

uint8_t sendAllPackets(std::queue<String>& sendQueue);

int receiveAllPackets(std::queue<String>& receiveQueue);

bool closeSession(int lastPacket);
```

#### `handshake()`

- A `MASTER` sends `SYN`, waits for `SYN-ACK`, and sends repeated final
  acknowledgements.
- A `SLAVE` waits for `SYN`, sends `SYN-ACK`, and waits for the final
  acknowledgement.
- `shift` receives the packet number supplied by the peer during the
  handshake.

#### `sendAllPacketsAndManageMemory()`

This is the method used by the current sensor and relay firmware.

For each `memory_line`, it:

1. Sends a `DATA` packet.
2. Waits for an `ACK` containing the exact payload.
3. Removes the line from the queue only after that acknowledgement.
4. Advances `initial_adress` to the acknowledged line's successor address.

Each packet can be attempted up to ten times, with up to six acknowledgement
receives per attempt. The method returns the number of acknowledged packets.
If transmission ultimately fails, unacknowledged lines are not reflected by an
advance of `initial_adress`; the caller should use that offset to retry the
remaining SD data later.

#### `sendAllPackets()`

This queue-only method uses the same `DATA`/`ACK` exchange without SD offset
management. It is retained for compatibility but is not used by the current
sensor or relay application paths.

#### `receiveAllPackets()`

Waits for incoming packets for up to 60 seconds or until a `FIN` packet is
received. Data payloads are added to `receiveQueue`, and each accepted packet
is acknowledged. Duplicate packet numbers are acknowledged again without
adding a second payload. The queue is limited to `MAX_QUEUE_SIZE` (255
entries).

The return value is the last packet number observed, or the packet number from
the terminating condition.

#### `closeSession()`

Sends a `FIN` packet and waits for a matching `FIN` response. It retries up to
15 times before returning `false`.

## Configuration update API

The header also exposes:

```cpp
bool receiveConfigUpdate(
    const char* filepath,
    uint16_t* outMeasureInterval,
    uint16_t* outLoraInterval,
    unsigned long timeout_ms = 15000
);
```

This API is reserved for the downstream configuration-update workflow and is
not part of the normal packet/session documentation above.
