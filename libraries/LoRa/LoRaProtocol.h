/** 
* Protocollo LoRaProtocol
* @file LoRaProtocol.h
*/

#ifndef LORA_PROTOCOL_H
#define LORA_PROTOCOL_H

#include <stdint.h>
#include "LoRa.h"
#include <SPI.h>
#include "../init_configs/mac_address.h"


#define PACKET_TYPE_MASK 124

#define PACKET_TYPE_ACK 0
#define PACKET_TYPE_NORM 128
#define PACKET_TYPE_REQUESTS_ACK 1

#define QOS_REQUEST_ACK 1

#define PACKET_SENDING_ERROR 0
#define SUCCESFUL_RESPONSE 1
#define HOST_UNREACHABLE_RESPONSE 2

#define ACK_WAITING_MILLIS 200 

const MacAddress BROADCAST = MacAddress((uint64_t)0x0000000000000000);

class Packet{
    public:
     MacAddress dest;
     MacAddress sender;
     uint8_t type;
	 uint8_t packetNumber;
	 uint8_t packetLength;
     char body[245];
	 
     Packet(MacAddress _dest, MacAddress _sender, uint8_t _type, uint8_t _packetNumber, char* _body, uint8_t _packetLength){
         dest = _dest;
         sender = _sender;
         type = _type;
		 packetNumber = _packetNumber;
         for(int a = 0; a < _packetLength; a++)
            body[a] = _body[a];
         packetLength = _packetLength;
	 }
	 
     Packet(){
        dest = MacAddress(-1); sender = BROADCAST; type = -1; packetNumber = -1; packetLength = 0;
     }

    bool operator == ( const Packet &rhs) {
            if(packetLength != rhs.packetLength)
                return false;
            for(int a = 0; a < packetLength; a++)
                if(body[a] != rhs.body[a])
                    return false;

            return dest == rhs.dest && sender == rhs.sender && type == rhs.type && packetNumber == rhs.packetNumber;
	}
	
	
    bool operator != (const Packet& rhs){return !(*this == rhs);}

	bool isAck() {
		return (type & 128) == PACKET_TYPE_ACK;
	}
	bool requestsAck() {
		return (type & 3) == QOS_REQUEST_ACK;
	}
	bool isUninitialized(){
		return dest.value == -1;
	}
	void printPacket(){
		Serial.print("dest: ");
		Serial.println(dest.toString());
		Serial.print("sender: ");
		Serial.println(sender.toString());
		Serial.print("type: ");
		Serial.println(type);
		if((type & 128) == 128){
			Serial.println("Message");
		}else{
			Serial.println("ACK");
		}
		uint8_t packType = type & PACKET_TYPE_MASK;
		Serial.print("packetType: ");
		Serial.println(packType & PACKET_TYPE_MASK);
		Serial.print("packetNumber: ");
		Serial.println(packetNumber);
		Serial.print("packetLength: ");
		Serial.println(packetLength);
		Serial.println("-----------------");
	}
};

typedef struct Helpers {
	static uint32_t read32bitInt(uint8_t bytes[]) {
		int shifter = 24;
		uint32_t result = 0;
		for (int a = 0; a < 4; a++) {
			result |= (((uint32_t)bytes[a]) << shifter);
			shifter -= 8;
		}
		return result;
	}

	static uint64_t read48bitInt(uint8_t bytes[]) {
		int shifter = 40;
		uint64_t result = 0;
		for (int a = 0; a < 6; a++) {
			result |= (((uint64_t)bytes[a]) << shifter);
			shifter -= 8;
		}
		return result;
	}

	static void write48bitIntToPacket(MacAddress address) {
		for(int a = 0; a < 6; a++){
			LoRa.write(address.bytes[a]);
		}
	}

	static void read6BytesInto(uint8_t buffer[]) {
		for (int a = 0; a < 6; a++)
			buffer[a] = LoRa.read();
	}
	static void printResponseMessage(int response_code) {
		switch (response_code) {
		case PACKET_SENDING_ERROR:
			Serial.println("There was an error sending the message");
			break;
		case HOST_UNREACHABLE_RESPONSE:
			Serial.println("Host is unreachable, it could be powered off or broken");
			break;
		case SUCCESFUL_RESPONSE:
			Serial.println("Operation completed succesfully");
			break;
		default:
			Serial.println("Unknown response code");
				break;
		}
	}

	static Packet* readInputPacket() {
		Packet* result = new Packet();
		uint8_t buffer[6];
		read6BytesInto(result->dest.bytes);
		read6BytesInto(result->sender.bytes);
		result->type = (uint8_t)LoRa.read();
		result->packetNumber = (uint8_t)LoRa.read();
		result->packetLength = (uint8_t)LoRa.read();
		return result;
	}

}Helpers;

typedef struct hasReceivedAckHolder {
	bool hasAck = false; 
	Packet ack = Packet();
}AckHolder;


typedef void(*functionCall)(Packet arg); 

extern uint8_t packetCounter;
extern functionCall subscribedFunction;
extern MacAddress myAddress;

void initLoRa(MacAddress _myAddress, int csPin, int resetPin, int irqPin); 

void checkIncoming();

int sendPacket(Packet packet);

void receivePacket(int packetSize);

void changeAddress(MacAddress newAddress);

void subscribeToReceivePacketEvent(functionCall function);


static Packet MessagePacket(MacAddress dest, MacAddress sender, char body[], uint8_t packetLength) {
	return Packet(dest, sender, PACKET_TYPE_NORM, packetCounter, body, packetLength);
}

static Packet MessageAckPacket(MacAddress dest, MacAddress sender, char body[], uint8_t packetLength) {
	return Packet(dest, sender, PACKET_TYPE_NORM | PACKET_TYPE_REQUESTS_ACK, packetCounter, body, packetLength);
}

static Packet AckPacket(MacAddress dest, MacAddress sender, uint8_t reponsePacketNumber) {
	return Packet(dest, sender, PACKET_TYPE_ACK, reponsePacketNumber, "", 0);
}

#endif