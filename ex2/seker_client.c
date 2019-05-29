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
struct addrinfo* adrrinfo_result;

/***
 * Closes socketFd and returns exitCode
 * @param exitCode - return value wanted
 * @return exitCode
 */
int closeSocket(int exitCode) {
	close(socketFd);
	freeaddrinfo(adrrinfo_result);
	return exitCode;
}

int validateInputIsNumber(char* input, int numberOfDigits) {
	if (input == NULL) {
		return ERROR;
	}

	int i = 0;
	char c;
	while (input[i] != '\0') {
		c = input[i];
		if (c < '0' || c > '9') {
			return ERROR;
		}

		i++;
	}

	return i > numberOfDigits ? ERROR : SUCCESS;
}

int validateInputIsInQuotes(char* input) {
	if (input == NULL) {
		return ERROR;
	}

	if (input[0] != '"' || input[strlen(input) - 1] != '"') {
		return ERROR;
	}

	return SUCCESS;
}

/***
 * Parses command to get an integer representation for ease of handling
 * @param command - command to parse
 * @return appropriate value if command name is valid
 * 		   PARSING_ERROR otherwise
 */
int getCommandInt(char* command) {
	char buffer[MAXIMUM_RATING_TEXT_LENGTH + 101] = {0};

	strncpy(buffer, command, MAXIMUM_RATING_TEXT_LENGTH + 100);

	char* argument = strtok(buffer, DELIMITERS);
	char* rest = strtok(NULL, DELIMITERS);
	if (argument[strlen(argument) - 1] == '\n') {
		argument[strlen(argument) - 1] = '\0';
	}
	if (rest && rest[strlen(rest) - 1] == '\n') {
		rest[strlen(rest) - 1] = '\0';
	}


	if (!strcmp(argument, list_of_courses)) {
		if (rest != NULL) {
			return PARSING_ERROR;
		}

		return LIST_OF_COURSES;
	}

	if (!strcmp(argument, add_course)) {
		if (validateInputIsNumber(rest, 4) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, "\n");
		if (validateInputIsInQuotes(rest) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, DELIMITERS);
		if (rest != NULL) {
			return PARSING_ERROR;
		}

		return ADD_COURSE;
	}

	if (!strcmp(argument, rate_course)) {
		if (validateInputIsNumber(rest, 4) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, DELIMITERS);
		if (validateInputIsNumber(rest, 3) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, "\n");
		if (validateInputIsInQuotes(rest) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, DELIMITERS);
		if (rest != NULL) {
			return PARSING_ERROR;
		}

		return RATE_COURSE;
	}

	if (!strcmp(argument, get_rate)) {
		if (validateInputIsNumber(rest, 4) != SUCCESS) {
			return PARSING_ERROR;
		}

		rest = strtok(NULL, DELIMITERS);
		if (rest != NULL) {
			return PARSING_ERROR;
		}

		return GET_RATE;
	}

	if (!strcmp(argument, quit)) {
		if (rest != NULL) {
			return PARSING_ERROR;
		}

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
	if (getaddrinfo(hostname, port, NULL, &adrrinfo_result) < 0) {
		perror("Could not resolve hostname '%s'!");
		return HOSTNAME_RESOLUTION_ERROR;
	}

	if (adrrinfo_result == NULL) {
		printf("No results found for hostname %s and port %s!\n", hostname, port);
		return HOSTNAME_RESOLUTION_ERROR;
	}

	socketFd = socket(adrrinfo_result->ai_family, adrrinfo_result->ai_socktype, adrrinfo_result->ai_protocol);
	if (socketFd < 0) {
		perror("Could not open socket to server!");
		freeaddrinfo(adrrinfo_result);
		return SOCKET_OPENING_ERROR;
	}

	if (connect(socketFd, adrrinfo_result->ai_addr, (socklen_t) adrrinfo_result->ai_addrlen) < 0) {
		perror("Could not connect to server!");
		freeaddrinfo(adrrinfo_result);
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
	char username[MAXIMUM_USERNAME_LENGTH + 1];
	char password[MAXIMUM_USERNAME_LENGTH + 1];
	int response;

	do {
		printf("Welcome! Please log in.\n");
		memset(username, 0, MAXIMUM_USERNAME_LENGTH + 1);
		memset(password, 0, MAXIMUM_USERNAME_LENGTH + 1);
		memset(inputBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		fgets(inputBuffer, MAXIMUM_RATING_TEXT_LENGTH + 100, stdin);
		char* token = strtok(inputBuffer, " ");
		if (strcmp(token, "User:")) {
			printf("ERROR: malformed input\n");
			response = ERROR;
			continue;
		}

		token = strtok(NULL, " ");
		if (!token) {
			printf("ERROR: malformed input\n");
			response = ERROR;
			continue;
		}
		strncpy(username, token, MAXIMUM_USERNAME_LENGTH);
		memset(inputBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		fgets(inputBuffer, MAXIMUM_RATING_TEXT_LENGTH + 100, stdin);
		token = strtok(inputBuffer, " ");
		if (strcmp(token, "Password:")) {
			printf("ERROR: malformed input\n");
			response = ERROR;
			continue;
		}

		token = strtok(NULL, " ");
		if (!token) {
			printf("ERROR: malformed input\n");
			response = ERROR;
			continue;
		}
		strncpy(password, token, MAXIMUM_USERNAME_LENGTH);

		if (username[0] == '\0' || password[0] == '\0') {
			printf("ERROR: Empty strings received from user.\n");
			return INPUT_ERROR;
		}

		removeTrailingChar(username);
		removeTrailingChar(password);


		response = sendPositiveInt(socketFd, 0); //send "I'm ready to login int" for select()
		if (response != SUCCESS) {
			printf("ERROR: Could not initialize handshake!");
			return HANDSHAKE_ERROR;
		}
		if (sendString(socketFd, username) || sendString(socketFd, password)) {
			perror("Could not send message to server.");
			return TCP_SEND_ERROR;
		}

		response = receivePositiveInt(socketFd);
		if (response == -ERROR) {
			perror("Could not read server response!");
			return TCP_RECEIVE_ERROR;
		}

		if (response == ERROR) {
			printf("Failed to login.\n");
		}
	} while (response != SUCCESS);

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

	while (1) {
		memset(receiveBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		if (receiveString(socketFd, receiveBuffer)) {
			perror("Could not receive course list from server");
			return TCP_RECEIVE_ERROR;
		}

		if (strcmp(receiveBuffer, END_OF_LIST)) {
			printf("%s", receiveBuffer);
		} else {
			break;
		}
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

	int response = receivePositiveInt(socketFd);
	if (response == -ERROR) {
		perror("Could not receive rate_course response from server");
		return TCP_RECEIVE_ERROR;
	}

	if (response == ERROR) {
		printf("ERROR: %d doesn't exist", courseInt);
		return SUCCESS;
	}

	argument = strtok(NULL, DELIMITERS);
	int grade = atoi(argument);
	if (grade > 100) {
		printf("Illegal command.\n");
		return SUCCESS;
	}
	if (sendPositiveInt(socketFd, grade)) {
		perror("Could not send rating value to server");
		return TCP_SEND_ERROR;
	}

	argument = strtok(NULL, QUOTATION_MARK);
	if (sendString(socketFd, argument)) {
		perror("Could not send rating text to server");
		return TCP_SEND_ERROR;
	}

	printf("%d rated successfully.\n", courseInt);
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
	int courseExist = receivePositiveInt(socketFd);
	if (courseExist == ERROR) {
		printf("ERROR: course %d doesn't exist\n", atoi(courseNum));
		return SUCCESS;
	}
	while (1) {
		memset(receiveBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		if (receiveString(socketFd, receiveBuffer)) {
			perror("Could not receive rating details from server");
			return TCP_RECEIVE_ERROR;
		}

		if (strcmp(receiveBuffer, END_OF_LIST)) {
			printf("%s", receiveBuffer);
		} else {
			break;
		}
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
	int error = 0;
	while (!quitEntered) {
		memset(inputBuffer, 0, MAXIMUM_RATING_TEXT_LENGTH + 100);
		fgets(inputBuffer, MAXIMUM_RATING_TEXT_LENGTH + 100, stdin);
		int command = getCommandInt(inputBuffer);
		switch (command) {
			case LIST_OF_COURSES:
				error = handleListOfCourses();
				break;
			case ADD_COURSE:
				error = handleAddCourse();
				break;
			case RATE_COURSE:
				error = handleRateCourse();
				break;
			case GET_RATE:
				error = handleGetRate();
				break;
			case QUIT:
				sendPositiveInt(socketFd, QUIT);
				quitEntered = 1;
				break;
			default:
				printf("Illegal command.\n");
				break;
		}

		if (error) {
			return closeSocket(error);
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
