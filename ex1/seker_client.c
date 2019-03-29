//
// Created by Roi Koren on 2019-03-24.
//

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
#define QUOTATION_MARK "\""

#define PARSING_ERROR -1
#define USAGE_ERROR 1
#define HOSTNAME_RESOLUTION_ERROR 2
#define SOCKET_OPENING_ERROR 2
#define SERVER_CONNECTING_ERROR 3
#define INTERNAL_SERVER_ERROR 4
#define TCP_RECEIVE_ERROR 5
#define INPUT_ERROR 6
#define TCP_SEND_ERROR 7
#define HANDSHAKE_ERROR 8

char hostname[MAXIMUM_RATING_TEXT_LENGTH + 1] = {0};
char port[10] = {0};
char inputBuffer[MAXIMUM_RATING_TEXT_LENGTH + 101] = {0};
char receiveBuffer[MAXIMUM_RATING_TEXT_LENGTH + 101] = {0};
int socketFd;

/***
 * Closes socketFd and returns exitCode
 * @param exitCode - return value wanted
 * @return exitCode
 */
int closeSocket(int exitCode) {
	close(socketFd);
	return exitCode;
}

/***
 * Parses command to get an integer representation for ease of handling
 * @param command - command to parse
 * @return appropriate value if command name is valid
 * 		   PARSING_ERROR otherwise
 */
int getCommandInt(char* command) {
	char* firstArgument = strtok(command, DELIMITERS);
	if(firstArgument[strlen(firstArgument)-1]=='\n')
		firstArgument[strlen(firstArgument)-1]='\0';

	if (!strcmp(firstArgument, list_of_courses)) {
		return LIST_OF_COURSES;
	}

	if (!strcmp(firstArgument, add_course)) {
		command[strlen(add_course)]='\t';
		return ADD_COURSE;
	}

	if (!strcmp(firstArgument, rate_course)) {
		command[strlen(rate_course)]='\t';
		return RATE_COURSE;
	}

	if (!strcmp(firstArgument, get_rate)) {
			command[strlen(get_rate)]='\t';
			return GET_RATE;
	}

	if (!strcmp(firstArgument, quit)) {
		return QUIT;
	}

	return PARSING_ERROR;
}

/***
 * Verifies program arguments
 * @param argc - number of arguments
 * @param argv - array of arguments
 * @return USAGE_ERROR if more than 2 arguments are passed to the program
 * 		   SUCCESS otherwise
 */
int verifyArguments(int argc, char* argv[]) {
	if (argc > 3) { // No more than 2 arguments
		printf("USAGE: %s [hostname [port]]\n", argv[0]);
		return USAGE_ERROR;
	}

	if (argc > 1) { // Host name supplied
		strncpy(hostname, argv[1], MAXIMUM_RATING_TEXT_LENGTH);
	} else {
		strncpy(hostname, "localhost", 10);
	}

	if (argc > 2) { // Port supplied too
		strncpy(port, argv[2], 10);
	} else {
		sprintf(port, "%d", DEFAULT_PORT);
	}

	return SUCCESS;
}

/***
 * Finds server address info, opens a socket and connects to the server
 * @return appropriate error code if error occurs
 * 		   SUCCESS otherwise
 */
int findAndConnectToServer() {
	struct addrinfo* result;
	if (getaddrinfo(hostname, port, NULL, &result) < 0) {
		perror("Could not resolve hostname '%s'!");
		return HOSTNAME_RESOLUTION_ERROR;
	}

	if (result == NULL) {
		printf("No results found for hostname %s and port %s!\n", hostname, port);
		return HOSTNAME_RESOLUTION_ERROR;
	}

	socketFd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (socketFd < 0) {
		perror("Could not open socket to server!");
		return SOCKET_OPENING_ERROR;
	}

	if (connect(socketFd, result->ai_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("Could not connect to server!");
		return closeSocket(SERVER_CONNECTING_ERROR);
	}

	return SUCCESS;
}

/***
 * Initializes connection to server, and attempts to log user in
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int initialHandshake() {
	int response = receivePositiveInt(socketFd);
	if (response != SUCCESS) {
		printf("ERROR: Could not initialize handshake!");
		return HANDSHAKE_ERROR;
	}
	char username[MAXIMUM_USERNAME_LENGTH + 1];
	char password[MAXIMUM_USERNAME_LENGTH + 1];

	printf("Welcome! Please log in.\n");

	do {
		memset(username, 0, MAXIMUM_USERNAME_LENGTH + 1);
		memset(password, 0, MAXIMUM_USERNAME_LENGTH + 1);

		printf("Username: ");
		fgets(username, MAXIMUM_USERNAME_LENGTH, stdin);
		printf("Password: ");
		fgets(password, MAXIMUM_USERNAME_LENGTH, stdin);

		if (username[0] == '\0' || password[0] == '\0') {
			printf("ERROR: Empty strings received from user.\n");
			return INPUT_ERROR;
		}

		removeTrailingChar(username);
		removeTrailingChar(password);

		if (sendString(socketFd, username) || sendString(socketFd, password)) {
			perror("Could not send message to server.");
			return TCP_SEND_ERROR;
		}

		response = receivePositiveInt(socketFd);
		if (response == -ERROR) {
			perror("Could not read server response!");
			return TCP_RECEIVE_ERROR;
		}
	} while (response != 0);

	printf("Hi %s, good to see you.\n", username);

	return SUCCESS;
}

/***
 * Handles "list_of_courses" input from user
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int handleListOfCourses() {
	if (sendPositiveInt(socketFd, LIST_OF_COURSES)) {
		perror("Could not send requestId to server");
		return TCP_SEND_ERROR;
	}

	while(1){
		memset(receiveBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		if (receiveString(socketFd, receiveBuffer)){
			perror("Could not receive course list from server");
			return TCP_RECEIVE_ERROR;
		}
		if(strcmp(receiveBuffer, END_OF_LIST))
			printf("%s", receiveBuffer);
		else
			break;
	}

	return SUCCESS;
}

/***
 * Handles "add_course" input from user
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int handleAddCourse() {
	if (sendPositiveInt(socketFd, ADD_COURSE)) {
		perror("Could not send requestId to server");
		return TCP_SEND_ERROR;
	}

	char* argument = strtok(inputBuffer, DELIMITERS);
	argument = strtok(NULL, DELIMITERS);
	int courseInt = atoi(argument);
	if (sendPositiveInt(socketFd, courseInt)) {
		perror("Could not send course number to server");
		return TCP_SEND_ERROR;
	}

	int response = receivePositiveInt(socketFd);
	if (response == -ERROR) {
		perror("Could not receive response from server");
		return TCP_RECEIVE_ERROR;
	} else if (response == ERROR) {
		printf("%d exists in the database!\n", courseInt);
	} else {
		argument = strtok(NULL, QUOTATION_MARK);
		if (sendString(socketFd, argument)) {
			perror("Could not send course name to server");
			return TCP_SEND_ERROR;
		}

		printf("%d added successfully.\n", courseInt);
	}

	return SUCCESS;
}

/***
 * Handles "rate_course" input from user
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int handleRateCourse() {
	if (sendPositiveInt(socketFd, RATE_COURSE)) {
		perror("Could not send requestId to server");
		return TCP_SEND_ERROR;
	}

	char* argument = strtok(inputBuffer, DELIMITERS);
	argument = strtok(NULL, DELIMITERS);
	int courseInt = atoi(argument);
	if (sendPositiveInt(socketFd, courseInt)) {
		perror("Could not send course number to server");
		return TCP_SEND_ERROR;
	}

	int response;
	if ((response = receivePositiveInt(socketFd))==-ERROR) {
			perror("Could not receive rate_course response from server");
			return TCP_RECEIVE_ERROR;
	}
	if(response==ERROR){
		printf("ERROR: %d doesn't exist", courseInt);
		return SUCCESS;
	}

	argument = strtok(NULL, DELIMITERS);
	int grade = atoi(argument);
	if (sendPositiveInt(socketFd, grade)) {
		perror("Could not send rating value to server");
		return TCP_SEND_ERROR;
	}

	argument = strtok(NULL, QUOTATION_MARK);
	if (sendString(socketFd, argument)) {
		perror("Could not send rating text to server");
		return TCP_SEND_ERROR;
	}

	printf("%d added successfully.\n", courseInt);
	return SUCCESS;
}

/***
 * Handles "get_rate" input from user
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int handleGetRate() {
	if (sendPositiveInt(socketFd, GET_RATE)) {
		perror("Could not send requestId to server");
		return TCP_SEND_ERROR;
	}
	char* courseNum = strtok(inputBuffer, DELIMITERS);
	courseNum = strtok(NULL, DELIMITERS);
	if (sendPositiveInt(socketFd, atoi(courseNum))) {
			perror("Could not send courseNum to server");
			return TCP_SEND_ERROR;
	}

	while(1){
		memset(receiveBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		if (receiveString(socketFd, receiveBuffer)){
			perror("Could not receive rating details from server");
			return TCP_RECEIVE_ERROR;
		}
		if(strcmp(receiveBuffer, END_OF_LIST))
			printf("%s", receiveBuffer);
		else
			break;
	}
	return SUCCESS;
}

/***
 * Handles input from user in a loop, until he enters "quit"
 * @return appropriate error code if an error occurs
 * 		   SUCCESS otherwise
 */
int handleUserCommands() {
	int quitEntered = 0;
	int error;
	while (!quitEntered) {
		memset(inputBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		fgets(inputBuffer, MAXIMUM_RATING_TEXT_LENGTH + 100, stdin);
		int command = getCommandInt(inputBuffer);
		switch (command) {
			case LIST_OF_COURSES:
				error = handleListOfCourses();
				if (error) {
					return closeSocket(error);
				}
				break;
			case ADD_COURSE:
				error = handleAddCourse();
				if (error) {
					return closeSocket(error);
				}
				break;
			case RATE_COURSE:
				error = handleRateCourse();
				if (error) {
					return closeSocket(error);
				}
				break;
			case GET_RATE:
				error = handleGetRate();
				if (error) {
					return closeSocket(error);
				}
				break;
			case QUIT:
				sendPositiveInt(socketFd, QUIT);
				quitEntered = 1;
				break;
			default:
				printf("Illegal command.\n");
				break;
		}
	}

	return closeSocket(SUCCESS);
}

int main(int argc, char* argv[]) {
	int error = verifyArguments(argc, argv);
	if (error) {
		return error;
	}

	error = findAndConnectToServer();
	if (error) {
		return error;
	}

	error = initialHandshake();
	if (error) {
		return closeSocket(error);
	}

	return handleUserCommands();
}
