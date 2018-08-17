/* Author: Joel Herrick
 * Class: CS344 Operating Systems I
 * Date: 8/16/2018
 * Description: otp_dec sends cipher text + key to server daemon.  otp_dec can only connect to otp_dec_d
*/

//includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

//global consts
#define MAX_BUFFER 150000

// Error function used for reporting issues
void error(const char *msg) { perror(msg); exit(0); } 

//main
int main(int argc, char *argv[])
{
	//define some vars
	int socketFD;
	int portNumber;
	int charsWritten;
	int charsRead;
	int fd;
	int msgLength, keyLength;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char msgBuffer[MAX_BUFFER/2];
	char keyBuffer[MAX_BUFFER/2];
	char sendBuffer[MAX_BUFFER];
    
	if (argc < 2) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	//printf("client opening files\n");

	//open and read message file
	fd = open(argv[1], O_RDONLY);
	if (fd == -1) { fprintf(stderr, "Cannot open file %s\n", argv[1]); exit(1); }
	msgLength = read(fd, msgBuffer, MAX_BUFFER);
	close(fd);

	//open and read key file
	fd = open(argv[2], O_RDONLY);
	if (fd == -1) { fprintf(stderr, "Cannot open file %s\n", argv[2]); exit(1); }
	keyLength = read(fd, keyBuffer, MAX_BUFFER);
	close(fd);

	//printf("client files read, checking keys\n");

	//error if key is shorter than msg
	if(keyLength < msgLength) {
		fprintf(stderr, "Error: key %s is too short\n", argv[2]);
		exit(1);
	}

	//printf("client checking for bad chars\n");

	//check for any bad characters in msg
	int i;
	for(i=0; i<msgLength; i++) {
		if((msgBuffer[i] < 65 || msgBuffer[i] > 90) && (msgBuffer[i] != 32 && msgBuffer[i] != 10)) {
			fprintf(stderr, "Error: input contains bad characters\n");
			exit(1);
		}
	}

	//printf("client adding delimeters\n");

	//add some delimeter chars so server can parse
	sendBuffer[0] = 'd';
	strcat(sendBuffer, msgBuffer);
	strcat(sendBuffer, "!!");
	strcat(sendBuffer, keyBuffer);
	strcat(sendBuffer, "@@");

	//printf("client connecting to server\n");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	//printf("client sending msg\n");

	// Send full message (plaintext + key) to server
	charsWritten = send(socketFD, sendBuffer, msgLength+keyLength+5, 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(sendBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//printf("%s\n", msgBuffer);

	//printf("I am client: sent msg\n");

	// Get return message from server
	memset(sendBuffer, '\0', sizeof(sendBuffer)); // Clear out the buffer again for reuse
	char buffer[1000];

	//receive decrypted text back from server in chunks
	while (strstr(sendBuffer, "@@") == NULL) {
		memset(buffer, '\0', sizeof(buffer));
		charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0);
		strcat(sendBuffer, buffer);
		//printf("PARENT: Message received from child: \"%s\", total: \"%s\"\n", buffer, completeMessage);
		if (charsRead == -1) { printf("charsRead == -1\n"); exit(1); }
		if (charsRead == 0) { exit(1);}
	}	

	//delete delimeter from end
	int terminalLocation = strstr(sendBuffer, "@@") - sendBuffer;
	sendBuffer[terminalLocation] = '\0';

	//charsRead = recv(socketFD, sendBuffer, sizeof(sendBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	//if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	
	//send response to stdout
	printf("%s\n", sendBuffer);

	close(socketFD); // Close the socket
	return 0;
}
