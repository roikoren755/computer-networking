//
// Created by Roi Koren on 2019-03-24.
//

#include "seker_helpers.h"

int receiveAll(int socketFd, char* buffer, int* length) {
	int received = 0;
	int total = 0;
	while (total < *length) {
		memset(buffer, 0, *length);
		if ((received = recv(socketFd, buffer, *length, 0)) < 0) {
			break;
		}

		total += received;
	}

	*length = total;

	return received < 0 ? -1 : 0;
}

int sendAll(int socketFd, char* buffer, int* length) {
	int total = 0;
	int bytesLeft = *length;
	int sent;

	while (total < *length) {
		sent = send(socketFd, buffer + total, bytesLeft, 0);
		if (sent < 0) {
			break;
		}

		total += sent;
		bytesLeft -= sent;
	}

	*length = total;

	return sent < 0 ? -1 : 0;
}