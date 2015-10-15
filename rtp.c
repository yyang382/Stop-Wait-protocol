#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/* 
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){

	/*  ----  FIXME  ----
	 *
	 *  The goal is to return a number that is determined by the contents
	 *  of the buffer passed in as a parameter.  There a multitude of ways
	 *  to implement this function.  For simplicity, simply sum the ascii
	 *  values of all the characters in the buffer, and return the total.
	 */ 
	 int i;
	 int sum = 0;
	 for (i = 0;i < length; i++) {
	 	sum += buffer[i];
	 }
	 return sum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){

	/*  ----  FIXME  ----
	 *  The goal is to turn the buffer into an array of packets.
	 *  You should allocate the space for an array of packets and
	 *  return a pointer to the first element in that array.  Each 
	 *  packet's type should be set to DATA except the last, as it 
	 *  should be LAST_DATA type. The integer pointed to by 'count' 
	 *  should be updated to indicate the number of packets in the 
	 *  array.
	 */ 
	 PACKET* packet;
	 int size;
	 size = (length + MAX_PAYLOAD_LENGTH - 1)/ MAX_PAYLOAD_LENGTH;
	 *count = size;
	 int i;
	 int pos;
	PACKET* packets = calloc(size, sizeof(PACKET));
	if(packets == NULL) {
		exit(1);
	} else {
		for (i = 0; i < length; i++) {
			packet = packets + (i / MAX_PAYLOAD_LENGTH);
			pos = (i % MAX_PAYLOAD_LENGTH);
			packet->payload[pos] = buffer[i];

			if (i == length - 1) {

				packet->type = LAST_DATA;
				packet->payload_length = pos + 1;
				packet->checksum = checksum(packet->payload, packet->payload_length);

			} else if (pos == (MAX_PAYLOAD_LENGTH - 1 )) {
				packet->type = DATA;
				packet->payload_length = MAX_PAYLOAD_LENGTH;
				packet->checksum = checksum(packet->payload, packet->payload_length);
			}
		}
	}

	return packets;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE*msg){
	/* ---- FIXME ----
	 * The goal of this function is to turn the message buffer
	 * into packets and then, using stop-n-wait RTP protocol,
	 * send the packets and re-send if the response is a NACK.
	 * If the response is an ACK, then you may send the next one
	 */
	 
	 int count = 0;
	 int i = 0;
	 int len;
	 PACKET *packets = packetize(msg->buffer,msg->length,&count);
	 PACKET *resPacket = malloc(sizeof(PACKET));
	 assert(resPacket != NULL);
	 len = sizeof(packets[count - 1]);
	 for( i = 0; i < count; i++) {
	 	if (i == count - 1) {
	 		do {
	 											
	 			sendto(connection->socket, packets + i,
	 					len, 0, connection->remote_addr,
	 					connection->addrlen);
	 			
	 			do {
	 				recvfrom(connection->socket, resPacket,
	 					len, 0, connection->remote_addr,
	 				    &(connection->addrlen));
	 			} while(resPacket->type != ACK && resPacket->type != NACK);

	 			
	 		} while (resPacket->type != ACK);
	 	}else {
	 		sendto(connection->socket, packets + i,
	 					len, 0, connection->remote_addr,
	 					connection->addrlen);
			do {
				recvfrom(connection->socket, resPacket,
	 					len, 0, connection->remote_addr,
	 				    &(connection->addrlen));
	 		} while(resPacket->type != ACK && resPacket->type != NACK);
	 		if (resPacket->type == NACK) {
	 			i--;
	 		}
	 	}
	}
	free(resPacket);
	return 0;

}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	/* ---- FIXME ----
	 * The goal of this function is to handle 
	 * receiving a message from the remote server using
	 * recvfrom and the connection info given. You must 
	 * dynamically resize a buffer as you receive a packet
	 * and only add it to the message if the data is considered
	 * valid. The function should return the full message, so it
	 * must continue receiving packets and sending response 
	 * ACK/NACK packets until a LAST_DATA packet is successfully 
	 * received.
	 */
	 int end = 0;
	 PACKET *packet = malloc(sizeof(PACKET));
	 assert(packet != NULL) ;
	 MESSAGE *message = malloc(sizeof(MESSAGE));
	 assert(message != NULL) ;
	 int sum_rec = 0;
	 int size = 0;
	 int res;
	 char* mesBuffer = (char*)malloc(1);
	 assert(mesBuffer != NULL); 
	 char* mesIter = mesBuffer;
	 int i;
	 do{

	 	recvfrom(connection->socket, packet, sizeof(PACKET), 0,connection->remote_addr,
						&(connection->addrlen));
	 	sum_rec = checksum(packet->payload, packet->payload_length);
	 	if (packet->type == LAST_DATA) {
	 		end = 1;
		 }
	 	if (sum_rec != packet->checksum) {
	 		packet->type = NACK;
	 		res = NACK;
		 	sendto(connection->socket, packet, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);

		 } else {
			size = size +  packet->payload_length;
	 		packet->type = ACK;
	 		res = ACK;
	 		char* temp = (char*) realloc(mesBuffer, size);
	 		if (temp != NULL) {
	 			mesBuffer = temp;
	 		}
	 		for (i = 0; i < packet->payload_length; i++) {
	 			*(mesIter++) = packet->payload[i];
	 		}
		 	sendto(connection->socket, packet, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);

			
	 	}
	}while(res != ACK||!end);
	*mesIter = 0;
	message->length = packet->payload_length;
	message->buffer = mesBuffer;
	free(packet);
	return message;

}
