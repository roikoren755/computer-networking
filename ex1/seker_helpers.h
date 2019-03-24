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

int receiveAll(int socketFd, char* buffer, int* length);

int sendAll(int socketFd, char* buffer, int* length);

#endif //PROJECT_SEKER_HELPERS_H
