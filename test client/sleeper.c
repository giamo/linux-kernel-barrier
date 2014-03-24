#include <stdlib.h>
#include <stdio.h>
#include "barrier_client.h"

int main(int argc, char **argv) {
	int key, tag, bd, ret;

	if (argc < 3) {
		printf("Usage: sleeper <barrier_key> <tag> \n");
		return -1;
	}
	key = atoi(argv[1]);
	tag = atoi(argv[2]);
	
	bd = get_barrier(key, O_CREAT);
	
	printf("Yawn, going to sleep on tag %d \n", tag);
	
	ret = sleep_on_barrier(bd, tag);
	if (ret == -1) {
		printf("Error: can't spleep on that barrier with that tag \n");
		return -1;
	}
	
	printf("I'm awake! \n");

	return 0;
}
