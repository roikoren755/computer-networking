#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include "seker_helpers.h"

#define min(a,b) a<b ? a : b
#define IO_ERROR 2
#define TCP_RECEIVE_ERROR 3
#define TCP_SEND_ERROR 4

struct user_details{
    char userName[MAXIMUM_USERNAME_LENGTH];
    char password[MAXIMUM_USERNAME_LENGTH];
};


struct user_details Users[MAXIMUM_NUMBER_OF_USERS];
int users_count;
char *dir_path;
char *course_list_path;
int listen_port = DEFAULT_PORT;
struct sockaddr_in server_addr;

/*read users_file and stores all usernames and passwords in Users*/
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

void InitServerFolder(){
	//remove previous files if exist
	char removeFiles[20 + strlen(dir_path)];
	if(dir_path[0]=='/') //absolut path
		sprintf((char*)&removeFiles, "exec rm -r %s/*", dir_path);
	else //relative path
		sprintf((char*)&removeFiles, "exec rm -r ./%s/*", dir_path);
	system(removeFiles);

	//create course_list file
	course_list_path = malloc(strlen(dir_path) + 20);
	if(course_list_path==NULL){
		printf("ERROR: can't allocate course_list_path string");
		return;
	}
	sprintf(course_list_path, "%s/courselist", dir_path);
	FILE *courselist = fopen(course_list_path, "w+");
	if(courselist==NULL){
		printf("ERROR: can't create file %s", course_list_path);
		return;
	}
	fclose(courselist);
}

/*receive username and password from the client, validate and return the username*/
char * GetAndValidateUsername(int client_fd){
	char *username = calloc(sizeof(char), MAXIMUM_USERNAME_LENGTH+1);
	char password[MAXIMUM_USERNAME_LENGTH+1]={0};
	if(username == NULL){
		printf("ERROR: can't allocate username memory\n");
		return NULL;
	}

	//send greeting
	sendPositiveInt(client_fd, SUCCESS);
	//get username and password
	receiveString(client_fd, username);
	receiveString(client_fd, (char*)&password);
	//find and validate username and password
	int i;
	for(i=0; i<users_count; i++){
		if(strcmp(Users[i].userName, username)==0){
			if(strcmp(Users[i].password, password)==0){
				sendPositiveInt(client_fd, SUCCESS);
				return username;
			}
		}
	}
	//username or password don't match
	sendPositiveInt(client_fd, ERROR);
	free(username);
	return NULL;
}

/*send entire file to client, line by line*/
int sendFile(int client_fd, FILE * file, int bufferLen, char *filename){
	size_t length = bufferLen;
	char *buffer = calloc(sizeof(char), length);
	if(!buffer){
		printf("ERROR: can't allocate buffer for %s\n", filename);
		fclose(file);
		return IO_ERROR;
	}
	int bytesread;
	while(1){
		if((bytesread = getline(&buffer, &length, file)) < 0){
			break;
		}
		if(sendString(client_fd, buffer)){
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

/*return SUCCESS if courseNum exists,
 *-1 if courseNum doesn't exist
 *or an error number*/
int findCourseNum(int courseNum){
	FILE *courseList = fopen(course_list_path, "a+");
	if(courseList == NULL){
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}
	char buffer[MAXIMUM_COURSE_NAME_LENGTH+10]={0};
	char *currCourseNum;
	size_t length;
	char *bufferAddr = (char*)&buffer;
	while(getline(&bufferAddr, &length, courseList) > 0){
		currCourseNum = strtok(bufferAddr, ":");
		if(atoi(currCourseNum) == courseNum){
			fclose(courseList);
			return SUCCESS;
		}
	}
	//course doesn't exist
	fclose(courseList);
	return -1;
}

int listAllCourses(int client_fd){
	FILE *courselist = fopen(course_list_path, "r");
	if(courselist == NULL){
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}
	//send entire courselist file to client, line by line
	int result = sendFile(client_fd, courselist, MAXIMUM_COURSE_NAME_LENGTH +10, "courselist");
	return result;
}


/*receive courseNum from the client. send ERROR if courseNum exists
 * if not, receive course name from the client and add to courseList
 */
int addCousre(int client_fd){
	int courseNum = receivePositiveInt(client_fd);

	int result = findCourseNum(courseNum);
	if(result==SUCCESS){
		//courseNum already exist
		printf("course num %d already exist", courseNum);
		sendPositiveInt(client_fd, ERROR);
		return SUCCESS;
	}
	if(result > 0){
		//error occurred
		return result;
	}

	//course number doesn't exist in the list
	FILE *courseList = fopen(course_list_path, "a+");
	if(courseList==NULL){
		printf("ERROR: can't open %s\n", course_list_path);
		return IO_ERROR;
	}
	sendPositiveInt(client_fd, SUCCESS);
	char buffer[MAXIMUM_COURSE_NAME_LENGTH+10]={0};
	receiveString(client_fd, (char*)&buffer);
	fprintf(courseList, "%d:\t%s\n", courseNum, buffer);
	fclose(courseList);

	return SUCCESS;
}

int rateCourse(int client_fd, char *username){
	int courseNum = receivePositiveInt(client_fd);
	if(courseNum==-ERROR){
		printf("ERROR: can't receive courseNum from client %d\n", client_fd);
		return TCP_RECEIVE_ERROR;
	}
	int result = findCourseNum(courseNum);
	if(result < 0){
		//courseNum doesn't exist
		sendPositiveInt(client_fd, ERROR);
		return SUCCESS;
	}
	if( result > 0) //error occurred
		return result;
	else
		sendPositiveInt(client_fd, SUCCESS);

	char courseFilePath[strlen(dir_path)+10];
	memset(&courseFilePath, 0, sizeof(courseFilePath));
	sprintf((char*)&courseFilePath, "%s/%d", dir_path, courseNum);
	FILE *courseFile = fopen(courseFilePath, "a+");
	if(courseFile==NULL){
		printf("ERROR: can't open %s\n", courseFilePath);
		return IO_ERROR;
	}
	int ratingValue = receivePositiveInt(client_fd);
	if(ratingValue==-ERROR){
		printf("ERROR: can't receive ratingValue from client %d\n", client_fd);
		fclose(courseFile);
		return -ERROR;
	}
	char ratingText[MAXIMUM_RATING_TEXT_LENGTH+10]={0};
	if(receiveString(client_fd, (char*)&ratingText)<0){
		printf("ERROR: can't receive rating text from client %d\n", client_fd);
		fclose(courseFile);
		return -ERROR;
	}
	fprintf(courseFile, "%s:\t%d\t%s\n", username, ratingValue, ratingText);
	fclose(courseFile);
	return SUCCESS;
}

int getRate(int client_fd){
	//receive courseNum from client
	int courseNum;
	if((courseNum = receivePositiveInt(client_fd)) < 0){
		printf("ERROR: can't receive courseNum from client\n");
		return TCP_RECEIVE_ERROR;
	}
	//check courseNum exists
	int courseExist = findCourseNum(courseNum);
	if(courseExist < 0){
		printf("ERROR: course %d doesn't exist\n", courseNum);
		return SUCCESS;
	}
	if(courseExist > 0){ //error occurred
		return courseExist;
	}
	//open course file
	char courseFilePath[strlen(dir_path)+10];
	memset(&courseFilePath, 0, sizeof(courseFilePath));
	sprintf((char*)&courseFilePath, "%s/%d", dir_path, courseNum);
	FILE *courseFile = fopen((char*)&courseFilePath, "a+");
	if(courseFile == NULL){
		printf("ERROR: can't open course %d file\n", courseNum);
		return IO_ERROR;
	}
	//send entire file to client, line by line
	int result = sendFile(client_fd, courseFile, MAXIMUM_RATING_TEXT_LENGTH + 10, (char*)&courseFilePath);
	return result;
}

/*receive commands from client and response accordingly*/
int HandleCommands(int client_fd, char *username){
	int command;
	int result;
	while(1){
		command = receivePositiveInt(client_fd);
		switch(command){
			case LIST_OF_COURSES:
				if((result = listAllCourses(client_fd)))
					return result;
				break;
			case ADD_COURSE:
				if((result = addCousre(client_fd)))
					return result;
				break;
			case RATE_COURSE:
				if((result = rateCourse(client_fd, username)))
					return result;
				break;
			case GET_RATE:
				if((result = getRate(client_fd)))
					return result;
				break;
			case QUIT:
				close(client_fd);
				free(username);
				return SUCCESS;
			default:
				break;
		}
	}
	return 0;
}

/*accepting new connections*/
void StartListening(){
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0); //set socket to (IPv4, TCP, default protocol)
    if(socket_fd<0){
        printf("ERROR: can't create socket\n");
        return;
    }
    socklen_t adrrsize = sizeof(server_addr);
    if(bind(socket_fd, (struct sockaddr*)&server_addr, adrrsize)){
        printf("ERROR: can't bind socket to server_addr\n");
        return;
    }
    if(listen(socket_fd, 10)){
        printf("ERROR: listen() failed\n");
        return;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrsize = sizeof(client_addr);

    while(1){
        int client_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_addrsize);
        if(client_fd<0){
            printf("ERROR: can't accept new connection\n");
            return;
        }
        char *username = GetAndValidateUsername(client_fd);
        if(username == NULL){
        	printf("ERROR: username or password are incorrect\n");
        	close(client_fd);
        }
        int result = HandleCommands(client_fd, username);
        if(result){
        	close(client_fd);
        	free(username);
        }
    }

    close(socket_fd);
    return;
}

int main(int argc, char **argv)
{
	if(argc<3){
	        printf("ERROR: missing user_file or dir_path arguments");
	        return -1;
	    }
	    char *users_file = argv[1];
	    InitUsers(users_file);
	    dir_path = argv[2];
	    InitServerFolder();

	    if(argc>3){
	        listen_port = atoi(argv[3]);
	    }

	    server_addr.sin_family = AF_INET;
	    server_addr.sin_port = htons(listen_port);
	    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	    StartListening();
	    return 0;
}











