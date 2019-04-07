#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "seker_helpers.h"

#define min(a, b) a < b ? a : b
#define IO_ERROR 2
#define TCP_RECEIVE_ERROR 3
#define TCP_SEND_ERROR 4

struct user_details {
	char userName[MAXIMUM_USERNAME_LENGTH];
	char password[MAXIMUM_USERNAME_LENGTH];
};

struct user_details Users[MAXIMUM_NUMBER_OF_USERS];
int users_count;
char *dir_path;
char *course_list_path;
int listen_port = DEFAULT_PORT;
struct sockaddr_in server_addr;

/***
 * Reads from users_file, and stores all usernames and passwords in the Users array
 * @param users_file - file containing lines of the format username\tpassword
 */
void InitUsers(char* users_file) {
	FILE* fp = fopen(users_file, "r");
	if (fp == NULL) {
		printf("ERROR: can't open file %s", users_file);
		exit(1);
	}

	size_t length = MAXIMUM_USERNAME_LENGTH * 3;
	char* buffer = calloc(sizeof(char), length);
	int bytesread;
	int i = 0;
	while (i < MAXIMUM_NUMBER_OF_USERS) {
		if ((bytesread = getline(&buffer, &length, fp)) < 0) {
			break;
		}

		buffer[bytesread - 1] = '\0'; // remove the \n character

		char* username = strtok(buffer, "\t");
		strncpy(Users[i].userName, username, MAXIMUM_USERNAME_LENGTH);
		char* password = strtok(NULL, "\t");
		strncpy(Users[i].password, password, MAXIMUM_USERNAME_LENGTH);

		Users[i].userName[MAXIMUM_USERNAME_LENGTH - 1] = '\0';
		Users[i].password[MAXIMUM_USERNAME_LENGTH - 1] = '\0';
		i++;
	}

	users_count = i;
	fclose(fp);
	free(buffer);
}

/***
 * Initializes the server folder, deleting old files that might interfere, and creating new files needed
 */
void InitServerFolder() {
	// remove previous files if exist
	char removeFiles[20 + strlen(dir_path)];
	if (dir_path[0] == '/') { // absolute path
		sprintf((char*) &removeFiles, "exec rm -f %s/*.rating", dir_path);
	} else { // relative path
		sprintf((char*) &removeFiles, "exec rm -f ./%s/*.rating", dir_path);
	}

	system(removeFiles);

	// create course_list file
	course_list_path = malloc(strlen(dir_path) + 20);
	if (course_list_path == NULL) {
		printf("ERROR: can't allocate course_list_path string");
		return;
	}

	sprintf(course_list_path, "%s/courselist", dir_path);
	FILE* courselist = fopen(course_list_path, "w+");
	if (courselist == NULL) {
		printf("ERROR: can't create file %s", course_list_path);
		free(course_list_path);
		return;
	}

	fclose(courselist);
}

/***
 * Logs a connected client in, and returns his username
 * @param client_fd - socket connected to the client
 * @return NULL if an error occurred while allocating username string
 * 		   the username of the logged in user, otherwise
 */
char* GetAndValidateUsername(int client_fd) {
	printf("In handshake\n");
	char* username = calloc(sizeof(char), MAXIMUM_USERNAME_LENGTH + 1);
	char password[MAXIMUM_USERNAME_LENGTH + 1] = {0};
	if (username == NULL) {
		printf("ERROR: can't allocate username memory\n");
		return NULL;
	}

	// send greeting
	sendPositiveInt(client_fd, SUCCESS);

	int i;
	while (1) {
		// get username and password
		receiveString(client_fd, username);
		receiveString(client_fd, (char*) &password);
		// find and validate username and password
		for (i = 0; i < users_count; i++) {
			if (!strcmp(Users[i].userName, username)) {
				if (!strcmp(Users[i].password, password)) {
					sendPositiveInt(client_fd, SUCCESS);
					return username;
				}
			}
		}

		// username or password don't match
		sendPositiveInt(client_fd, ERROR);
	}
}

/***
 * Sends the entire content of file through the client_fd socket
 * @param client_fd - socket to a connected client
 * @param file - pointer to file to be sent
 * @param bufferLen - length of buffer to use
 * @param filename - name of the file being sent
 * @return IO_ERROR if an allocation error occurred
 * 		   TCP_SEND_ERROR if an error occurred while sending data to client
 * 		   SUCCESS otherwise
 */
int sendFile(int client_fd, FILE* file, int bufferLen, char* filename) {
	size_t length = bufferLen;
	char* buffer = calloc(sizeof(char), length);
	if (!buffer) {
		printf("ERROR: can't allocate buffer for %s\n", filename);
		fclose(file);
		return IO_ERROR;
	}

	int bytesread;
	while (1) {
		if ((bytesread = getline(&buffer, &length, file)) < 0) {
			break;
		}
		if (sendString(client_fd, buffer)) {
			printf("ERROR: failed to send string to client %d\n", client_fd);
			free(buffer);
			fclose(file);
			return TCP_SEND_ERROR;
		}
	}

	sendString(client_fd, END_OF_LIST);
	free(buffer);
	fclose(file);
	return SUCCESS;
}

/***
 * Verifies that a course whose number is courseNum already exists
 * @param courseNum - course number to check
 * @return IO_ERROR if an error occurred opening the courselist file
 * 		   -1 if the course does not exist
 * 		   SUCCESS otherwise
 */
int findCourseNum(int courseNum) {
	FILE *courseList = fopen(course_list_path, "a+");
	if (courseList == NULL) {
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}

	char* currCourseNum;
	size_t length = MAXIMUM_COURSE_NAME_LENGTH + 10;
	char buffer[length];
	memset(&buffer, 0, length);
	char* bufferAddr = (char*) &buffer;
	while (getline(&bufferAddr, &length, courseList) > 0) {
		currCourseNum = strtok(bufferAddr, ":");
		if (atoi(currCourseNum) == courseNum) {
			fclose(courseList);
			return SUCCESS;
		}
	}

	// course doesn't exist
	fclose(courseList);
	return -1;
}

/***
 * Handles list_of_courses request from the client, by sending the content of the courselist file
 * @param client_fd - socket connected to the client
 * @return IO_ERROR if the courselist file could not be opened
 * 		   the return value of sendFile on the courselist file otherwise
 */
int listAllCourses(int client_fd) {
	FILE* courselist = fopen(course_list_path, "r");
	if (courselist == NULL) {
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}

	// send entire courselist file to client, line by line
	int result = sendFile(client_fd, courselist, MAXIMUM_COURSE_NAME_LENGTH + 10, "courselist");
	return result;
}

/***
 * Handles add_course request from client
 * @param client_fd - socket connected to the client
 * @return findCourseNum return value if an error occurred there
 * 		   IO_ERROR if an error occurred opening the courselist file
 * 		   SUCCESS if the course already exists, or was added successfully
 */
int addCourse(int client_fd) {
	int courseNum = receivePositiveInt(client_fd);

	int result = findCourseNum(courseNum);
	if (result == SUCCESS) {
		// courseNum already exist
		printf("course num %d already exist", courseNum);
		sendPositiveInt(client_fd, ERROR);
		return SUCCESS;
	}

	if (result > 0) {
		// error occurred
		return result;
	}

	// course number doesn't exist in the list
	FILE* courseList = fopen(course_list_path, "a+");
	if (courseList == NULL) {
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}

	sendPositiveInt(client_fd, SUCCESS);
	char buffer[MAXIMUM_COURSE_NAME_LENGTH + 10] = {0};
	receiveString(client_fd, (char*) &buffer);

	fprintf(courseList, "%d:\t%s\n", courseNum, buffer);
	fclose(courseList);

	return SUCCESS;
}

/***
 * Handles a rate_course request from the client. Saves the logged-in user's rate of a course
 * @param client_fd - socket connected to client
 * @param username - logged-in user's username
 * @return TCP_RECEIVE_ERROR if an error occurred receiving the course number
 * 		   the return value of findCourseNum if an error occurred there
 * 		   IO_ERROR if an error occurred opening the course's ratings file
 * 		   -ERROR if an error occurred receiving the rating or rating text from the client
 * 		   SUCCESS if the course does not exist, or the rating was added successfully
 */
int rateCourse(int client_fd, char* username) {
	int courseNum = receivePositiveInt(client_fd);
	if (courseNum == -ERROR) {
		printf("ERROR: can't receive courseNum from client %d\n", client_fd);
		return TCP_RECEIVE_ERROR;
	}

	int result = findCourseNum(courseNum);
	if (result < 0) {
		// courseNum doesn't exist
		sendPositiveInt(client_fd, ERROR);
		return SUCCESS;
	}

	if (result > 0) { // error occurred
		return result;
	}

	sendPositiveInt(client_fd, SUCCESS);

	char courseFilePath[strlen(dir_path) + 10];
	memset(&courseFilePath, 0, sizeof(courseFilePath));
	sprintf((char*) &courseFilePath, "%s/%d.rating", dir_path, courseNum);
	FILE* courseFile = fopen(courseFilePath, "a+");
	if (courseFile == NULL) {
		printf("ERROR: can't open %s\n", courseFilePath);
		return IO_ERROR;
	}

	int ratingValue = receivePositiveInt(client_fd);
	if (ratingValue == -ERROR) {
		printf("ERROR: can't receive ratingValue from client %d\n", client_fd);
		fclose(courseFile);
		return -ERROR;
	}

	char ratingText[MAXIMUM_RATING_TEXT_LENGTH + 10] = {0};
	if (receiveString(client_fd, (char*) &ratingText) < 0) {
		printf("ERROR: can't receive rating text from client %d\n", client_fd);
		fclose(courseFile);
		return -ERROR;
	}

	fprintf(courseFile, "%s:\t%d\t%s\n", username, ratingValue, ratingText);
	fclose(courseFile);
	return SUCCESS;
}

/***
 * Handles a get_rate request from the client. Sends all ratings of requested course to client.
 * @param client_fd - socket connected to the client
 * @return TCP_RECEIVE_ERROR if an error occurred receiving course number from client
 * 		   SUCCESS if the course does not exist
 * 		   the return value of findCourseNum if an error occurred there
 * 		   IO_ERROR if an error occurred opening the course's ratings file
 * 		   the return value of sendFile on the course's ratings file otherwise
 */
int getRate(int client_fd) {
	// receive courseNum from client
	int courseNum;
	if ((courseNum = receivePositiveInt(client_fd)) < 0) {
		printf("ERROR: can't receive courseNum from client\n");
		return TCP_RECEIVE_ERROR;
	}

	// check courseNum exists
	int courseExist = findCourseNum(courseNum);
	if (courseExist < 0) {
		printf("ERROR: course %d doesn't exist\n", courseNum);
		sendPositiveInt(client_fd, ERROR);
		return SUCCESS;
	}
	if (courseExist > 0) { // error occurred
		return courseExist;
	}
	sendPositiveInt(client_fd, SUCCESS);

	// open course file
	char courseFilePath[strlen(dir_path) + 10];
	memset(&courseFilePath, 0, sizeof(courseFilePath));
	sprintf((char*) &courseFilePath, "%s/%d.rating", dir_path, courseNum);
	FILE* courseFile = fopen((char*) &courseFilePath, "a+");
	if (courseFile == NULL) {
		printf("ERROR: can't open course %d file\n", courseNum);
		return IO_ERROR;
	}

	// send entire file to client, line by line
	int result = sendFile(client_fd, courseFile, MAXIMUM_RATING_TEXT_LENGTH + 10, (char*) &courseFilePath);
	return result;
}

/***
 * Until a quit request is received, handles client commands in a loop
 * @param client_fd - socket connected to client
 * @param username - logged-in user's username
 * @return SUCCESS if the client sends a quit request, or a network error occurred
 * 		   exits the server with exit code 1 if an IO_ERROR occurred
 */
int HandleCommands(int client_fd, char* username) {
	int command;
	int result;
	while (1) {
		command = receivePositiveInt(client_fd);
		switch (command) {
			case LIST_OF_COURSES:
				result = listAllCourses(client_fd);
				break;
			case ADD_COURSE:
				result = addCourse(client_fd);
				break;
			case RATE_COURSE:
				result = rateCourse(client_fd, username);
				break;
			case GET_RATE:
				result = getRate(client_fd);
				break;
			case QUIT:
				close(client_fd);
				free(username);
				return SUCCESS;
			default:
				break;
		}

		if (result == IO_ERROR) {
			//can't open server files or memory allocation error
			close(client_fd);
			free(course_list_path);
			exit(1);
		}
		if (result == TCP_RECEIVE_ERROR || result == TCP_SEND_ERROR) {
			close(client_fd);
			free(username);
			return SUCCESS; //accept other connections
		}
	}
}

/***
 * Initializes the server and begins accepting incoming connections in a loop, one at a time
 */
void StartListening() {
	int socket_fd = socket(PF_INET, SOCK_STREAM, 0); // set socket to (IPv4, TCP, default protocol)
	if (socket_fd < 0) {
		printf("ERROR: can't create socket\n");
		return;
	}

	socklen_t adrrsize = sizeof(server_addr);
	if (bind(socket_fd, (struct sockaddr*) &server_addr, adrrsize)) {
		printf("ERROR: can't bind socket to server_addr\n");
		return;
	}

	if (listen(socket_fd, 10)) {
		printf("ERROR: listen() failed\n");
		return;
	}

	struct sockaddr_in client_addr;
	socklen_t client_addrsize = sizeof(client_addr);

	while (1) {
		int client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addrsize);
		if (client_fd < 0) {
			printf("ERROR: can't accept new connection\n");
			return;
		}

		char* username = GetAndValidateUsername(client_fd);
		HandleCommands(client_fd, username);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("ERROR: missing user_file or dir_path arguments");
		return -1;
	}

	char* users_file = argv[1];
	InitUsers(users_file);
	dir_path = argv[2];
	InitServerFolder();

	if (argc > 3) {
		listen_port = atoi(argv[3]);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(listen_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	StartListening();
	free(course_list_path);
	return 0;
}
