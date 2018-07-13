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


#define PACKET_TYPE_MASK 124 /**< Maschera per il tipo di pacchetto.  Estrapola i bit di tipo dal byte tipo della classe `Packet`, separandoli dai flag `TYPE` e `QoS`*/

#define PACKET_TYPE_ACK 0                       /**< Valore del primo bit se il pacchetto è un ack.*/
#define PACKET_TYPE_NORM 128                    /**< Valore del primo bit se il pacchetto non è un ack.*/
#define PACKET_TYPE_REQUESTS_ACK 1              /**< Valore degli ultimi 2 bit se il pacchetto richiede ACK in risposta*/

#define QOS_REQUEST_ACK 1                       /**< Come `PACKET_TYPE_REQUESTS_ACK`*/

#define PACKET_SENDING_ERROR 0                  /**< valore di return della funzione `sendPacket()` se non riesce ad inviare un pacchetto*/
#define SUCCESFUL_RESPONSE 1                    /**< valore di return della funzione `sendPacket()` se riesce ad inviare un pacchetto*/
#define HOST_UNREACHABLE_RESPONSE 2             /**< valore di return della funzione `sendPacket()` se non riesce a contattare il destinatario*/

#define ACK_WAITING_MILLIS 200                  /**< Tempo di attesa prima di reinviare un messaggio che richiede ACK*/

const MacAddress BROADCAST = MacAddress((uint64_t)0x0000000000000000);


/**
* Classe che rappresenta un pacchetto in circolazione nella rete LoRa.
*/
class Packet{
    public:
     MacAddress dest;            /**< Il destinatario del pacchetto. */
     MacAddress sender;          /**< Il mittente del pacchetto. */
     uint8_t type;             	 /**< Il tipo di pacchetto. */
	 uint8_t packetNumber;     	 /**< Il numero di sequenza del pacchetto. */
	 uint8_t packetLength;     	 /**< La lunghezza del payload. */
     char body[245];           	 /**< Il payload del pacchetto. */
	 
	 /**
	   * Costruttore completo per inizializzare un pacchetto.
	   * Un pacchetto creato con questo costruttore avrà `isUninitialized() = false`
	   * @param _dest destinatario
	   * @param _sender mittente
	   * @param _type tipo
	   * @param _packetNumber numero di sequenza
	   * @param _body contenuto del pacchetto
	   * @param _packetLength lunghezza del payload
	 */
     Packet(MacAddress _dest, MacAddress _sender, uint8_t _type, uint8_t _packetNumber, char* _body, uint8_t _packetLength){
         dest = _dest;
         sender = _sender;
         type = _type;
		 packetNumber = _packetNumber;
         for(int a = 0; a < _packetLength; a++)
            body[a] = _body[a];
         packetLength = _packetLength;
	 }
	 
	 /**
	   * Costruttore vuoto per un pacchetto.
	   * Un pacchetto creato con questo costruttore avrà `isUninitialized() = true`
	 */
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

	/**
	   * Controlla se il pacchetto è un ACK.
	   * @return `true` se il pacchetto è un ACK, `false` altrimenti.
	 */
	bool isAck() {
		return (type & 128) == PACKET_TYPE_ACK;
	}

	/**
	   * Controlla se il pacchetto richiede un ACK in risposta.
	   * @return `true` se il pacchetto richiede un ACK, `false` altrimenti.
	 */
	bool requestsAck() {
		return (type & 3) == QOS_REQUEST_ACK;
	}

	/**
	   * Controlla se il pacchetto è stato inizializzato.
	   * @return `true` se il pacchetto non è stato inizializzato, `false` altrimenti.
	 */
	bool isUninitialized(){
		return dest.value == -1;
	}

	/**
	* Visualizza le informazioni del pacchetto
	*/
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


/**
* Struct per semplificare la lettura e scrittura di pacchetti LoRa.
*/
typedef struct Helpers {

	/**
	* Legge 4 bytes da un array.
	* I bytes vengono letti con MSB a sinistra.
	* @param bytes array da cui leggere i dati
	* @return una variabile di tipo uint32_t contenente i primi 4 bytes dell'array.
	*/
	static uint32_t read32bitInt(uint8_t bytes[]) {
		int shifter = 24;
		uint32_t result = 0;
		for (int a = 0; a < 4; a++) {
			result |= (((uint32_t)bytes[a]) << shifter);
			shifter -= 8;
		}
		return result;
	}

	/**
	* Legge 6 bytes da un array.
	* I bytes vengono letti con MSB a sinistra.
	* @param bytes array da cui leggere i dati
	* @return una variabile di tipo uint32_t contenente i primi 4 bytes dell'array.
	*/
	static uint64_t read48bitInt(uint8_t bytes[]) {
		int shifter = 40;
		uint64_t result = 0;
		for (int a = 0; a < 6; a++) {
			result |= (((uint64_t)bytes[a]) << shifter);
			shifter -= 8;
		}
		return result;
	}

	/**
	* Scrive 4 bytes nel pacchetto LoRa corrente.
	* I bytes vengono scritti con MSB a sinistra.
	* @param value i 4 bytes da scrivere
	*/
	static void write48bitIntToPacket(MacAddress address) {
		for(int a = 0; a < 6; a++){
			LoRa.write(address.bytes[a]);
		}
	}

	/**
	* Legge primi 4 bytes disponibili in un pacchetto LoRa.
	* Due chiamate sucessive leggeranno bytes diversi.<br/>
	* I dati possono essere letti una volta sola.<br/>
	* I bytes vengono letti con MSB a sinistra.<br/>
	* @param buffer un array di minimo 4 bytes in cui verranno scritti i dati letti
	*/
	static void read6BytesInto(uint8_t buffer[]) {
		for (int a = 0; a < 6; a++)
			buffer[a] = LoRa.read();
	}

	/**
	* Stampa l'esito di una operazione di invio pacchetto su `Serial`.
	* @see `sendPacket()`
	* @param response_code il valore di return di `sendPacket`
	*/
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

	/**
	* Legge un pacchetto ricevuto in LoRa.
	* Due chiamate sucessive leggeranno bytes diversi.<br/>
	* I dati possono essere letti una volta sola.<br/>
	* I bytes vengono letti con MSB a sinistra.<br/>
	* @return una istanza della classe `Packet` contenete le informazioni sul pacchetto ricevuto
	*/
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

/**
* Struttura per contenere le informazioni sugli ACK ricevuti.
* Una implementazione di questo tipo diventa necessaria se si vuole usare la ricezione asincrona.
*/
typedef struct hasReceivedAckHolder {
	bool hasAck = false;       /**< Indica se si ha ricevuto un nuovo pacchetto ACK */
	Packet ack = Packet();     /**< L'eventuale pacchetto ACK ricevuto */
}AckHolder;


typedef void(*functionCall)(Packet arg); /**< Il tipo delle funzioni che possono essere utilizzate per iscriversi al'evento di ricezione dei pacchetti.*/

extern uint8_t packetCounter; /**< Il contatore dei pacchetti*/
extern functionCall subscribedFunction; /**< La funzione che viene richiamata alla ricezione di un pacchetto*/

/**
* Inizializza il modulo lora.
* @param _myAddress l'indirizzo che si vuole utilizzare per il dispositivo
* @param csPin il pin a cui è collegato il pin `NSS` del modulo
* @param resetPin il pin a cui è collegato il pin `reset` del modulo
* @param irqPin il pin a cui è collegato il pin `DIO0` del modulo. Serve solo se si utilizza la ricezione asincrona,altrimenti passare 0
*/
void initLoRa(MacAddress _myAddress, int csPin, int resetPin, int irqPin); 

/**
* Controlla se è stato ricevuto un pacchetto.
* Se ha ricevuto un pacchetto richiama automaticamente la funzione `subscribedFunction` 
*/
void checkIncoming();

/**
* Invia un pacchetto in LoRa.
* @param packet il pacchetto di inviare
* @return il risultato dell'operazione, da passare alla funzione `Helpers::printResponseMessage()`
*/
int sendPacket(Packet packet);

/**
* Legge un pacchetto LoRa. utilizzata internamente
*/
void receivePacket(int packetSize);

/**
* Cambia il proprio indirizzo nella rete LoRa.
* @param newAddress il nuovo indirizzo che si vuole avere
*/
void changeAddress(MacAddress newAddress);


/**
* Funzione per iscriversi all'evento di ricezione di pacchetti.
* @param function la funzione di tipo `functionCall` che si vuole venga richiamata ogni volta che si riceve un pacchetto
*/
void subscribeToReceivePacketEvent(functionCall function);

/**
* Crea un pacchetto di tipo messaggio.
* @param dest il destinatario
* @param sender il mittente
* @param body il payload
* @param packetLength la lunghezza di body
* @return the packet created. To use as: `sendPacket(MessagePacket(...))`
*/
static Packet MessagePacket(MacAddress dest, MacAddress sender, char body[], uint8_t packetLength) {
	return Packet(dest, sender, PACKET_TYPE_NORM, packetCounter, body, packetLength);
}

/**
* Crea un pacchetto di tipo messaggio che richiede un ack in risposta.
* @param dest il destinatario
* @param sender il mittente
* @param body il payload
* @param packetLength la lunghezza di body
* @return the packet created. To use as: `sendPacket(MessageAckPacket(...))`
*/
static Packet MessageAckPacket(MacAddress dest, MacAddress sender, char body[], uint8_t packetLength) {
	return Packet(dest, sender, PACKET_TYPE_NORM | PACKET_TYPE_REQUESTS_ACK, packetCounter, body, packetLength);
}

/**
* Crea un pacchetto di tipo ACK.
* @param dest il destinatario (lo stesso del messaggio ricevuto)
* @param sender il mittente
* @param reponsePacketNumber il contatore del pacchetto (lo stesso del messaggio ricevuto)
* @return the packet created. To use as: `sendPacket(AckPacket(...))`
*/
static Packet AckPacket(MacAddress dest, MacAddress sender, uint8_t reponsePacketNumber) {
	return Packet(dest, sender, PACKET_TYPE_ACK, reponsePacketNumber, "", 0);
}

#endif