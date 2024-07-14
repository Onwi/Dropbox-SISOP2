#include <stdlib.h>
#include <stdint.h>

typedef struct packet{
	int command; //Tipo do comando
	uint16_t seqn; //Número de sequência
	uint32_t total_size; //Número total de fragmentos
	uint16_t length; //Comprimento do payload
	const char* filename;	
	const char* payload; //Dados do pacote
} Packet;

// send file thru socket
// file infos are defined in packet 
void upload_file(int socket, Packet *packet, size_t packetSize);

// send a command to download a file from the server.
// command will be represented by command in packet struct
// the download command is represented by 1
void download_file(int socket, Packet *packet, size_t packetSize);

// send a request to delete a file thru socket
// file info will be defined in packet 
void delete_file(int socket, Packet *packet, size_t packetSize);

