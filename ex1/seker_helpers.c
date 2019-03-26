//
// Created by Roi Koren on 2019-03-24.
//

#include "seker_helpers.h"

#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

int receiveAll(int socketFd, char* buffer, int* length) {
	int total = 0;
	int bytesLeft = *length;
	int received;

	memset(buffer, 0, *length);
	while (total < *length) {
		received = recv(socketFd, buffer + total, bytesLeft, 0);
		if (received < 0) {
			break;
		}

		total += received;
		bytesLeft -= received;
	}

	*length = total;

	return received < 0 ? ERROR : SUCCESS;
}

int sendAll(int socketFd, char* buffer, int* length) {
	int total = 0;
	int bytesLeft = *length;
	int sent;

	memset(buffer, 0, *length);
	while (total < *length) {
		sent = send(socketFd, buffer + total, bytesLeft, 0);
		if (sent < 0) {
			break;
		}

		total += sent;
		bytesLeft -= sent;
	}

	*length = total;

	return sent < 0 ? ERROR : SUCCESS;
}

int sendInt(int socketFd, int toSend) {
	uint32_t networkToSend = htonl(toSend);
	char* byteToSend = (char*) &networkToSend;
	int size = sizeof(networkToSend);

	return sendAll(socketFd, byteToSend, &size);
}

int sendString(int socketFd, char* toSend) {
	int length = strlen(toSend);
	if (sendInt(length, socketFd)) {
		return ERROR;
	}

	return sendAll(socketFd, toSend, &length);
}

int receiveString(int socketFd, char* buffer) {
	int stringLength = receivePositiveInt(socketFd);
	if (stringLength < 0) {
		return ERROR;
	}

	return receiveAll(socketFd, buffer, &stringLength);
}

int receivePositiveInt(int socketFd) {
	uint32_t response;
	char* byteResponse = (char*) &response;
	int size = sizeof(response);
	if (receiveAll(socketFd, byteResponse, &size) < 0) {
		return -ERROR;
	}

	return ntohl(response);
}