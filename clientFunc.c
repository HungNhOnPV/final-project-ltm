#include <stdio.h>          /* These are the usual header files */
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <inttypes.h>
#include "protocol.h"
#include "validate.h"
#include "status.h"

#define DATA_SIZE 1000
char current_user[255];
int client_sock;
int under_client_sock;
struct sockaddr_in server_addr; /* server's address information */
pthread_t tid;
int choose;
Message *mess;
int isOnline = 0;
char fileRepository[100];
#define DIM(x) (sizeof(x)/sizeof(*(x)))


/*
* count number param of command
* @param temp
* @return number param
*/
int numberElementsInArray(char** temp) {
	int i = 0;
	while (*(temp + i) != NULL)
	{
		i++;
	}
	return i;
}
/*
* handle show notify
* @param notify
* @return void
*/
void *showBubbleNotify(void *notify){
	char command[200];
	sprintf(command, "terminal-notifier -message \"%p\"", notify);
	// system(command);
	return NULL;
}

/*
* handle bind new socket to server
* @param connect port, serverAddr
* @return void
*/
void bindClient(int port, char *serverAddr){
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(serverAddr);
}

/*
* handle find or create folder as user
* @param username
* @return void
*/
void findOrCreateFolderUsername(char* username) {
	struct stat st = {0};
	if (stat(username, &st) == -1) {
	    mkdir(username, 0700);
	}
	strcpy(fileRepository, username);
}

/*
* handle get path of file to folder of user
* @param filename, fullpath
* @return void
*/
void getFullPath(char* fileName, char* fullPath) {
	sprintf(fullPath, "%s/%s", fileRepository, fileName);
}

/*
* method of socket run on background
* @param
* @return void
*/
void* backgroundHandle() {
	// hanlde request in background
	Message recvMess;
	while(1) {
		//receives message from client
		if(receiveMessage(under_client_sock, &recvMess) < 0) {
			break;
		}

		switch(recvMess.type) {
			case TYPE_REQUEST_FILE:
			{
				Message msg;
                	msg.requestId = recvMess.requestId;
                	msg.length = 0;
                	char fileName[50];
                	strcpy(fileName, recvMess.payload);
                	char fullPath[100];
                	getFullPath(fileName, fullPath);
                	FILE *fptr;
                	fptr = fopen(fullPath, "r");

                	if(fptr != NULL) {
                		msg.type = TYPE_REQUEST_FILE;
                		long filelen;
                	    fseek(fptr, 0, SEEK_END);          // Jump to the end of the file
                	    filelen = ftell(fptr);             // Get the current byte offset in the file
                	    rewind(fptr);
                	    char len[100];
                	    sprintf(len, "%ld", filelen);
                	    strcpy(msg.payload, len);
                	    msg.length = strlen(msg.payload);
                	    fclose(fptr);
                	} else {
                		msg.type = TYPE_ERROR;
                	}
                	sendMessage(under_client_sock, msg);
                	}
				break;
			case TYPE_REQUEST_DOWNLOAD:
				{
					char fileName[30];
                	char fullPath[100];
                	strcpy(fileName, recvMess.payload);
                	getFullPath(fileName, fullPath);
                	Message msg, sendMsg;
                	sendMsg.requestId = recvMess.requestId;
                	FILE* fptr;
                	if ((fptr = fopen(fullPath, "rb+")) == NULL){
                        //printf("Error: File not found\n");
                        fclose(fptr);
                        msg.type = TYPE_ERROR;
                        msg.requestId = recvMess.requestId;
                        msg.length = 0;
                        sendMessage(under_client_sock, msg);
                    }
                    else {
                    	long filelen;
                        fseek(fptr, 0, SEEK_END);          // Jump to the end of the file
                        filelen = ftell(fptr);             // Get the current byte offset in the file
                        rewind(fptr);    // pointer to start of file
                    	// int check = 1;
                        int sumByte = 0;
                    	while(!feof(fptr)) {
                            int numberByteSend = PAYLOAD_SIZE;
                            if((sumByte + PAYLOAD_SIZE) > filelen) {// if over file size
                                numberByteSend = filelen - sumByte;
                            }
                            char* buffer = (char *) malloc((numberByteSend) * sizeof(char));
                            fread(buffer, numberByteSend, 1, fptr); // read buffer with size
                            memcpy(sendMsg.payload, buffer, numberByteSend);
                            sendMsg.length = numberByteSend;
                            sumByte += numberByteSend; //increase byte send
                            //printf("sumByte: %d\n", sumByte);
                            if(sendMessage(under_client_sock, sendMsg) <= 0) {
                                printf("Connection closed!\n");
                                // check = 0;
                                break;
                            }
                            free(buffer);
                            if(sumByte >= filelen) {
                                break;
                            }
                        }
                        sendMsg.length = 0;
                        sendMessage(under_client_sock, sendMsg);
                    }
				}
				break;
			default: break;
		}
	}
	return NULL;
}

// get username and password from keyboard to login
void getLoginInfo(char *str){
	char username[255];
	char password[255];
	printf("Enter username: ");
	scanf("%[^\n]s", username);
	while(getchar() != '\n');
	printf("Enter password: ");
	scanf("%[^\n]s", password);
	while(getchar()!='\n');
	sprintf(mess->payload, "LOGIN\nUSER %s\nPASS %s", username, password);
	strcpy(str, username);
}

/*
* get login info and login
* @param current user
* @return void
*/
void loginFunc(char *current_user){
	char username[255];
	mess->type = TYPE_AUTHENTICATE;
	getLoginInfo(username);
	mess->length = strlen(mess->payload);
	sendMessage(client_sock, *mess);
	receiveMessage(client_sock, mess);
	if(mess->type != TYPE_ERROR){
		isOnline = 1;
		strcpy(current_user, username);
		int newsock = socket(AF_INET, SOCK_STREAM, 0);
        	if (newsock == -1 ){
        		perror("\nError: ");
        		exit(0);
        	}
        	under_client_sock = newsock;
        	if(connect(under_client_sock, (struct sockaddr*) (&server_addr), sizeof(struct sockaddr)) < 0){
            			printf("\nError!Can not connect to sever! Client exit imediately!\n");
            			exit(0);
            		} else {
            			mess->type = TYPE_BACKGROUND;
                        sendMessage(under_client_sock, *mess);
            			pthread_create(&tid, NULL, &backgroundHandle, NULL);
            		}
		findOrCreateFolderUsername(username);
	} else {
		showBubbleNotify("Error: Login Failed!!");
	}
	printf("%s\n", mess->payload);
}

/*
* get register info
* @param user
* @return void
*/
int getRegisterInfo(char *user){
	char username[255], password[255], confirmPass[255];
	printf("Username: ");
	scanf("%[^\n]s", username);
	printf("Password: ");
	while(getchar()!='\n');
	scanf("%[^\n]s", password);
	printf("Confirm password: ");
	while(getchar()!='\n');
	scanf("%[^\n]s", confirmPass);
	while(getchar()!='\n');
	if(!strcmp(password, confirmPass)){
		sprintf(mess->payload, "REGISTER\nUSER %s\nPASS %s", username, password);
		strcpy(user, username);
		return 1;
	}
	else{
		printf("Confirm password invalid!\n");
		return 0;
	}
}

/*
* get register info and login
* @param current user
* @return void
*/
void registerFunc(char *current_user){
	char username[255];
	if(getRegisterInfo(username)){
		mess->type = TYPE_AUTHENTICATE;
		mess->length = strlen(mess->payload);
		sendMessage(client_sock, *mess);
		receiveMessage(client_sock, mess);
		if(mess->type != TYPE_ERROR){
			isOnline = 1;
			strcpy(current_user, username);
			int newsock = socket(AF_INET, SOCK_STREAM, 0);
            	if (newsock == -1 ){
            		perror("\nError: ");
            		exit(0);
            	}
            	under_client_sock = newsock;
            	if(connect(under_client_sock, (struct sockaddr*) (&server_addr), sizeof(struct sockaddr)) < 0){
                			printf("\nError!Can not connect to sever! Client exit imediately!\n");
                			exit(0);
                		} else {
                			mess->type = TYPE_BACKGROUND;
                            sendMessage(under_client_sock, *mess);
                			pthread_create(&tid, NULL, &backgroundHandle, NULL);
                		}
			findOrCreateFolderUsername(username);
		} else {
			showBubbleNotify("Error: Register Failed!!");
		}
		printf("%s\n", mess->payload);
	}
}

/*
* logout from system
* @param current user
* @return void
*/
void logoutFunc(char *current_user){
	mess->type = TYPE_AUTHENTICATE;
	sprintf(mess->payload, "LOGOUT\n%s", current_user);
	mess->length = strlen(mess->payload);
	sendMessage(client_sock, *mess);
	receiveMessage(client_sock, mess);
	if(mess->type != TYPE_ERROR){
		isOnline = 0;
		current_user[0] = '\0';
		close(under_client_sock);
	}
	printf("%s\n", mess->payload);
}

/*
* remove one file
* @param filename
* @return void
*/
void removeFile(char* fileName) {
	// remove file
    if (remove(fileName) != 0)
    {
        perror("Following error occurred\n");
    }
}

/*
* show list user who have file
* @param
* @return void
*/
int showListSelectUser(char* listUser, char* username, char* fileName) {
	if(strlen(listUser) == 0) {
		printf("\n--This File Not Found In System!!\n");
		return -1;
	}
	char** list = str_split(listUser, '\n');
	int i=0;
	printf("\n--------------------------- List User -------------------------------\n");
	printf("Username\t\t\tFile\t\t\tSize\n");
	while (*(list + i)!=NULL)
    {
    	char** tmp = str_split(*(list + i), ' ');
        printf("\n%d. %s\t\t\t%s\t\t\t%s", i + 1, tmp[0], fileName, tmp[1]);
        i++;
    }

    char choose[10];
    int option;
    while(1) {
	    printf("\nPlease select user to download (Press 0 to cancel): ");
	    scanf("%s", choose);
		while(getchar() != '\n');
		option = atoi(choose);
		if((option >= 0) && (option <= i)) {
			break;
		} else {
			printf("Please Select Valid Options!!\n");
		}
	}
	if(option == 0) {
		return -1;
	}
	else {
		char** tmp = str_split(list[option - 1], ' ');
		strcpy(username, tmp[0]);
	}
	return 1;
}

/*
* receive file from server and save
* @param filename, path
* @return void
*/
int download(char* fileName, char* path) {
	Message recvMsg;
	FILE *fptr;
	char tmpFileName[100];
	char fullPath[100];
	char** tmp = str_split(fileName, '.');
	if(numberElementsInArray(tmp) == 1) {
		sprintf(tmpFileName, "%s_%lu", tmp[0], (unsigned long)time(NULL));
	} else{
		sprintf(tmpFileName, "%s_%lu.%s", tmp[0], (unsigned long)time(NULL), tmp[1]);
	}

	getFullPath(tmpFileName, fullPath);
	strcpy(path, fullPath);
	fptr = fopen(fullPath, "w+");
	while(1) {
        receiveMessage(client_sock, &recvMsg);
        if(recvMsg.type == TYPE_ERROR) {
        	fclose(fptr);
        	removeFile(fullPath);
        	return -1;
        }
        if(recvMsg.length > 0) {
            fwrite(recvMsg.payload, recvMsg.length, 1, fptr);
        } else {
            break;
        }
    }
    fclose(fptr);
    return 1;
}


/*
* method download
* @param filename, path
* @return void
*/
void handleDownloadFile(char* selectedUser,char* fileName) {
	Message msg;
	msg.requestId = mess->requestId;
	msg.type = TYPE_REQUEST_DOWNLOAD;
	sprintf(msg.payload, "%s\n%s", selectedUser, fileName);
	msg.length = strlen(msg.payload);
	sendMessage(client_sock, msg);
	printf("......................Donwloading.....................\n");
	char path[1000];
	if(download(fileName, path) == -1) {
		showBubbleNotify("Error: Something Error When Downloading File!!");
		printf("Error: Something Error When Downloading File!!\n");
		return;
	}
	char message[2000];
	sprintf(message, "....................Donwload Success.................. File save in %s\n", path);
	showBubbleNotify(message);
	printf("....................Donwload Success.................. File save in %s\n", path);
}

// communicate from client to server
// send and recv message with server
void communicateWithUser(){
	while(1) {
		if(!isOnline) {
			printf("\n=======================FileShareSystem==================\n");
                printf("\n\t\t\t1 Login");
                printf("\n\t\t\t2 Register");
                printf("\n\t\t\t3 Exit");
                printf("\n========================================================\n");
                printf("\nPlease choose: ");
            	scanf("%lc", &choose);
            	while(getchar() != '\n');
            	switch (choose){
            		case '1':
            			loginFunc(current_user);
            			break;
            		case '2':
            			registerFunc(current_user);
            			break;
            		case '3':
            			exit(0);
            		default:
            			printf("Syntax Error! Please choose again!\n");
            	}
		} else {
			 {
            	printf("\n=======================FileShareSystem==================\n");
                	printf("\n\t\t\t1 Search File In Shared System ");
                	printf("\n\t\t\t2 View Your List Files ");
                	printf("\n\t\t\t3 Create new file ");
                	printf("\n\t\t\t4 Logout ");
                	printf("\n========================================================");
                	printf("\nPlease choose: ");
            	scanf("%d", &choose);
            	while(getchar() != '\n');
            	switch (choose) {
            		case 1:
            			{
                        	char fileName[100];
                        	char selectedUser[30];
                        	char choose = '\0';
                        	printf("Please Input File Name You Want To Search: ");
                        	scanf("%[^\n]s", fileName);
                        	char fullPath[100];
                        	getFullPath(fileName, fullPath);
                        	while(getchar() != '\n');
                        	FILE *fptr;
                        	fptr = fopen(fullPath, "r");
                        	if(fptr != NULL) {
                        		while(1) {
                        			printf("\nYou have a file with same name!\n ---> Are you want to continue search? y/n: ");
                        			scanf("%c", &choose);
                        			while(getchar() != '\n');
                        			if((choose == 'y' || (choose == 'n'))) {
                        				break;
                        			}
                        		}
                        	}
                        	if(choose == 'n') {
                        		return;
                        	}
                        	mess->type = TYPE_REQUEST_FILE;
                        	strcpy(mess->payload, fileName);
                        	mess->length = strlen(mess->payload);
                        	sendMessage(client_sock, *mess);
                        	printf("\n.........................Please waiting.........................\n");
                        	receiveMessage(client_sock, mess);
                        	printf("%s\n",mess->payload);
                        	if(showListSelectUser(mess->payload, selectedUser, fileName) == 1) {
                        		handleDownloadFile(selectedUser, fileName);
                        	}
                        }
            			break;
            		case 2:
            			{
                        	DIR *dir;
                        	struct dirent *ent;
                        	char folderPath[1000];
                        	sprintf(folderPath, "./%s", current_user);
                        	if ((dir = opendir (folderPath)) != NULL) {
                        	  /* print all the files and directories within directory */
                        	  while ((ent = readdir (dir)) != NULL) {
                        	  	if(ent->d_name[0] != '.') {
                        	  		printf ("%s\n", ent->d_name);
                        	  	}
                        	  }
                        	  closedir (dir);
                        	} else {
                        	  /* could not open directory */
                        	  perror ("Permission denied!!");
                        	}
                        }
            			break;
            		case 3:{
            			char data[DATA_SIZE];
                        FILE * fptr;
                        char file_name[100];
                        printf("Please enter the file name: ");
                        scanf("%s",file_name);
                        char current_user_path[100];
                        strcpy(current_user_path,current_user);
                        strcat(current_user_path,"/");
                        fptr = fopen(strcat(current_user_path,file_name), "w");
                        if(fptr == NULL)
                           {
                                printf("Unable to create file.\n");
                                exit(0);
                            }

                        printf("Enter contents to store in file : \n");
                        scanf("%s", data);
                        fprintf(fptr, "%s", data);
                        fclose(fptr);
                        printf("%s...",data);
                        printf("File saved successfully.\n");
            			break;}
            		case 4:
            			logoutFunc(current_user);
            			break;
            		default:
            			printf("Syntax Error! Please choose again!\n");
            	}
            }
		}
	}
}
