/* Author: Joel Herrick
 * Class: CS344 Operating Systems I
 * Date: 8/16/2018
 * Description: accepts number X as input and outputs key string of length X random UPPERCASE (or space) characters
*/

//includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//main
int main(int argc, char*argv[]) {
	//hint taken from Piazza to prevent duplicate keys generated within 1 second
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &time);

	//check # of args
	if(argc != 2) {
		fprintf(stderr, "Keygen requires 1 argument denoting key length\n");
		return 1;
	}

	//set up some vars
	int keyLength = atoi(argv[1]);
	char randomLetter;
	char key[keyLength+2];
	memset(key, '\0', keyLength+2);

	//printf("keylength: %d\n", keyLength);

	//seed the randomizer
	srand(time.tv_nsec);

	//loop and set key to a random letter A-Z or space.
	int i;
	for(i=0; i<keyLength; i++) {
		//randomLetter = NULL;
		randomLetter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "[rand() % 27];
		key[i] = randomLetter;
		//printf("%c\n", randomLetter);
	}

	//add a newline char to end
	key[keyLength] = '\n';

	//send to stdout
	printf("%s", key);

	//success!
	return 0;
}
