//
// Created by Roi Koren on 2019-03-24.
//

#include "seker_client.h"
#include "seker_helpers.h"

#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
	char hostname[1024] = {};
	char port[10] = {};

	if (argc > 3) {
		printf("USAGE: %s [hostname [port]]\n", argv[0]);
		return 1;
	}

	if (argc > 1) {
		strcpy(hostname, argv[1]);
	} else {
		strcpy(hostname, "localhost");
	}

	if (argc > 2) {
		strcpy(port, argv[2]);
	} else {
		strcpy(port, "1337");
	}

	struct addrinfo* result;
	if (getaddrinfo(hostname, port, NULL, &result) < 0) {
		perror("Could not resolve hostname '%s'!");
		return 2;
	}

	if (result == NULL) {
		printf("No results found for hostname %s and port %s!\n", hostname, port);
		return 3;
	}

	int socketFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (socketFd < 0) {
		perror("Could not open socket to server!");
		return 4;
	}

//	if (connect(socketFd, result->ai_addr, sizeof(struct sockaddr_in)))
//
//	int quitEntered = 0
//	while (!quitEntered) {
//		if (1) {
//			quitEntered = 1
//		}
//	}
//
//	close(socketFd);

	return 0;
}