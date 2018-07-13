#ifndef LOCATION_PROTOCOL_H
#define LOCATION_PROTOCOL_H

#include "LoRaProtocol.h"

#define LOCATION_SCAN_PACKET_LENGTH 3
#define PACKET_TYPE_LOCATION_SCAN 8

Packet locationScanPacket(MacAddress sender, MacAddress dest, float distance);
bool isLocationScanPacket(uint8_t type, uint8_t packetLength);

#endif