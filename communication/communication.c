#include "communication.h"
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

void upload_file(int socket, Packet *packet, size_t packetSize) {
	int bytes_sent = write(socket, packet, sizeof(Packet)); 		

	if (bytes_sent == -1)
		printf("Error to upload packet");
}

void download_file(int socket, Packet *packet, size_t packetSize) {
	int bytes_read = read(socket, packet, sizeof(Packet));
	
	if (bytes_read == -1)
		printf("Failed to read packet");
}

void delete_file(int socket, Packet *packet, size_t packetSize) {
}

