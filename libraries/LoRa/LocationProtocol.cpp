#include "LocationProtocol.h"

Packet locationScanPacket(MacAddress sender, MacAddress dest, float distance){
  char body[LOCATION_SCAN_PACKET_LENGTH];
  int16_t distInt = (int16_t) distance;
  body[0] = ((distInt & 0xFF00) >> 8);
  body[1] = ((distInt & 0x00FF) >> 0);
  distance -= distInt;
  distance *= 100;
  int8_t distFloat = (int8_t) distance;
  body[2] = distFloat;
  return Packet(dest,sender,PACKET_TYPE_NORM | PACKET_TYPE_LOCATION_SCAN , packetCounter,body,LOCATION_SCAN_PACKET_LENGTH); 
}

bool isLocationScanPacket(uint8_t type, uint8_t packetLength){
	return ((type & PACKET_TYPE_MASK) == PACKET_TYPE_LOCATION_SCAN) && packetLength == LOCATION_SCAN_PACKET_LENGTH;
}