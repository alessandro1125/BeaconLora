#include "LoRaProtocol.h"
#include <stdint.h>


#define PACKET_TYPE_TEMPERATURE_SCAN 4          /**< Valore dei bit 2-6 se il pacchetto invia un dato di temperatura*/
#define TEMPERATURE_SCAN_PACKET_LENGTH 3        /**< lunghezza del payload el pacchetto contenente la temperatura*/

Packet temperatureScanPacket(MacAddress sender, MacAddress dest, float temperature){
	char body[TEMPERATURE_SCAN_PACKET_LENGTH];
	int16_t tempInt = (int16_t) temperature;
	body[0] = ((tempInt & 0xFF00) >> 8);
    body[1] = ((tempInt & 0x00FF) >> 0);
	temperature -= tempInt;
	temperature *= 100;
	int8_t tempFloat = (int8_t) temperature;
	body[2] = tempFloat;
	return Packet(dest,sender,PACKET_TYPE_NORM | PACKET_TYPE_TEMPERATURE_SCAN , packetCounter,body,TEMPERATURE_SCAN_PACKET_LENGTH);
}

float decodeTemperatureFromPacket(char* data){
	int16_t temperatureInt = 0;
	temperatureInt |= (data[0] << 8);
	temperatureInt |= (data[1] << 0);
	float temperature = (float) data[2];
	temperature /= 100;
	temperature += temperatureInt;
	return temperature;
}

bool isTemperatureScanPacket(uint8_t type, uint8_t packetLength){
	return ((type & PACKET_TYPE_MASK) == PACKET_TYPE_TEMPERATURE_SCAN) && packetLength == TEMPERATURE_SCAN_PACKET_LENGTH;
}

