//
// Created by Roi Koren on 2019-03-24.
//

#ifndef PROJECT_SEKER_HELPERS_H
#define PROJECT_SEKER_HELPERS_H

#define LIST_OF_COURSES 1
#define ADD_COURSE 2
#define RATE_COURSE 3
#define GET_RATE 4
#define SUCCESS 0
#define ERROR 1
#define MAXIMUM_RATING_TEXT_LENGTH 1024
#define MAXIMUM_COURSE_NAME_LENGTH 100
#define MAXIMUM_USERNAME_LENGTH 15
#define MAXIMUM_NUMBER_OF_USERS 25
#define DEFAULT_PORT 1337

/***
 * Receive *length bytes of data from socketFd, and store them in buffer.
 * @param socketFd - open socket from which to receive data
 * @param buffer - buffer to store data in, of at least *length size
 * @param length - pointer to length of data to receive
 * @return ERROR if an error occurs
 * 		   SUCCESS otherwise
 */
int receiveAll(int socketFd, char* buffer, int* length);

/***
 * Send *length bytes from buffer, via socketFd.
 * @param socketFd - open socket to send data through
 * @param buffer - buffer containing data to send
 * @param length - pointer to length of data to send
 * @return ERROR if an error occurs
 * 		   SUCCESS otherwise
 */
int sendAll(int socketFd, char* buffer, int* length);

/***
 * Sends string pointed at by toSend, through socketFd.
 * This function first sends the string's length, then the string itself.
 * @param socketFd - open socket through which to send the data
 * @param toSend - NULL-terminated string to send
 * @return ERROR if an error occurs
 * 		   SUCCESS otherwise
 */
int sendString(int socketFd, char* toSend);

/***
 * Sends a positive integer through socketFd.
 * @param socketFd - open socket through which to send the data
 * @param toSend - positive integer to send
 * @return ERROR if an error occurs
 * 		   SUCCESS otherwise
 */
int sendPositiveInt(int socketFd, int toSend);

/***
 * Receives a string from socketFd, and stores is in buffer
 * First receives the string's length from the socket, then the string itself.
 * Assumes the string will not overflow the buffer (***FINGERS CROSSED!***)
 * @param socketFd - open socket through which to receive the data
 * @param buffer - buffer to store the data in. Should be larger than the maximum expected string's length
 * @return ERROR if an error occurs
 * 		   SUCCESS otherwise
 */
int receiveString(int socketFd, char* buffer);

/***
 * Receives a positive integer through socketFd.
 * @param socketFd
 * @return -ERROR if an error occurs
 * 		   positive integer sent through socketFd otherwise
 */
int receivePositiveInt(int socketFd);

/***
 * Sets the last char in the string different from '\0' to '\0'
 * @param string - string to terminate early
 */
void removeTrailingChar(char* string);

#endif //PROJECT_SEKER_HELPERS_H
