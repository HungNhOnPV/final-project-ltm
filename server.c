#include "serverFunc.c"

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
 	if(port_number<0 && port_number >65535) {
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
	printf("Server start...\n");
	int i = 0;
    	while(i < 1000) {
    		onlineClient[i].requestId = 0;    //set default value 0 for requestId
    		onlineClient[i].uploadSuccess = 0; //set default value 0 for uploadSuccess
    		i++;
    	}
	char username[254];
    	char password[32];
    	int status;
    	char c;
    	FILE* fIn;
    	fIn = fopen(ACCOUNT_FILE, "r");
    	if(!fIn) {
    		printf("File not exist??\n");
    		printf("End Program : File %s not existed\n", ACCOUNT_FILE);
            exit(0);
    	}

    	while(!feof(fIn)) {
    		if(fscanf(fIn, "%s %s %d%c", username, password, &status, &c) != EOF) {
    			append(createNewUser(username, password, status));
    		}
    		if(feof(fIn)) break;
    	}
    	fclose(fIn);
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
