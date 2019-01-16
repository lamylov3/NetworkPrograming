#include <stdio.h>          /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

#include "protocol.h"
#include "authenticate.h"
#include "validate.h"
#include "status.h"

#define PORT 5550   /* Port that will be opened */ 
#define BACKLOG 2   /* Number of allowed connections */
#define MAX_SIZE 10e6 * 100
#define STORAGE "./storage/" //default save file place

#define BUFF_SEND 1024
#define PRIVATE_KEY 256
pthread_mutex_t lock;
int requestId = 1;

Client onlineClient[1000];

void initArrayClient() {
	int i;
	for(i = 0; i < 1000; i++) {
		onlineClient[i].requestId = 0;
	}
}
// count number param of command
int numberElementsInArray(char** temp) {
	int i;
	for (i = 0; *(temp + i); i++)
    {
        // count number elements in array
    }
    return i;
}
void printListOnlineClient() {
	int i;
	for (i = 0; i < 1000; i++) {
		if(onlineClient[i].requestId > 0) {
			printf("\n---ConnSock---: %d\n", onlineClient[i].connSock);
			printf("---RequestId---: %d\n", onlineClient[i].requestId);
			printf("---Username---: %s\n", onlineClient[i].username);
		}
	}
}

int findAvaiableElementInArrayClient() {
	int i;
	for (i = 0; i < 1000; i++) {
		if(onlineClient[i].requestId == 0) {
			return i;
		}
	}
	return -1;
}

int findClient(int requestId) {
	int i;
	for (i = 0; i < 1000; i++) {
		if(onlineClient[i].requestId == requestId) {
			return i;
		}
	}
	return -1;
}

int findClientByUsername(char* username) {
	int i;
	for (i = 0; i < 1000; i++) {
		if(!strcmp(onlineClient[i].username, username) && (onlineClient[i].requestId > 0)) {
			return i;
		}
	}
	return -1;
}

void setClient(int i, int requestId, char* username) {
	if(i >= 0) {
		onlineClient[i].requestId = requestId;
		strcpy(onlineClient[i].username, username);
	} else {
		printf("Full Client, Service not avaiable!!\n");
	}
}

void handleLogin(Message mess, int connSock) {
	char** temp = str_split(mess.payload, '\n');
	StatusCode loginCode;
	//User* curUser = NULL;
	if(numberElementsInArray(temp) == 3) {
		char** userStr = str_split(temp[1], ' ');
		char** passStr = str_split(temp[2], ' ');
		if((numberElementsInArray(userStr) == 2) && (numberElementsInArray(passStr) == 2)) {
			if(!(strcmp(userStr[0], COMMAND_USER) || strcmp(passStr[0], COMMAND_PASSWORD))) {
				char username[30];
				char password[20];
				strcpy(username, userStr[1]);
				strcpy(password, passStr[1]);
				if(validateUsername(username) || validatePassword(password)) {
					loginCode = login(username, password);
					if(loginCode != LOGIN_SUCCESS)
						mess.type = TYPE_ERROR;
					else{
						if(mess.requestId == 0) {
							mess.requestId = requestId++;
							int i = findAvaiableElementInArrayClient();
							setClient(i, mess.requestId, username);
						}
					}
				} else {
					mess.type = TYPE_ERROR;
					loginCode = USERNAME_OR_PASSWORD_INVALID;
				}
			}
			else{
				loginCode = COMMAND_INVALID;
				mess.type=TYPE_ERROR;
			}
		}
		else{
			loginCode = COMMAND_INVALID;
			mess.type=TYPE_ERROR;
		}
	}
	else {
		mess.type=TYPE_ERROR;
		loginCode = COMMAND_INVALID;
		printf("Fails on handle Login!!");
	}
	sendWithCode(mess, loginCode, connSock);
}

void handleRegister(Message mess, int connSock){
	char** temp = str_split(mess.payload, '\n');
	StatusCode registerCode;
	if(numberElementsInArray(temp) == 3) {
		char** userStr = str_split(temp[1], ' ');
		char** passStr = str_split(temp[2], ' ');
		if((numberElementsInArray(userStr) == 2) && (numberElementsInArray(passStr) == 2)) {
			if(!(strcmp(userStr[0], COMMAND_USER) || strcmp(passStr[0], COMMAND_PASSWORD))) {
				char username[30];
				char password[20];
				strcpy(username, userStr[1]);
				strcpy(password, passStr[1]);
				if(validateUsername(username) || validatePassword(password)) {
					registerCode = registerUser(username, password);
					if(registerCode != REGISTER_SUCCESS)
						mess.type=TYPE_ERROR;
					else {
						if(mess.requestId == 0) {
							mess.requestId = requestId++;
							int i = findAvaiableElementInArrayClient();
							setClient(i, mess.requestId, username);
						}
					}
				} else {
					mess.type = TYPE_ERROR;
					registerCode = USERNAME_OR_PASSWORD_INVALID;
				}
			}
			else{
				registerCode = COMMAND_INVALID;
				mess.type=TYPE_ERROR;
			}
		}
		else{
			registerCode = COMMAND_INVALID;
			mess.type=TYPE_ERROR;
		}
	}
	else {
		mess.type=TYPE_ERROR;
		registerCode = COMMAND_INVALID;
		printf("Fails on handle Register!!");
	}
	sendWithCode(mess, registerCode, connSock);
}

void handleLogout(Message mess, int connSock){
	char** temp = str_split(mess.payload, '\n');
	StatusCode logoutCode;
	if(numberElementsInArray(temp) != 2) {
		mess.type = TYPE_ERROR;
		logoutCode = COMMAND_INVALID;
		printf("Fails on handle logout!!");
	}
	else{
		logoutCode = logoutUser(temp[1]);
		if(logoutCode == LOGOUT_SUCCESS) {
			int i = findClient(mess.requestId);
			if(i >= 0) {
				onlineClient[i].requestId = 0;
				onlineClient[i].username[0] = '\0';
			}
		}
	}
	sendWithCode(mess, logoutCode, connSock);
}

void handleAuthenticateRequest(Message mess, int connSock) {
	char* payloadHeader;
	char temp[PAYLOAD_SIZE];
	strcpy(temp, mess.payload);
	payloadHeader = getHeaderOfPayload(temp);
	if(!strcmp(payloadHeader, LOGIN_CODE)) {
		handleLogin(mess, connSock);
	} else if (!strcmp(payloadHeader, REGISTER_CODE)) {
		handleRegister(mess, connSock);

	} else if(!strcmp(payloadHeader, LOGOUT_CODE)) {
		handleLogout(mess, connSock);
	}
}

char* searchFileInOnlineClients(char* fileName, int requestId, char* listUser) {
	int i;
	Message msg, recvMsg;
	msg.requestId = requestId;
	char user[200];
	strcpy(msg.payload, fileName);
	msg.length = strlen(msg.payload);
	msg.type = TYPE_REQUEST_FILE;
	for(i = 0; i < 1000; i++) {
		if((onlineClient[i].requestId > 0) && (onlineClient[i].requestId != requestId)) {
			sendMessage(onlineClient[i].connSock, msg);
			receiveMessage(onlineClient[i].connSock, &recvMsg);
			if(recvMsg.type != TYPE_ERROR) {
				sprintf(user, "%s %s", onlineClient[i].username, recvMsg.payload);
				strcat(listUser, user);
				strcat(listUser, "\n");
			}
		}
	}
	if(strlen(listUser) > 0) {
		listUser[strlen(listUser) - 1] = '\0';
	}
	return listUser;
}

void handleRequestFile(Message recvMess, int connSock) {
	//printMess(recvMess);
	char fileName[100];
	char listUser[2000] = "";
	strcpy(fileName, recvMess.payload);
	int requestId = recvMess.requestId;
	searchFileInOnlineClients(fileName, requestId, listUser);
	//printf("List: %s\n", listUser);
	Message msg;
	msg.requestId = recvMess.requestId;
	msg.type = TYPE_REQUEST_FILE;
	strcpy(msg.payload, listUser);
	msg.length = strlen(msg.payload);
	sendMessage(connSock, msg);
}

void addClientSocket(int id, int connSock) {
	int i = findClient(id);
	if(i >= 0) {
		onlineClient[i].connSock = connSock;
	}
}

void removeFile(char* fileName) {
	// remove file
    if (remove(fileName) == 0)
        printf("Handle Success!!! %s file deleted successfully.\n", fileName);
    else
    {
        printf("Unable to delete the file\n");
        perror("Following error occurred\n");
    }
}


int sendRequestDownload(int requestId, char* selectedUser, char* fileName, int connSock) {
	int i = findClientByUsername(selectedUser);
	char tmpFileName[100];
	FILE *tmpFile;
	Message msg, recvMsg, sendMess;
	fflush(stdout);
	if(i >= 0) {
		msg.type = TYPE_REQUEST_DOWNLOAD;
		strcpy(msg.payload, fileName);
		msg.length = strlen(msg.payload);
		msg.requestId = onlineClient[i].requestId;
		sendMessage(onlineClient[i].connSock, msg);
		sprintf(tmpFileName, "%lu", (unsigned long)time(NULL));
		if((tmpFile = fopen(tmpFileName, "wb+")) == NULL) {
			msg.type = TYPE_ERROR;
			sendMessage(connSock, msg);
			perror("You have not create file permission!!\n");
			return -1;
		}
		pthread_mutex_lock(&lock);
		while(1) {
			if(receiveMessage(onlineClient[i].connSock, &recvMsg) < 0) {
				break;
			}
			if(recvMsg.type == TYPE_ERROR) {
				sendMessage(connSock, recvMsg);
				break;
			}
			if(recvMsg.length > 0) {
				fwrite(recvMsg.payload, 1, recvMsg.length, tmpFile);
			}
			else if(recvMsg.length == 0) {
				fseek(tmpFile, 0, SEEK_SET);
				break;
			}
		}
		pthread_mutex_unlock(&lock);
		if(recvMsg.type == TYPE_ERROR) {
			fclose(tmpFile);
       		removeFile(tmpFileName);
			return -1;
		}
		while(!feof(tmpFile)) {
			fflush(stdout);
			//printf("11111111");
            char buffer[PAYLOAD_SIZE];
           	int bytes_send = fread(buffer, 1, PAYLOAD_SIZE, tmpFile);
           	if(bytes_send <= 0) {
           		break;
           	}
            sendMess.length = bytes_send;
            sendMess.requestId = requestId;
            memcpy(sendMess.payload, buffer, bytes_send);
            sendMessage(connSock, sendMess);
        }

        sendMess.length = 0;
        sendMessage(connSock, sendMess);
        fclose(tmpFile);
       	removeFile(tmpFileName);
	} else {
		msg.type = TYPE_ERROR;
		sendMessage(connSock, msg);
		return 1;
	}
	return 1;
}
void handleRequestDownload(Message recvMess, int connSock) {
	char** temp = str_split(recvMess.payload, '\n');
	char selectedUser[30];
	char fileName[30];
	//printMess(recvMess);
	MessageType type = TYPE_ERROR;
	if(numberElementsInArray(temp) == 2) {
		strcpy(selectedUser, temp[0]);
		strcpy(fileName, temp[1]);
		sendRequestDownload(recvMess.requestId, selectedUser, fileName, connSock);
	} else {
		type = TYPE_ERROR;
	}

}

/*
* Handler Request from Client
* @param char* message, int key
* return void*
*/
void* client_handler(void* conn_sock) {
	int connSock;
	connSock = *((int *) conn_sock);
	Message recvMess;
	pthread_detach(pthread_self());
	while(1) {
		//receives message from client
		if(receiveMessage(connSock, &recvMess) < 0) {
			break;
		}
		//blocking
		switch(recvMess.type) {
			case TYPE_AUTHENTICATE: 
				handleAuthenticateRequest(recvMess, connSock);
				break;
			case TYPE_BACKGROUND: 
				addClientSocket(recvMess.requestId, connSock);
				//printListOnlineClient();
				return NULL;

			case TYPE_REQUEST_FILE: 
				handleRequestFile(recvMess, connSock);
				break;
			case TYPE_REQUEST_DOWNLOAD: 
				handleRequestDownload(recvMess, connSock);
				break;
			case TYPE_UPLOAD_FILE: 
				break;
			case TYPE_ERROR: 
				break;
			default: break;
		}

	}

	return NULL;
}

int main(int argc, char **argv)
{
 	int port_number;
 	int listen_sock, conn_sock; /* file descriptors */
	struct sockaddr_in server; /* server's address information */
	struct sockaddr_in client; /* client's address information */
	int sin_size;
	pthread_t tid;
		
 	if(argc != 2) {
 		perror(" Error Parameter! Please input only port number\n ");
 		exit(0);
 	}
 	if((port_number = atoi(argv[1])) == 0) {
 		perror(" Please input port number\n");
 		exit(0);
 	}
 	if(!validPortNumber(port_number)) {
 		perror("Invalid Port Number!\n");
 		exit(0);
 	}
	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  /* calls socket() */
		perror("\nError: ");
		return 0;
	}
	
	//Step 2: Bind address to socket
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;         
	server.sin_port = htons(port_number);   /* Remember htons() from "Conversions" section? =) */
	server.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY puts your IP address automatically */   
	if(bind(listen_sock, (struct sockaddr*)&server, sizeof(server)) == -1){ /* calls bind() */
		perror("\nError: ");
		return 0;
	}     
	
	//Step 3: Listen request from client
	if(listen(listen_sock, BACKLOG) == -1){  /* calls listen() */
		perror("\nError: ");
		return 0;
	}
	initArrayClient();
	readFile();
	printList();
	//Step 4: Communicate with client
	while(1) {
		//accept request
		sin_size = sizeof(struct sockaddr_in);
		if ((conn_sock = accept(listen_sock,( struct sockaddr *)&client, (unsigned int*)&sin_size)) == -1) 
			perror("\nError: ");
  
		printf("You got a connection from %s\n", inet_ntoa(client.sin_addr) ); /* prints client's IP */
		if (pthread_mutex_init(&lock, NULL) != 0) 
	    { 
	        printf("\n mutex init has failed\n"); 
	        return 1; 
	    } 
		//start conversation
		pthread_create(&tid, NULL, &client_handler, &conn_sock);	
	}
	
	close(listen_sock);
	return 0;
}