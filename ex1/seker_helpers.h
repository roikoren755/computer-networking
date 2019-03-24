//
// Created by Roi Koren on 2019-03-24.
//

#ifndef PROJECT_SEKER_HELPERS_H
#define PROJECT_SEKER_HELPERS_H

int receiveAll(int socketFd, char* buffer, int* length);

int sendAll(int socketFd, char* buffer, int* length);

#endif //PROJECT_SEKER_HELPERS_H
