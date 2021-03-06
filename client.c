#include "clientFunc.c"
/*
* Main function
* @param int argc, char** argv
* @return 0
*/
int main(int argc, char const *argv[])
{

	// check valid of IP and port number
	if(argc!=3){
		printf("Error!\nPlease enter two parameter as IPAddress and port number!\n");
		exit(0);
	}
	char *serAddr = malloc(sizeof(argv[1]) * strlen(argv[1]));
	strcpy(serAddr, argv[1]);
	int port = atoi(argv[2]);
	mess = (Message*) malloc (sizeof(Message));
	mess->requestId = 0;
 	if(port<0 && port>65535) {
 		perror("Invalid Port Number!\n");
 		exit(0);
 	}
	if(!checkIP(serAddr)) {
		printf("Invalid Ip Address!\n"); // Check valid Ip Address
		exit(0);
	}
	strcpy(serAddr, argv[1]);
	if(!hasIPAddress(serAddr)) {
		printf("Not found information Of IP Address [%s]\n", serAddr); // Find Ip Address
		exit(0);
	}

	//Step 1: Construct socket
	int newsock = socket(AF_INET, SOCK_STREAM, 0);
    	if (newsock == -1 ){
    		perror("\nError: ");
    		exit(0);
    	}
	client_sock = newsock;
	//Step 2: Specify server address
	bindClient(port, serAddr);

	//Step 3: Request to connect server
    if(connect(client_sock, (struct sockaddr*) (&server_addr), sizeof(struct sockaddr)) < 0){
			printf("\nError!Can not connect to sever! Client exit imediately!\n");
			exit(0);
		}
	//Step 4: Communicate with server
	communicateWithUser();

	close(client_sock);
	return 0;
}
