#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "seker_helpers.h"

#define min(a,b) a<b ? a : b
#define DEFAULT_PORT 1337
struct user_details{
    char userName[MAXIMUM_USERNAME_LENGTH];
    char password[MAXIMUM_USERNAME_LENGTH];
};


struct user_details Users[MAXIMUM_NUMBER_OF_USERS];
int users_count;
char *dir_path;
int listen_port = DEFAULT_PORT;
struct sockaddr_in server_addr;


void InitUsers(char *users_file){
    FILE *fp = fopen(users_file, "r");
    if(fp == NULL){
        printf("ERROR: can't open file %s", users_file);
        exit(1);
    }
    char *buffer;
    size_t length = 0;
    int bytesread;
    int i=0;
    while(i < MAXIMUM_NUMBER_OF_USERS){
        if((bytesread = getline(&buffer, &length, fp)) < 0){
            break;
        }
        buffer[bytesread-1] = '\0'; //remove the \n character
        char *username = strtok(buffer, "\t");
        strncpy(Users[i].userName, username, MAXIMUM_USERNAME_LENGTH);
        char *password = strtok(NULL, "\t");
        strncpy(Users[i].password, password, MAXIMUM_USERNAME_LENGTH);
        Users[i].userName[MAXIMUM_USERNAME_LENGTH-1]='\0';
        Users[i].password[MAXIMUM_USERNAME_LENGTH-1]='\0';
        i++;
    }
    users_count = i;
    fclose(fp);
    free(buffer);
}

/*receives username and password from the client, validates and returns the username*/
char * GetAndValidateUsername(int client_fd){
	//send greeting
	uint32_t success = htonl(SUCCESS);
	int success_len = sizeof(success);
	sendAll(client_fd, (char*)&success, &success_len);

	//read user name length and actual user name
	uint32_t str_len;
	int len = sizeof(str_len);
	char *username = malloc(MAXIMUM_USERNAME_LENGTH+1);
	if(receiveAll(client_fd, (char*)&str_len, &len)){
		printf("ERROR: can't read username length");
		free(username);
		return NULL;
	}
	int username_len = min(ntohl(str_len), MAXIMUM_USERNAME_LENGTH);
	if(receiveAll(client_fd, username, &username_len)){
		printf("ERROR: can't read full username");
		free(username);
		return NULL;
	}

	//read password length and actual password
	len = sizeof(str_len);
	if(receiveAll(client_fd, (char*)&str_len, &len)){
		printf("ERROR: can't read password length");
		free(username);
		return NULL;
	}
	char password[MAXIMUM_USERNAME_LENGTH+1];
	int password_len = min(ntohl(str_len), MAXIMUM_USERNAME_LENGTH);
	if(receiveAll(client_fd, (char*)&password, &password_len)){
		printf("ERROR: can't read full password");
		free(username);
		return NULL;
	}

	//find and validate username and password
	int i;
	for(i=0; i<users_count; i++){
		if(strcmp(Users[i].userName, username)==0){
			if(strcmp(Users[i].password, password)==0){
				sendAll(client_fd, (char*)&success, &success_len);
				return username;
			}
		}
	}
	//username or password don't match
	success = htonl(ERROR);
	sendAll(client_fd, (char*)&success, &success_len);
	free(username);
	return NULL;
}

/*waits for new connections*/
void StartListening(){
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0); //set socket to (IPv4, TCP, default protocol)
    if(socket_fd<0){
        printf("ERROR: can't create socket");
        return;
    }
    socklen_t adrrsize = sizeof(server_addr);
    if(bind(socket_fd, (struct sockaddr*)&server_addr, adrrsize)){
        printf("ERROR: can't bind socket to server_addr");
        return;
    }
    if(listen(socket_fd, 10)){
        printf("ERROR: listen() failed");
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrsize = sizeof(client_addr);

    while(1){
        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addrsize);
        if(client_fd<0){
            printf("ERROR: can't accept new connection");
            return;
        }
        char *username = GetAndValidateUsername(client_fd);
        if(username == NULL){
        	close(client_fd);
        }
        HandleCommands(username);
    }

    close(socket_fd);
    return;
}


/*receive commands from client and responses accordingly*/
void HandleCommands(char *username){

}

int main(int argc, char **argv)
{
	if(argc<2){
	        printf("ERROR: missing user_file or dir_path arguments");
	        return -1;
	    }
	    char *users_file = argv[1];
	    InitUsers(users_file);
	    dir_path = argv[2];
	    if(argc>2){
	        listen_port = atoi(argv[3]);
	    }

	    server_addr.sin_family = AF_INET;
	    server_addr.sin_port = htons(listen_port);
	    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	    StartListening();
	    return 0;
}











