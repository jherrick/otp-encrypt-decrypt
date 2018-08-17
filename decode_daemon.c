/* Author: Joel Herrick
 * Class: CS344 Operating Systems I
 * Date: 8/16/2018
 * Description: otp_dec_d connects to otp_dec and decrypts cipher with provided key, then returns decrypted text.
 *              otp_dec_d refuses connections from otp_enc
*/

//includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//global consts
#define MAX_BUFFER 150000
#define CHUNK_SIZE 1000

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//main
int main(int argc, char *argv[]) {
	//define some vars
	int listenSocketFD;
	int establishedConnectionFD;
	int portNumber;
	int charsRead;
	socklen_t sizeOfClientInfo;
	char completeMessage[MAX_BUFFER];
	char buffer[CHUNK_SIZE];
	char msgBuffer[MAX_BUFFER/2];
	char keyBuffer[MAX_BUFFER/2];
	char decryptBuffer[MAX_BUFFER/2];
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;
	size_t buffer_size;
	int pid;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket"); 

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");

	if (listen(listenSocketFD, 5) < 0) error("ERROR on listening to port"); // Flip the socket on - it can now receive up to 5 connections


	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect

	//while loop to listen for connections
	while(1) {

		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");

		//upon connect create new child proc
		pid = fork();

		//switch example taken from prev lectures
		switch (pid) {
			//error
			case -1:
				printf("Hull Breach!  Erorr forking.\n");
				break;

			//child process
			case 0:
				//clear out all the buffers
				memset(completeMessage, '\0', sizeof(completeMessage));
				memset(msgBuffer, '\0', sizeof(msgBuffer));
				memset(keyBuffer, '\0', sizeof(keyBuffer));

				//receive the message from otp_dec in chunks
				while (strstr(completeMessage, "@@") == NULL) {
					memset(buffer, '\0', sizeof(buffer));
					charsRead = recv(establishedConnectionFD, buffer, sizeof(buffer)-1, 0);
					strcat(completeMessage, buffer);
					//printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", buffer, completeMessage);
					if (charsRead == -1) { printf("charsRead == -1\n"); break; }
					if (charsRead == 0) { printf("charsRead == 0\n"); break;}
				}

				//delete delimeter from end
				int terminalLocation = strstr(completeMessage, "@@") - completeMessage;
				completeMessage[terminalLocation] = '\0';

				//check to see we're communicating with right client
				if (completeMessage[0] != 'd') {
					fprintf(stderr, "otp_enc cannot connect to otp_dec_d!\n");
					exit(1);
				}

				//delete first char
				memmove(completeMessage, completeMessage+1, strlen(completeMessage));
				
				//split message and key by delimeter
				char *token;
				token = strtok(completeMessage, "!!");
				strcpy(msgBuffer, token);
				token = strtok(NULL, "!!");
				strcpy(keyBuffer, token);

				//clear out buffer
				memset(decryptBuffer, '\0', sizeof(msgBuffer));

				//some vars to help decrypt
				int msg;
				int key;
				int temp;
				int i;	

				//loop through the message 
				for(i=0; i<strlen(msgBuffer); i++){	
					//case msg space
					if(msgBuffer[i] == ' '){
						msg = 26;
					}
					else{
						msg = msgBuffer[i] - 65;
					}
					//case key space
					if(keyBuffer[i] == ' '){
						key = 26;
					}
					else{
						key = keyBuffer[i] - 65;
					}
					//calculate the character
					temp = (msg - key + 27) % 27;
					//place it if it's a space
					if(temp == 26){
						decryptBuffer[i] = ' ';
					}
					//otherwise calculate the char
					else{
						decryptBuffer[i] = 65 + (char)temp;
					}
				}

				//set last char to null terminator
				terminalLocation = strlen(decryptBuffer)-1;
				decryptBuffer[terminalLocation] = '\0';

				//prep it to be returned to client
				strcat(decryptBuffer, "@@");
				
				//printf("SERVER: I received this from the client: msg: \"%s\"\n key: \"%s\"\n", msgBuffer, keyBuffer);

				//printf("msg: %s\n", msgBuffer);
				//printf("key: %s\n", keyBuffer);

				// Send a decrypted message back to the client
				charsRead = send(establishedConnectionFD, decryptBuffer, sizeof(decryptBuffer), 0); // Send decryption back
				if (charsRead < 0) error("ERROR writing to socket");

				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				close(listenSocketFD); // Close the listening socket	
				exit(0);
				break;

			default:
				//parent, close the socket
				close(establishedConnectionFD);
		}
	}	

	return 0;
}