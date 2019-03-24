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

#define list_of_courses "list_of_courses"
#define add_course "add_course"
#define rate_course "rate_course"
#define get_rate "get_rate"
#define quit "quit"
#define DELIMITERS " \t"
#define QUIT 5
#define PARSING_ERROR -1

int getCommandInt(char* command) {
	char* firstArgument = strtok(command, DELIMITERS);
	command = strtok(NULL, DELIMITERS);
	if (!strcmp(firstArgument, list_of_courses)) {
		return LIST_OF_COURSES;
	}

	if (!strcmp(firstArgument, add_course)) {
		return ADD_COURSE;
	}

	if (!strcmp(firstArgument, rate_course)) {
		return RATE_COURSE;
	}

	if (!strcmp(firstArgument, quit)) {
		return QUIT;
	}

	return PARSING_ERROR;
}

int main(int argc, char* argv[]) {
	char hostname[MAXIMUM_RATING_TEXT_LENGTH] = {};
	char port[10] = {};
	char* inputBuffer = (char*) malloc(sizeof(char) * (MAXIMUM_RATING_TEXT_LENGTH + 100));
	char* receiveBuffer = (char*) malloc(sizeof(char) * MAXIMUM_RATING_TEXT_LENGTH);

	if (argc > 3) {
		printf("USAGE: %s [hostname [port]]\n", argv[0]);
		return 1;
	}

	if (argc > 1) {
		strncpy(hostname, argv[1], MAXIMUM_RATING_TEXT_LENGTH);
	} else {
		strncpy(hostname, "localhost", 10);
	}

	if (argc > 2) {
		strncpy(port, argv[2], 10);
	} else {
		strncpy(port, "1337", 5);
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

	if (connect(socketFd, result->ai_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not connect to server!");
		return 5;
	}

	uint32_t response;
	char* ret = (char*) &response;
	int size = sizeof(response);
	if (receiveAll(socketFd, ret, &size) < 0) {
		perror("Could not read server message!");
		return 6;
	}

	if (ntohl(response)) {
		printf("ERROR: server returned 1!");
		return 7; // TODO - need to close socket...
	}

	do {
		char username[MAXIMUM_USERNAME_LENGTH + 1];
		char password[MAXIMUM_USERNAME_LENGTH + 1];

		printf("Username: ");
		fgets(username, sizeof(username), stdin);
		printf("Password: ");
		fgets(password, sizeof(password), stdin);
		if (username[0] == '\0' || password[0] == '\0') {
			printf("ERROR: WTF IS GOING ON???");
			return 7777;
		}

		int length = strlen(username);
		uint32_t networkLength = htonl(length);
		char* byteLength = (char*) &networkLength;
		size = sizeof(networkLength);
		if (sendAll(socketFd, byteLength, &size) < 0) {
			printf("ERROR: Could not send username size to server!");
			return 777;
		}

		if (sendAll(socketFd, username, &length) < 0) {
			printf("ERROR: Could not send username to server!");
			return 77777;
		}

		length = strlen(password);
		networkLength = htonl(length);
		byteLength = (char*) &networkLength;
		size = sizeof(networkLength);
		if (sendAll(socketFd, byteLength, &size) < 0) {
			printf("ERROR: Could not send password size to server!");
			return 179238;
		}

		if (sendAll(socketFd, password, &length) < 0) {
			printf("ERROR: Could not send password to server!");
			return 12301827;
		}

		ret = (char*) &response;
		size = sizeof(response);
		if (receiveAll(socketFd, ret, &size) < 0) {
			perror("Could not read server message!");
			return 6;
		}
	} while (!ntohl(response));

	if (receiveAll(socketFd, receiveBuffer, &received) < 0) {
		printf("ERROR: Could not read server message!");
		return 1239716;
	}



	int quitEntered = 0;
	while (!quitEntered) {
		fgets
		if (1) {
			quitEntered = 1;
		}
	}

	close(socketFd);

	return 0;
}